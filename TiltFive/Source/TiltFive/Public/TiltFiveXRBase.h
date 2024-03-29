// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HeadMountedDisplayBase.h"
#include "XRTrackingSystemBase.h"
#include "IStereoLayers.h"
#include "SceneViewExtension.h"
#include "Misc/EngineVersionComparison.h"
#include "XRRenderBridge.h"
#include "XRRenderTargetManager.h"
#include "IXRTrackingSystem.h"
#include "TiltFiveSpectatorController.h"
#include "Runtime/Launch/Resources/Version.h"

#include <atomic>

class FTiltFiveXRBase;

namespace ETiltFiveDeviceId
{
	enum Type
	{
		Invalid = INDEX_NONE,
		// 0 should represent the default HMD Device, see IXRTrackingSystem::HMDDeviceId
		Player1 = 0,
		Player2 = 1,
		Player3 = 2,
		Player4 = 3,
	};
}

class FTiltFiveCustomPresent : public FXRRenderBridge
{
public:
	FTiltFiveCustomPresent(TArray<TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe>> GlassesList);
	virtual bool NeedsNativePresent() override;
	virtual bool Present(int32& InOutSyncInterval) override;

	void UpdateViewport(const class FViewport& Viewport, class FRHIViewport* InViewportRHI) override;

private:
	TArray<TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe>> GlassesList;
};

/**
 * This class is responsible for tracking each Tilt Five HMD as part of a list. ALL HMDs will exist by default, but their cameras will be created or destroyed as headsets connect.
 */
class TILTFIVE_API FTiltFiveXRBase : public FXRTrackingSystemBase,
									 public IStereoRendering,
									 public IStereoLayers,
									 public FXRRenderTargetManager,
									 public TSharedFromThis<FTiltFiveXRBase, ESPMode::ThreadSafe>
{
public:
	FTiltFiveXRBase(IARSystemSupport* InARImplementation);
	virtual ~FTiltFiveXRBase();

	void Startup();

	virtual FName GetSystemName() const override;
	virtual bool IsHeadTrackingAllowed() const override;

	virtual bool IsVersionCompatible() const;
	virtual void SetSpectatedPlayer(int32 DeviceId) const;

	virtual int32 GetXRSystemFlags() const override;
	virtual bool DoesSupportPositionalTracking() const override;
	virtual bool DoesSupportLateUpdate() const override;

	virtual bool HasValidTrackingPosition() override;
	virtual bool EnumerateTrackedDevices(TArray<int32>& OutDevices, EXRTrackedDeviceType Type = EXRTrackedDeviceType::Any) override;
	
	//virtual bool IsTracking(int32 DeviceId) override;
	virtual bool GetCurrentPose(int32 DeviceId, FQuat& OutOrientation, FVector& OutPosition) override;
#if UE_VERSION_OLDER_THAN(5, 0, 0)
	virtual bool GetRelativeEyePose(int32 DeviceId, EStereoscopicPass Eye, FQuat& OutOrientation, FVector& OutPosition) override;
#else
	virtual bool GetRelativeEyePose(int32 DeviceId, int32 ViewIndex, FQuat& OutOrientation, FVector& OutPosition) override;
#endif

	virtual void SetTrackingOrigin(EHMDTrackingOrigin::Type NewOrigin) override;
#if UE_VERSION_NEWER_THAN(4, 22, 3)
	virtual EHMDTrackingOrigin::Type GetTrackingOrigin() const override;
#endif

	virtual float GetWorldToMetersScale() const override; //This is currently going to make world scale the same for all players. See if this can be changed to be per player with a duplicate function
	virtual void ResetOrientationAndPosition(float Yaw = 0.f) override;

	virtual class TSharedPtr< class IXRCamera, ESPMode::ThreadSafe > GetXRCamera(int32 DeviceId = HMDDeviceId) override;
	virtual class IHeadMountedDisplay* GetHMDDevice() override;
	virtual class IHeadMountedDisplay* GetHMDDevice(int32 DeviceId);
	virtual class TSharedPtr<class IStereoRendering, ESPMode::ThreadSafe> GetStereoRenderingDevice() override;

	virtual bool OnStartGameFrame(FWorldContext& WorldContext) override;
	virtual void OnBeginRendering_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
	virtual void OnLateUpdateApplied_RenderThread(
#if ENGINE_MINOR_VERSION >= 27 || ENGINE_MAJOR_VERSION == 5
		FRHICommandListImmediate& RHICmdList,
#endif	  // ENGINE_MINOR_VERSION >= 27
		const FTransform& NewRelativeTransform) override;

	virtual void OnLateUpdateApplied_RenderThread(
#if ENGINE_MINOR_VERSION >= 27 || ENGINE_MAJOR_VERSION == 5
		FRHICommandListImmediate& RHICmdList,
#endif	  // ENGINE_MINOR_VERSION >= 27
		const FTransform& NewRelativeTransform,
		int32 DeviceId);

	// IStereoRendering Interface
	virtual bool IsStereoEnabled() const override;
	virtual bool EnableStereo(bool stereo = true) override;
#if UE_VERSION_OLDER_THAN(5, 0, 0)
	virtual void AdjustViewRect(enum EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override;
	virtual FVector2D GetTextSafeRegionBounds() const override;
	virtual FMatrix GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType) const override;
#else
	virtual void AdjustViewRect(int32 ViewIndex, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override;
	void AdjustViewRect(int32 ViewIndex, int32& X, int32& Y, uint32& SizeX, uint32& SizeY, int32 playerIndex) const;
	virtual FVector2D GetTextSafeRegionBounds() const override;
	virtual FMatrix GetStereoProjectionMatrix(const int32 ViewIndex) const override;
	virtual FMatrix GetStereoProjectionMatrix(const int32 ViewIndex, const int32 playerIndex) const;
#endif
	virtual void InitCanvasFromView(class FSceneView* InView, class UCanvas* Canvas) override;
	virtual void RenderTexture_RenderThread(class FRHICommandListImmediate& RHICmdList,
		class FRHITexture* BackBuffer,
		class FRHITexture* SrcTexture,
		FVector2D WindowSize) const override;
	virtual IStereoRenderTargetManager* GetRenderTargetManager() override;
	virtual void CalculateStereoViewOffset(const int32 ViewIndex, FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation) override;
	void CalculateStereoViewOffset(const int32 ViewIndex, FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation, int32 playerIndex);
	virtual FIntRect GetFullFlatEyeRect_RenderThread(FTexture2DRHIRef EyeTexture) const { return FIntRect(0, 0, 1, 1); }
	virtual FVector2D GetEyeCenterPoint_RenderThread(const int32 ViewIndex) const;

#if UE_VERSION_NEWER_THAN(4, 20, 3)
	virtual void CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList,
		FRHITexture2D* SrcTexture,
		FIntRect SrcRect,
		FRHITexture2D* DstTexture,
		FIntRect DstRect,
		bool bClearBlack,
		bool bNoAlpha) const;
#else
	virtual void CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList,
		FRHITexture2D* SrcTexture,
		FIntRect SrcRect,
		FRHITexture2D* DstTexture,
		FIntRect DstRect,
		bool bClearBlack) const;
