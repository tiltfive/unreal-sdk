#include "TiltFiveXRBase.h"

#include "ClearQuad.h"
#include "PipelineStateCache.h"
#include "ScreenRendering.h"
#include "CommonRenderResources.h"
#include "DefaultSpectatorScreenController.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/EngineVersionComparison.h"
#include "XRThreadUtils.h"
#include "HMD/TiltFiveHMD.h"
#include "HMD/TiltFiveXRCamera.h"
#include "TiltFiveSpectatorController.h"
#include "TiltFiveManager.h"
#include "IXRCamera.h"
#include "Engine/GameInstance.h"

#include <thread>

#define ENGINE_SPECIFIC_HEADER_INNER(Prefix, Major, Minor, Suffix) \
	PREPROCESSOR_TO_STRING(PREPROCESSOR_JOIN(                      \
		PREPROCESSOR_JOIN(PREPROCESSOR_JOIN(PREPROCESSOR_JOIN(Prefix, _), Major), _), PREPROCESSOR_JOIN(Minor, Suffix)))
// #include ENGINE_SPECIFIC_HEADER(Example) creates an include command appending the current engine,
// e.g. for 4.20 it creates #include "Example_4_20.h"
#define ENGINE_SPECIFIC_HEADER(Prefix) ENGINE_SPECIFIC_HEADER_INNER(Prefix, ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION, .h)

#include ENGINE_SPECIFIC_HEADER(HMD/Engine/TiltFiveHMD)

FTiltFiveXRBase::FTiltFiveXRBase(IARSystemSupport* InARImplementation)
	: FXRTrackingSystemBase(InARImplementation)
{
	static const FName RendererModuleName("Renderer");
	RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);
}

FTiltFiveXRBase::~FTiltFiveXRBase()
{
	GlassesList.Empty();
}

void FTiltFiveXRBase::Startup()
{
	for (int32 i = 0; i < GMaxNumTiltFiveGlasses; i++) {
		GlassesList.Add(MakeShared<class FTiltFiveHMD, ESPMode::ThreadSafe>(this, i));
	}
	CustomPresent = new FTiltFiveCustomPresent(GlassesList);
	TiltFiveSceneViewExtension = FSceneViewExtensions::NewExtension<FTiltFiveSceneViewExtension>(this);
	SpectatorScreenController = MakeUnique<TiltFiveSpectatorController>(this);
	SpectatorScreenController->SetSpectatorScreenMode(ESpectatorScreenMode::Undistorted);
}

const FName FTiltFiveXRBase::TiltFiveSystemName(TEXT("TiltFiveHMD"));
FName FTiltFiveXRBase::GetSystemName() const
{
	return TiltFiveSystemName;
}

bool FTiltFiveXRBase::IsHeadTrackingAllowed() const
{
	return true;
}

bool FTiltFiveXRBase::IsVersionCompatible() const
{
	return GlassesList[0]->bVersionCompatible;
}

void FTiltFiveXRBase::SetSpectatedPlayer(int32 deviceId) const {
	if (deviceId < GMaxNumTiltFiveGlasses && deviceId >= 0) {
		if (GlassesList[deviceId]->IsHMDEnabled()) {
			currentSpectatedPlayer = deviceId;
		}
		else {
			UE_LOG(LogTiltFive, Error, TEXT("Specified Glasses index is not currently connected"));
			return;
		}
	}
	UE_LOG(LogTiltFive, Error, TEXT("Invalid Device Id"));
	return;
}

#if UE_VERSION_NEWER_THAN(4, 25, 4)
int32 FTiltFiveXRBase::GetXRSystemFlags() const
{
	return EXRSystemFlags::IsHeadMounted;
}
#endif

bool FTiltFiveXRBase::DoesSupportPositionalTracking() const
{
	return true;
}

bool FTiltFiveXRBase::DoesSupportLateUpdate() const
{
	return true;
}

bool FTiltFiveXRBase::HasValidTrackingPosition()
{
	FQuat Orientation;
	FVector Position;
	return GetCurrentPose(HMDDeviceId, Orientation, Position);
}

bool FTiltFiveXRBase::EnumerateTrackedDevices(TArray<int32>& OutDevices, EXRTrackedDeviceType Type /*= EXRTrackedDeviceType::Any*/)
{
	if (Type == EXRTrackedDeviceType::HeadMountedDisplay || Type == EXRTrackedDeviceType::Any)
	{
		for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> Hmd : GlassesList) {
			if (Hmd->IsHMDEnabled()) {
				OutDevices.Add(Hmd->DeviceId);
			}
		}
		return true;
	}
	return false;
}

bool FTiltFiveXRBase::GetCurrentPose(int32 DeviceId, FQuat& OutOrientation, FVector& OutPosition)
{
	if (IsInRenderingThread() && GlassesList[DeviceId]->CachedGlassesPoseIsValid_RenderThread)
	{
		OutOrientation = GlassesList[DeviceId]->CachedGlassesOrientation_RenderThread;
		OutPosition = GlassesList[DeviceId]->CachedGlassesPosition_RenderThread;
		return true;
	}
	else if (IsInGameThread() && GlassesList[DeviceId]->CachedGlassesPoseIsValid_GameThread)
	{
		OutOrientation = GlassesList[DeviceId]->CachedGlassesOrientation_GameThread;
		OutPosition = GlassesList[DeviceId]->CachedGlassesPosition_GameThread;
		return true;
	}
	OutOrientation = FQuat::Identity;
	OutPosition = FVector::ZeroVector;
	return false;
}

#if UE_VERSION_OLDER_THAN(5, 0, 0)
bool FTiltFiveXRBase::GetRelativeEyePose(int32 DeviceId, EStereoscopicPass Eye, FQuat& OutOrientation, FVector& OutPosition)
{
	OutOrientation = FQuat::Identity;
	OutPosition = FVector::ZeroVector;
	if (Eye == eSSP_LEFT_EYE || Eye == eSSP_RIGHT_EYE)
	{
		OutPosition = FVector(
			0, (Eye == EStereoscopicPass::eSSP_LEFT_EYE ? -.5 : .5) * GlassesList[DeviceId]->GetInterpupillaryDistance() * GlassesList[DeviceId]->GetWorldToMetersScale(), 0);
		return true;
	}
	else
	{
		return false;
	}
}
#else
bool FTiltFiveXRBase::GetRelativeEyePose(int32 DeviceId, int32 ViewIndex, FQuat& OutOrientation, FVector& OutPosition)
{
	OutOrientation = FQuat::Identity;
	OutPosition = FVector::ZeroVector;
	if (ViewIndex == EStereoscopicEye::eSSE_LEFT_EYE || ViewIndex == EStereoscopicEye::eSSE_RIGHT_EYE)
	{
		OutPosition = FVector(0,
			(ViewIndex == EStereoscopicEye::eSSE_LEFT_EYE ? -.5 : .5) * GlassesList[DeviceId]->GetInterpupillaryDistance() * GlassesList[DeviceId]->GetWorldToMetersScale(),
			0);
		return true;
	}
	else
	{
		return false;
	}
}
#endif

void FTiltFiveXRBase::SetTrackingOrigin(EHMDTrackingOrigin::Type NewOrigin)
{
	if (NewOrigin != EHMDTrackingOrigin::Floor)
	{
		UE_LOG(LogTiltFive, Error, TEXT("Tilt Five only supports Floor Level Tracking Origin"));
	}
}

#if UE_VERSION_NEWER_THAN(4, 22, 3)
EHMDTrackingOrigin::Type FTiltFiveXRBase::GetTrackingOrigin() const
{
	return EHMDTrackingOrigin::Floor;
}
#endif

float FTiltFiveXRBase::GetWorldToMetersScale() const
{
	FTiltFiveWorldStatePtr CurrentState;
	if (IsInRenderingThread())
	{
		CurrentState = RenderThreadWorldState;
	}
	else if (IsInGameThread())
	{
		CurrentState = CurrentWorldState;
	}
	else
	{
		// Fallback to default value
		return 100.0f;
	}

	if (!CurrentState.IsValid())
	{
		// Fallback to default value
		return 100.0f;
	}

	return CurrentState->WorldToMetersScale;
}

void FTiltFiveXRBase::ResetOrientationAndPosition(float Yaw /*= 0.f*/)
{
	unimplemented();
}

TSharedPtr< class IXRCamera, ESPMode::ThreadSafe > FTiltFiveXRBase::GetXRCamera(int32 DeviceId)
{
	if (GlassesList[DeviceId]->IsHMDEnabled()) {
		return GlassesList[DeviceId]->GetXRCamera();
	}
	return nullptr;
}

class IHeadMountedDisplay* FTiltFiveXRBase::GetHMDDevice()
{
	return GlassesList[0]->GetHMDDevice();
}

class IHeadMountedDisplay* FTiltFiveXRBase::GetHMDDevice(int32 DeviceId)
{
	return GlassesList[DeviceId]->GetHMDDevice();
}

class TSharedPtr<class IStereoRendering, ESPMode::ThreadSafe> FTiltFiveXRBase::GetStereoRenderingDevice()
{
	return SharedThis(this);
}