#endif
	// /IStereoRendering Interface

	// IStereoLayer Interface
	virtual uint32 CreateLayer(const FLayerDesc& InLayerDesc) override;
	virtual void DestroyLayer(uint32 LayerId) override;
	virtual void SetLayerDesc(uint32 LayerId, const FLayerDesc& InLayerDesc) override;
	virtual bool GetLayerDesc(uint32 LayerId, FLayerDesc& OutLayerDesc) override;
	virtual void MarkTextureForUpdate(uint32 LayerId) override;
#if UE_VERSION_NEWER_THAN(4, 21, 2)
	virtual bool ShouldCopyDebugLayersToSpectatorScreen() const override;
#endif
	// /IStereoLayer Interface

	// IFXRRenderTargetManager Interface
	virtual bool ShouldUseSeparateRenderTarget() const override;
#if UE_VERSION_NEWER_THAN(4, 25, 4)
	virtual bool AllocateRenderTargetTexture(uint32 Index,
		uint32 SizeX,
		uint32 SizeY,
		uint8 Format,
		uint32 NumMips,
		ETextureCreateFlags Flags,
		ETextureCreateFlags TargetableTextureFlags,
		FTexture2DRHIRef& OutTargetableTexture,
		FTexture2DRHIRef& OutShaderResourceTexture,
		uint32 NumSamples = 1) override;
#endif
	// /IFXRRenderTargetManager Interface

	FTiltFiveWorldStatePtr CurrentWorldState;
	FTiltFiveWorldStatePtr RenderThreadWorldState;

	mutable bool bVersionCompatible = false;
	float LastGlassesUpdate = -1.0f;
	bool bEnableStereo = true;

	TArray<TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe>> GlassesList;

	TRefCountPtr<FTiltFiveCustomPresent> CustomPresent;

	static const FName TiltFiveSystemName;

	mutable int stereoPassCount = 0;
	mutable int32 currentSpectatedPlayer = 0;

	static const int32 GMaxNumTiltFiveGlasses = 4;

	//This stuff is for the Spectator Controller
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily);

	TUniquePtr<TiltFiveSpectatorController> SpectatorScreenController;

	TSharedPtr<class FTiltFiveSceneViewExtension, ESPMode::ThreadSafe> TiltFiveSceneViewExtension;

	class IRendererModule* RendererModule;

	protected:
		virtual class FXRRenderBridge* GetActiveRenderBridge_GameThread(bool bUseSeparateRenderTarget) override;

		friend class FTiltFiveCustomPresent;
};