bool FTiltFiveXRBase::OnStartGameFrame(FWorldContext& WorldContext)
{
	const UWorld* World = WorldContext.World();

	if (!World || !World->IsGameWorld())
	{
		// ignore all non-game worlds
		return false;
	}

	const float CurrentTime = static_cast<float>(FApp::GetCurrentTime());
	// If the current time for some reason gets reset, we also reset our timer,
	// otherwise we may wait a very long time here
	if (LastGlassesUpdate > CurrentTime)
	{
		LastGlassesUpdate = CurrentTime;
	}
	// Poll Glasses once per second
	if (LastGlassesUpdate + 1.0f < CurrentTime)
	{
		LastGlassesUpdate = CurrentTime;

		for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> Hmd : GlassesList) {
			Hmd->ConditionalInitializeGlasses();
		}
	}

	CurrentWorldState.Reset();

	if (const AWorldSettings* WorldSettings = WorldContext.World()->GetWorldSettings())
	{
		FTiltFiveWorldState WorldState;
		WorldState.WorldToMetersScale = WorldSettings->WorldToMeters;
		CurrentWorldState = MakeShared<FTiltFiveWorldState, ESPMode::ThreadSafe>(WorldState);
	}

	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> Hmd : GlassesList) {
		if (Hmd->IsHMDEnabled()) {
			Hmd->CurrentWorldState = CurrentWorldState;
			Hmd->UpdateCachedGlassesPose_GameThread();
			FQuat GlassesOrientation;
			FVector GlassesPosition;
			TOptional<FTransform> MaybeRelativeGlassesTransform;

			if (GetCurrentPose(Hmd->DeviceId, GlassesOrientation, GlassesPosition))
			{
				MaybeRelativeGlassesTransform = FTransform(GlassesOrientation, GlassesPosition);
			}
			FTiltFiveWorldStatePtr RenderThreadCopy = MakeShared<FTiltFiveWorldState, ESPMode::ThreadSafe>(*CurrentWorldState);

			ExecuteOnRenderThread_DoNotWait(
				[this, RenderThreadCopy, MaybeRelativeGlassesTransform, Hmd](FRHICommandListImmediate& RHICmdList)
				{
					Hmd->RenderThreadWorldState = RenderThreadCopy;
					Hmd->MaybeRelativeGlassesTransform_RenderThread = MaybeRelativeGlassesTransform;
				});
		}
	}

	if (SpectatorScreenController)
	{
		SpectatorScreenController->SetSpectatorScreenMode(ESpectatorScreenMode::Undistorted);
	}

	RefreshTrackingToWorldTransform(WorldContext);

	return false;
}

void FTiltFiveXRBase::OnBeginRendering_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily)
{
	check(IsInRenderingThread());

	if (SpectatorScreenController)
	{
		SpectatorScreenController->UpdateSpectatorScreenMode_RenderThread();
	}
}

void FTiltFiveXRBase::OnLateUpdateApplied_RenderThread(
#if ENGINE_MINOR_VERSION >= 27 || ENGINE_MAJOR_VERSION == 5
	FRHICommandListImmediate& RHICmdList,
#endif	  // ENGINE_MINOR_VERSION >= 27
	const FTransform& NewRelativeTransform)
{
	check(IsInRenderingThread());

	GlassesList[0]->MaybeRelativeGlassesTransform_RenderThread = NewRelativeTransform;
}

void FTiltFiveXRBase::OnLateUpdateApplied_RenderThread(
#if ENGINE_MINOR_VERSION >= 27 || ENGINE_MAJOR_VERSION == 5
	FRHICommandListImmediate& RHICmdList,
#endif	  // ENGINE_MINOR_VERSION >= 27
	const FTransform& NewRelativeTransform,
	int32 DeviceId)
{
	check(IsInRenderingThread());

	GlassesList[DeviceId]->MaybeRelativeGlassesTransform_RenderThread = NewRelativeTransform;
}

bool FTiltFiveXRBase::IsStereoEnabled() const
{
	return bEnableStereo;
}

bool FTiltFiveXRBase::EnableStereo(bool stereo /*= true*/)
{
	bEnableStereo = true;
	return true;
}

#if UE_VERSION_OLDER_THAN(5, 0, 0)
void FTiltFiveXRBase::AdjustViewRect(enum EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const
{
	if (StereoPass == eSSP_FULL)
	{
		return;
	}
	const int32 EyeIndex = StereoPass == EStereoscopicPass::eSSP_RIGHT_EYE ? 1 : 0;
	FIntRect Rect = EyeInfos[EyeIndex].SourceEyeRect;
	X = Rect.Min.X;
	Y = Rect.Min.Y;
	SizeX = Rect.Width();
	SizeY = Rect.Height();
	stereoPassCount++;
	int activeGlassesCount = 0;
	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> HMD : GlassesList) {
		if (HMD->IsHMDEnabled()) {
			activeGlassesCount++;
		}
	}
	if (stereoPassCount >= (activeGlassesCount * 2)) {
		stereoPassCount = 0;
	}
}
#else
void FTiltFiveXRBase::AdjustViewRect(int32 ViewIndex, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const
{
	if (ViewIndex != EStereoscopicEye::eSSE_RIGHT_EYE && ViewIndex != EStereoscopicEye::eSSE_LEFT_EYE)
	{
		return;
	}
	
	const int32 EyeIndex = ViewIndex == EStereoscopicEye::eSSE_RIGHT_EYE ? 1 : 0;
	FIntRect Rect = GlassesList[stereoPassCount/2]->EyeInfos[EyeIndex].SourceEyeRect;
	X = Rect.Min.X;
	Y = Rect.Min.Y;
	SizeX = Rect.Width();
	SizeY = Rect.Height();
	stereoPassCount++;
	int activeGlassesCount = 0;
	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> HMD : GlassesList) {
		if (HMD->IsHMDEnabled()) {
			activeGlassesCount++;
		}
	}
	if (stereoPassCount >= (activeGlassesCount*2)) {
		stereoPassCount = 0;
	}
}

void FTiltFiveXRBase::AdjustViewRect(int32 ViewIndex, int32& X, int32& Y, uint32& SizeX, uint32& SizeY, int32 playerIndex) const
{
	if (ViewIndex != EStereoscopicEye::eSSE_RIGHT_EYE && ViewIndex != EStereoscopicEye::eSSE_LEFT_EYE)
	{
		return;
	}

	const int32 EyeIndex = ViewIndex == EStereoscopicEye::eSSE_RIGHT_EYE ? 1 : 0;
	FIntRect Rect = GlassesList[playerIndex]->EyeInfos[EyeIndex].SourceEyeRect;
	X = Rect.Min.X;
	Y = Rect.Min.Y;
	SizeX = Rect.Width();
	SizeY = Rect.Height();
}
#endif

FVector2D FTiltFiveXRBase::GetTextSafeRegionBounds() const
{
	return FVector2D(0.75f, 0.75f);
}

#if UE_VERSION_OLDER_THAN(5, 0, 0)
FMatrix FTiltFiveXRBase::GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType) const
{
	const int32 EyeIndex = StereoPassType == EStereoscopicPass::eSSP_RIGHT_EYE ? 1 : 0;

	const float HalfFov = FMath::DegreesToRadians(FOV) / 2.f;
	const float HalfFovTan = FMath::Tan(HalfFov);
	const float InWidth = EyeInfos[EyeIndex].DestEyeRect.Width();
	const float InHeight = EyeInfos[EyeIndex].DestEyeRect.Height();
	const float XS = 1.0f / HalfFovTan;
	const float YS = InWidth / HalfFovTan / InHeight;

	const float InNearZ = GNearClippingPlane;
	return FMatrix(FPlane(XS, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, YS, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, 0.0f, 1.0f),
		FPlane(0.0f, 0.0f, InNearZ, 0.0f));
}
#else
FMatrix FTiltFiveXRBase::GetStereoProjectionMatrix(const int32 ViewIndex) const
{
	const int32 EyeIndex = ViewIndex == EStereoscopicEye::eSSE_RIGHT_EYE ? 1 : 0;

	const float HalfFov = FMath::DegreesToRadians(GlassesList[stereoPassCount]->FOV) / 2.f;
	const float HalfFovTan = FMath::Tan(HalfFov);
	const float InWidth = GlassesList[stereoPassCount]->EyeInfos[EyeIndex].DestEyeRect.Width();
	const float InHeight = GlassesList[stereoPassCount]->EyeInfos[EyeIndex].DestEyeRect.Height();
	const float XS = 1.0f / HalfFovTan;
	const float YS = InWidth / HalfFovTan / InHeight;

	const float InNearZ = GNearClippingPlane;
	return FMatrix(FPlane(XS, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, YS, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, 0.0f, 1.0f),
		FPlane(0.0f, 0.0f, InNearZ, 0.0f));
}
FMatrix FTiltFiveXRBase::GetStereoProjectionMatrix(const int32 ViewIndex, const int32 playerIndex) const
{
	const int32 EyeIndex = ViewIndex == EStereoscopicEye::eSSE_RIGHT_EYE ? 1 : 0;

	const float HalfFov = FMath::DegreesToRadians(GlassesList[playerIndex]->FOV) / 2.f;
	const float HalfFovTan = FMath::Tan(HalfFov);
	const float InWidth = GlassesList[playerIndex]->EyeInfos[EyeIndex].DestEyeRect.Width();
	const float InHeight = GlassesList[playerIndex]->EyeInfos[EyeIndex].DestEyeRect.Height();
	const float XS = 1.0f / HalfFovTan;
	const float YS = InWidth / HalfFovTan / InHeight;

	const float InNearZ = GNearClippingPlane;
	return FMatrix(FPlane(XS, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, YS, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, 0.0f, 1.0f),
		FPlane(0.0f, 0.0f, InNearZ, 0.0f));
}
#endif

void FTiltFiveXRBase::InitCanvasFromView(class FSceneView* InView, class UCanvas* Canvas)
{
	FSceneView HMDView(*InView);

	Canvas->UpdateSafeZoneData();

	const FQuat DeltaRot = HMDView.BaseHmdOrientation.Inverse() * Canvas->HmdOrientation;
	HMDView.ViewRotation = FRotator(HMDView.ViewRotation.Quaternion() * DeltaRot);

	FVector StereoViewLocation = HMDView.ViewLocation;
	FRotator StereoViewRotation = HMDView.ViewRotation;
	if (GEngine->StereoRenderingDevice.IsValid() && IStereoRendering::IsStereoEyePass(InView->StereoPass))
	{
		CalculateStereoViewOffset(HMDView.StereoViewIndex, StereoViewRotation, HMDView.WorldToMetersScale, StereoViewLocation, HMDView.PlayerIndex);
		HMDView.ViewLocation = StereoViewLocation;
		HMDView.ViewRotation = StereoViewRotation;
	}

	HMDView.ViewMatrices.UpdateViewMatrix(StereoViewLocation, StereoViewRotation);
	GetViewFrustumBounds(HMDView.ViewFrustum, HMDView.ViewMatrices.GetViewProjectionMatrix(), false);

	// We need to keep ShadowViewMatrices in sync.
	HMDView.ShadowViewMatrices = HMDView.ViewMatrices;
	Canvas->ViewProjectionMatrix = HMDView.ViewMatrices.GetViewProjectionMatrix();
}

void FTiltFiveXRBase::RenderTexture_RenderThread(class FRHICommandListImmediate& RHICmdList,
	class FRHITexture* BackBuffer,
	class FRHITexture* SrcTexture,
	FVector2D WindowSize) const
{
	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> HMD : GlassesList) {
		if(HMD->IsHMDEnabled())
		for (int32 EyeIndex = 0; EyeIndex < NumEyeRenderTargets; ++EyeIndex)
		{
			const FIntRect DestEyeRect(
				0, 0, HMD->EyeInfos[EyeIndex].BufferedRTRHI->GetSizeX(), HMD->EyeInfos[EyeIndex].BufferedRTRHI->GetSizeY());
			CopyTexture_RenderThread(RHICmdList,
				SrcTexture,
				HMD->EyeInfos[EyeIndex].SourceEyeRect,
				HMD->EyeInfos[EyeIndex].BufferedRTRHI,
				DestEyeRect,
				false
#if UE_VERSION_NEWER_THAN(4, 20, 3)
				,
				true
#endif
			);

#if UE_VERSION_NEWER_THAN(4, 25, 4)
			RHICmdList.Transition(FRHITransitionInfo(HMD->EyeInfos[EyeIndex].BufferedSRVRHI, ERHIAccess::WritableMask, ERHIAccess::SRVMask));
#else
			FRHITexture* Texture = EyeInfos[EyeIndex].BufferedRTRHI;
			RHICmdList.TransitionResources(EResourceTransitionAccess::EReadable, &Texture, 1);
#endif
		}
	}
	if (SpectatorScreenController)
	{
		SpectatorScreenController->RenderSpectatorScreen_RenderThread(RHICmdList, BackBuffer, GlassesList[currentSpectatedPlayer]->EyeInfos[0].BufferedSRVRHI, WindowSize);
	}
}

IStereoRenderTargetManager* FTiltFiveXRBase::GetRenderTargetManager()
{
	return this;
}

void FTiltFiveXRBase::CalculateStereoViewOffset(const int32 ViewIndex, FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation) {
	
	TSharedPtr<class IXRCamera, ESPMode::ThreadSafe> HMDCamera = GetXRCamera(stereoPassCount);
	if (HMDCamera.IsValid())
	{
		HMDCamera->CalculateStereoCameraOffset(ViewIndex, ViewRotation, ViewLocation);
	}
	stereoPassCount++;
	int activeGlassesCount = 0;
	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> HMD : GlassesList) {
		if (HMD->IsHMDEnabled()) {
			activeGlassesCount++;
		}
	}
	if (stereoPassCount >= (activeGlassesCount * 2)) {
		stereoPassCount = 0;
	}
}

void FTiltFiveXRBase::CalculateStereoViewOffset(const int32 ViewIndex, FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation, int32 playerIndex) {
	TSharedPtr<class IXRCamera, ESPMode::ThreadSafe> HMDCamera = GetXRCamera(playerIndex);
	if (HMDCamera.IsValid())
	{
		HMDCamera->CalculateStereoCameraOffset(ViewIndex, ViewRotation, ViewLocation);
	}
}

uint32 FTiltFiveXRBase::CreateLayer(const FLayerDesc& InLayerDesc)
{
	unimplemented();
	return INDEX_NONE;
}

void FTiltFiveXRBase::DestroyLayer(uint32 LayerId)
{
	unimplemented();
}

void FTiltFiveXRBase::SetLayerDesc(uint32 LayerId, const FLayerDesc& InLayerDesc)
{
	unimplemented();
}

bool FTiltFiveXRBase::GetLayerDesc(uint32 LayerId, FLayerDesc& OutLayerDesc)
{
	unimplemented();
	return false;
}

void FTiltFiveXRBase::MarkTextureForUpdate(uint32 LayerId)
{
	unimplemented();
}

#if UE_VERSION_NEWER_THAN(4, 21, 2)
bool FTiltFiveXRBase::ShouldCopyDebugLayersToSpectatorScreen() const
{
	return false;
}
#endif

bool FTiltFiveXRBase::ShouldUseSeparateRenderTarget() const
{
	return IsStereoEnabled();
}

#if UE_VERSION_NEWER_THAN(4, 25, 4)
bool FTiltFiveXRBase::AllocateRenderTargetTexture(uint32 Index,
	uint32 SizeX,
	uint32 SizeY,
	uint8 Format,
	uint32 NumMips,
	ETextureCreateFlags Flags,
	ETextureCreateFlags TargetableTextureFlags,
	FTexture2DRHIRef& OutTargetableTexture,
	FTexture2DRHIRef& OutShaderResourceTexture,
	uint32 NumSamples /*= 1*/)
{
	FRHIResourceCreateInfo CreateInfo{ TEXT("TiltFiveCreateInfo") };
	const int32 EyeSizeX = SizeX / 2;
	const int32 EyeSizeY = SizeY / 4;
	GlassesList[0]->WidthToHeight = (float)EyeSizeX / (float)EyeSizeY;

#if UE_VERSION_NEWER_THAN(5, 2, 0)
	FRHITextureCreateDesc Desc =
		FRHITextureCreateDesc::Create2D(TEXT("TiltFiveCreateInfo"))
		.SetExtent(EyeSizeX, EyeSizeY)
		.SetFormat(PF_R8G8B8A8)
		.SetFlags(ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::ShaderResource);
#endif

	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> HMD : GlassesList) {
		for (int32 EyeIndex = 0; EyeIndex < NumEyeRenderTargets; ++EyeIndex)
		{
#if !UE_BUILD_SHIPPING
			HMD->EyeInfos[EyeIndex].DebugName = FString::Printf(TEXT("TiltFiveEyeRT%d"), EyeIndex);
			CreateInfo.DebugName = *HMD->EyeInfos[EyeIndex].DebugName;
#endif
			HMD->EyeInfos[EyeIndex].SourceEyeRect = FIntRect(EyeSizeX * EyeIndex, (EyeSizeY * HMD->DeviceId), EyeSizeX * EyeIndex + EyeSizeX, (EyeSizeY * HMD->DeviceId) + EyeSizeY);
			HMD->EyeInfos[EyeIndex].DestEyeRect = FIntRect(0, 0, EyeSizeX, EyeSizeY);

#if UE_VERSION_NEWER_THAN(5, 2, 0)
			FTexture2DRHIRef Texture = RHICreateTexture(Desc);
			HMD->EyeInfos[EyeIndex].BufferedRTRHI = Texture;
			HMD->EyeInfos[EyeIndex].BufferedSRVRHI = Texture;
#else

			RHICreateTargetableShaderResource2D(EyeSizeX,
				EyeSizeY,
				PF_R8G8B8A8,
				1,
				TexCreate_None,
				TexCreate_RenderTargetable,
				false,
				CreateInfo,
				EyeInfos[EyeIndex].BufferedRTRHI,
				EyeInfos[EyeIndex].BufferedSRVRHI);
#endif
		}
	}

	// Create the native buffer for rendering using the default handling, so return false here
	return false;
}
#endif

void FTiltFiveXRBase::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	check(IsInGameThread());
	if (SpectatorScreenController != nullptr)
	{
		SpectatorScreenController->BeginRenderViewFamily();
	}
}

class FXRRenderBridge* FTiltFiveXRBase::GetActiveRenderBridge_GameThread(bool bUseSeparateRenderTarget)
{
	return CustomPresent;
}

FVector2D FTiltFiveXRBase::GetEyeCenterPoint_RenderThread(const int32 ViewIndex) const
{
	check(IsInRenderingThread());

	// Note: IsHeadTrackingAllowed() can only be called from the game thread.
	// IsStereoEnabled() and IsHeadTrackingEnforced() can be called from both the render and game threads, however.
	if (!(IsStereoEnabled() || IsHeadTrackingEnforced()))
	{
		return FVector2D(0.5f, 0.5f);
	}

	const FMatrix StereoProjectionMatrix = GetStereoProjectionMatrix(ViewIndex);
	//0,0,1 is the straight ahead point, wherever it maps to is the center of the projection plane in -1..1 coordinates.  -1,-1 is bottom left.
	const FVector4 ScreenCenter = StereoProjectionMatrix.TransformPosition(FVector(0.0f, 0.0f, 1.0f));
	//transform into 0-1 screen coordinates 0,0 is top left.  
	const FVector2D CenterPoint(0.5f + (ScreenCenter.X / 2.0f), 0.5f - (ScreenCenter.Y / 2.0f));
	return CenterPoint;
}

FTiltFiveCustomPresent::FTiltFiveCustomPresent(TArray<TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe>> InGlassesList)
	: GlassesList(InGlassesList)
{
}

bool FTiltFiveCustomPresent::NeedsNativePresent()
{
	return true;
}

bool FTiltFiveCustomPresent::Present(int32& InOutSyncInterval)
{
	//UE_LOG(LogTiltFive, Error, TEXT("Running Custom Present"));
	// XXX: This currently passes, but is this always going to be called on the render thread?
	check(IsInRenderingThread());
	// XXX: Important note: Custom Present currently runs on the "RHI" thread, which, when using
	// DirectX 11 is synonymous with the render thread. Unreal 5.0 forward supported DirectX 12
	// by default. When this happens, we are no longer on the render thread, and are wholly on
	// the RHI thread. We don't currently support DirectX 12, but once we do, we need to refactor
	// this section to make sure the pose is accessible on the RHI thread specifically, separate
	// from the render thread.

	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> HMD : GlassesList) {
		if (!HMD->IsHMDEnabled() || !HMD->MaybeRelativeGlassesTransform_RenderThread.IsSet())
		{
			continue;
		}

		FQuat GlassesOrientation = HMD->MaybeRelativeGlassesTransform_RenderThread->GetRotation();
		FVector GlassesPosition = HMD->MaybeRelativeGlassesTransform_RenderThread->GetTranslation();

		FT5FrameInfo FrameInfo;

		const float WorldToMetersScale = HMD->GetWorldToMetersScale();

		float IPD_UWRLD = HMD->GetInterpupillaryDistance() * WorldToMetersScale;

		FTiltFiveHMD::ConvertPositionToHardware(
			GlassesPosition - GlassesOrientation.GetRightVector() * IPD_UWRLD * 0.5f, FrameInfo.posLVC_GBD, WorldToMetersScale);
		FTiltFiveHMD::ConvertPositionToHardware(
			GlassesPosition + GlassesOrientation.GetRightVector() * IPD_UWRLD * 0.5f, FrameInfo.posRVC_GBD, WorldToMetersScale);

		FTiltFiveHMD::ConvertRotationToHardware(GlassesOrientation, FrameInfo.rotToLVC_GBD);
		FTiltFiveHMD::ConvertRotationToHardware(GlassesOrientation, FrameInfo.rotToRVC_GBD);

		FrameInfo.vci.startX_VCI = -FMath::Tan(HMD->FOV * (0.5f * PI / 180.0f));
		FrameInfo.vci.startY_VCI = FrameInfo.vci.startX_VCI / HMD->WidthToHeight;
		FrameInfo.vci.width_VCI = -2.0f * FrameInfo.vci.startX_VCI;
		FrameInfo.vci.height_VCI = -2.0f * FrameInfo.vci.startY_VCI;

		FrameInfo.isUpsideDown = true;
		FrameInfo.isSrgb = EnumHasAnyFlags(HMD->EyeInfos[0].BufferedSRVRHI->GetFlags(), TexCreate_SRGB) != 0;
		FrameInfo.leftTexHandle = HMD->EyeInfos[0].BufferedSRVRHI->GetNativeResource();
		FrameInfo.rightTexHandle = HMD->EyeInfos[1].BufferedSRVRHI->GetNativeResource();
		FrameInfo.texWidth_PIX = HMD->EyeInfos[0].BufferedSRVRHI->GetSizeX();
		FrameInfo.texHeight_PIX = HMD->EyeInfos[0].BufferedSRVRHI->GetSizeY();

		if (HMD->CurrentExclusiveGlasses)
		{
			FScopeLock ScopeLock(&(HMD->ExclusiveGroup1CriticalSection));

			if (HMD->CurrentExclusiveGlasses != HMD->GraphicsInitializedGlasses)
			{
				// Obtain a graphics context
				ET5GraphicsAPI GraphicsAPI{};
				void* GraphicsContext{};
				const FString RHIName = GDynamicRHI->GetName();

				GraphicsAPI = kT5_GraphicsApi_None;
				GraphicsContext = nullptr;
				if (RHIName == TEXT("D3D11"))
				{
					GraphicsAPI = kT5_GraphicsApi_D3D11;
					GraphicsContext = GDynamicRHI->RHIGetNativeDevice();
				}
				else if (RHIName == TEXT("OpenGL"))
				{
					GraphicsAPI = kT5_GraphicsApi_GL;
				}
				UE_LOG(LogTiltFive, Error, TEXT("Initializing Graphics Context"));
				FT5Result Result = t5InitGlassesGraphicsContext(HMD->CurrentExclusiveGlasses, GraphicsAPI, GraphicsContext);
				if (Result != T5_SUCCESS)
				{
					UE_LOG(LogTiltFive, Error, TEXT("Failed to initialize graphics context"));
					return false;
				}

				HMD->GraphicsInitializedGlasses = HMD->CurrentExclusiveGlasses;
			}
			const FT5Result Result = t5SendFrameToGlasses(HMD->CurrentExclusiveGlasses, &FrameInfo);

			UE_CLOG(Result != T5_SUCCESS, LogTiltFive, Error, TEXT("Failed to send frame: %S"), t5GetResultMessage(Result));
		}
	}
	return true;
}

//This needs to check all glasses to make sure they have textures assigned
void FTiltFiveCustomPresent::UpdateViewport(const class FViewport& Viewport, class FRHIViewport* InViewportRHI)
{
	FXRRenderBridge::UpdateViewport(Viewport, InViewportRHI);

	if (GlassesList[0]->EyeInfos[0].BufferedRTRHI)
	{
		return;
	}
	const FIntPoint Size = GlassesList[0]->GetIdealRenderTargetSize();

	FRHIResourceCreateInfo CreateInfo{ TEXT("TiltFiveCreateInfo") };
	const int32 EyeSizeX = Size.X / 2;
	const int32 EyeSizeY = Size.Y;
	GlassesList[0]->WidthToHeight = (float)EyeSizeX / (float)EyeSizeY;

#if UE_VERSION_NEWER_THAN(5, 2, 0)
	FRHITextureCreateDesc Desc =
		FRHITextureCreateDesc::Create2D(TEXT("TiltFiveCreateInfo"))
		.SetExtent(EyeSizeX, EyeSizeY)
		.SetFormat(PF_R8G8B8A8)
		.SetFlags(ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::ShaderResource);
#endif
	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> HMD : GlassesList) {
		for (int32 EyeIndex = 0; EyeIndex < NumEyeRenderTargets; ++EyeIndex)
		{
			FTiltFiveEyeInfo& EyeInfo = HMD->EyeInfos[EyeIndex];
#if !UE_BUILD_SHIPPING
			EyeInfo.DebugName = FString::Printf(TEXT("TiltFiveEyeRT%d"), EyeIndex);
			CreateInfo.DebugName = *EyeInfo.DebugName;
#endif
			EyeInfo.SourceEyeRect = FIntRect(EyeSizeX * EyeIndex, (EyeSizeY * HMD->DeviceId), EyeSizeX * EyeIndex + EyeSizeX, (EyeSizeY * HMD->DeviceId) + EyeSizeY);
			EyeInfo.DestEyeRect = FIntRect(0, 0, EyeSizeX, EyeSizeY);

#if UE_VERSION_OLDER_THAN(5, 3, 0)
			RHICreateTargetableShaderResource2D(EyeSizeX,
				EyeSizeY,
				PF_R8G8B8A8,
				1,
				TexCreate_None,
				TexCreate_RenderTargetable,
				false,
				CreateInfo,
				EyeInfo.BufferedRTRHI,
				EyeInfo.BufferedSRVRHI);
#else
			FTexture2DRHIRef Texture = RHICreateTexture(Desc);
			EyeInfo.BufferedRTRHI = Texture;
			EyeInfo.BufferedSRVRHI = Texture;
#endif
		}
	}
}