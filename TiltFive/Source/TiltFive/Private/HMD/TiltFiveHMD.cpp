// Copyright 2022 Tilt Five, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "HMD/TiltFiveHMD.h"

//#include "DefaultSpectatorScreenController.h"

#ifndef UE_ARRAY_COUNT
#define UE_ARRAY_COUNT ARRAY_COUNT
#endif

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerController.h"
#include "Modules/ModuleManager.h"
#include "PipelineStateCache.h"
#include "RendererInterface.h"
#include "TiltFiveXRBase.h"
#include "XRThreadUtils.h"

#include <chrono>
#include <thread>

const FQuat rotToUGLS_GLS = FQuat(0, 0.7071068, 0, -0.7071068);
const FQuat rotToGLS_UGLS = FQuat(0, 0.7071068, 0, 0.7071068);

FTiltFiveHMD::FTiltFiveHMD(IXRTrackingSystem *inTrackingSystem, int32 inDeviceId):
	TrackingSystem(inTrackingSystem),
	DeviceId(inDeviceId)
{
	for (int32 EyeIndex = 0; EyeIndex < NumEyeRenderTargets; ++EyeIndex)
	{
		FMemory::Memzero(EyeInfos[EyeIndex]);
	}
	CachedGameboardType_GameThread = kT5_GameboardType_None;
	CachedGameboardType_RenderThread = kT5_GameboardType_None;
	controlledPawn = nullptr;
}

FTiltFiveHMD::~FTiltFiveHMD()
{
	for (int32 EyeIndex = 0; EyeIndex < NumEyeRenderTargets; ++EyeIndex)
	{
		EyeInfos[EyeIndex].BufferedRTRHI = nullptr;
		EyeInfos[EyeIndex].BufferedSRVRHI = nullptr;
	}
	t5DestroyGlasses(&CurrentExclusiveGlasses);
	CurrentExclusiveGlasses = nullptr;
	CurrentReservedGlasses = nullptr;
	controlledPawn = nullptr;
}

void FTiltFiveHMD::ConditionalInitializeGlasses()
{
	if (!CurrentExclusiveGlasses)
	{
		if (!CurrentReservedGlasses)
		{
			CurrentReservedGlasses = RetrieveGlasses();
		}

		if (CurrentReservedGlasses)
		{
			FScopeLock ScopeLock(&ExclusiveGroup1CriticalSection);
			const FTiltFiveModule& TiltFiveModule = FTiltFiveModule::Get();
			char Identifier[T5_MAX_STRING_PARAM_LEN];
			size_t IdentifierSize = UE_ARRAY_COUNT(Identifier);

			ensure(t5GetGlassesIdentifier(CurrentReservedGlasses, Identifier, &IdentifierSize) == T5_SUCCESS);

			ET5ConnectionState ConnectionState = kT5_ConnectionState_Disconnected;

			// Try claiming the glasses for usage
			//FT5Result Result = t5ReserveGlasses(CurrentReservedGlasses, TiltFiveModule.GetApplicationDisplayName());

			//if (Result == T5_SUCCESS)
			//{
			FT5Result Result = t5GetGlassesConnectionState(CurrentReservedGlasses, &ConnectionState);
				if (Result != T5_SUCCESS)
				{
					UE_LOG(LogTiltFive, Error, TEXT("Failed to get connection state: %S"), t5GetResultMessage(Result));
				}

				// If we had some old glasses, destroy them
				if (CurrentExclusiveGlasses)
				{
					t5ReleaseGlasses(CurrentExclusiveGlasses);
					t5DestroyGlasses(&CurrentExclusiveGlasses);
					CurrentExclusiveGlasses = nullptr;
				}

				CurrentExclusiveGlasses = CurrentReservedGlasses;
				CurrentReservedGlasses = nullptr;

				int32 RetryLimit = 10;
				for (;;)
				{
					Result = t5EnsureGlassesReady(CurrentExclusiveGlasses);
					if (Result == T5_SUCCESS)
					{
						UE_LOG(LogTiltFive, Log, TEXT("Glasses made exclusive: %S"), Identifier);
						break;
					}
					else if (RetryLimit <= 0)
					{
						UE_LOG(LogTiltFive,
							Error,
							TEXT("Failed to ensure glasses are ready and reached retry limit. "
								"Releasing glasses."));
						t5ReleaseGlasses(CurrentExclusiveGlasses);
						t5DestroyGlasses(&CurrentExclusiveGlasses);
						CurrentExclusiveGlasses = nullptr;
						return;
					}
					else if (Result == T5_ERROR_TRY_AGAIN)
					{
						UE_LOG(LogTiltFive, Log, TEXT("Glasses not ready, trying again: %S"), Identifier);
						RetryLimit--;
						// Sleep for 100ms to wait for glasses to wake.
						FPlatformProcess::SleepNoStats(0.1f);
						continue;
					}
					else
					{
						UE_LOG(LogTiltFive,
							Error,
							TEXT("Failed to ensure glasses are ready. Releasing glasses.: %S"),
							t5GetResultMessage(Result));
						t5ReleaseGlasses(CurrentExclusiveGlasses);
						t5DestroyGlasses(&CurrentExclusiveGlasses);
						CurrentExclusiveGlasses = nullptr;
						return;
					}
				}

				const ET5GlassesParam Parameter = kT5_ParamGlasses_Float_IPD;

				double IPDValue;

				Result = t5GetGlassesFloatParam(CurrentExclusiveGlasses, 0, Parameter, &IPDValue);

				if (Result == T5_SUCCESS)
				{
					bVersionCompatible = true;
					CachedTiltFiveIPD = static_cast<float>(IPDValue);
				}
				else if (Result == T5_ERROR_SERVICE_INCOMPATIBLE)
				{
					bVersionCompatible = false;
					UE_LOG(LogTiltFive, Error, TEXT("Version incompatible, needs service upgrade"));
				}
				else
				{
					UE_LOG(LogTiltFive, Error, TEXT("Failed to retrieve IPD Value: %S"), t5GetResultMessage(Result));
				}
			//}
			/*else if (ConnectionState == kT5_ConnectionState_ExclusiveReservation)
			{
				UE_LOG(LogTiltFive, Verbose, TEXT("Glasses reserved: %S"), Identifier);
			}
			else
			{
				UE_LOG(LogTiltFive,
					Error,
					TEXT("Failed to make glasses exclusive for glasses %S: %S"),
					Identifier,
					t5GetResultMessage(Result));

				// Destroy them and re-try from the beginning, potentially with other glasses
				t5DestroyGlasses(&CurrentReservedGlasses);
				CurrentExclusiveGlasses = nullptr;
			}*/
		}
	}
	else
	{
		FScopeLock ScopeLock(&ExclusiveGroup1CriticalSection);
		ET5ConnectionState ConnectionState = kT5_ConnectionState_Disconnected;
		const FT5Result Result = t5GetGlassesConnectionState(CurrentExclusiveGlasses, &ConnectionState);
		if (Result != T5_SUCCESS || ConnectionState != kT5_ConnectionState_ExclusiveConnection)
		{
			UE_LOG(LogTiltFive, Error, TEXT("Lost connection to glasses: %S"), t5GetResultMessage(Result));
			t5ReleaseGlasses(CurrentExclusiveGlasses);
			t5DestroyGlasses(&CurrentExclusiveGlasses);
			CurrentExclusiveGlasses = nullptr;
		}
	}
}

class TSharedPtr< class IXRCamera, ESPMode::ThreadSafe > FTiltFiveHMD::GetXRCamera() {
	return SharedThis(this);
}

class IHeadMountedDisplay* FTiltFiveHMD::GetHMDDevice()
{
	return this;
}

FName FTiltFiveHMD::GetHMDName() const {
	return TrackingSystem->GetSystemName();
}

FQuat FTiltFiveHMD::ConvertFromHardware(const FQuat& rotToGLS_GBD)
{
	// Swap X and Y Axes to transform axis of rotation to Tilt Five GBD from Unreal World Space, and negate W component to swap Tilt
	// Five's RHS to Unreal's LHS
	FQuat Result = FQuat(rotToGLS_GBD.Y, rotToGLS_GBD.X, rotToGLS_GBD.Z, -rotToGLS_GBD.W);
	// Rotate to Unreal camera orientation from Tilt Five camera orientation.
	FQuat Result2 = rotToUGLS_GLS * Result;
	// Negate W component because Unreal requires Rotation from Camera space to world space, whereas Tilt Five provides a rotation
	// that takes point in world space to camera space.
	return FQuat(Result2.X, Result2.Y, Result2.Z, -Result2.W);
}

FQuat FTiltFiveHMD::ConvertToHardware(const FQuat& rotToGLS_GBD)
{
	// Negate W component because Unreal provides Rotation from Camera space to world space, whereas Tilt Five expect a rotation
	// that takes point in world space to camera space.
	FQuat Result = FQuat(rotToGLS_GBD.X, rotToGLS_GBD.Y, rotToGLS_GBD.Z, -rotToGLS_GBD.W);
	// Rotate from Unreal camera orientation to Tilt Five camera orientation.
	FQuat Result2 = rotToGLS_UGLS * Result;
	// Swap X and Y Axes to transform axis of rotation from Tilt Five GBD to Unreal World Space, and negate W component to swap
	// Unreals LHS to Tilt Five's RHS
	return FQuat(Result2.Y, Result2.X, Result2.Z, -Result2.W);
}

FVector FTiltFiveHMD::ConvertFromHardware(const FVector& Location, float WorldToMetersScale)
{
	return FVector(Location.Y, Location.X, Location.Z) * WorldToMetersScale;
}

FVector FTiltFiveHMD::ConvertToHardware(const FVector& Location, float WorldToMetersScale)
{
	return FVector(Location.Y, Location.X, Location.Z) / WorldToMetersScale;
}

bool FTiltFiveHMD::IsHMDConnected()
{
	if (CurrentExclusiveGlasses != nullptr)
	{
		return true;
	}
	else {
		const FTiltFiveModule& TiltFiveModule = FTiltFiveModule::Get();
		FT5GlassesPtr NewGlasses = nullptr;
		TArray<char, TFixedAllocator<FTiltFiveXRBase::GMaxNumTiltFiveGlasses * T5_MAX_STRING_PARAM_LEN>> GlassesStringBuffer;
		SIZE_T BufferSize = FTiltFiveXRBase::GMaxNumTiltFiveGlasses * T5_MAX_STRING_PARAM_LEN;

		{
			FScopeLock ScopeLock(&ExclusiveGroup1CriticalSection);
			const FT5Result Result = t5ListGlasses(TiltFiveModule.GetContext(), GlassesStringBuffer.GetData(), &BufferSize);

			GlassesStringBuffer.SetNumUnsafeInternal(BufferSize);

			TArray<FString> Glasses;

			char* CurrentPosition = GlassesStringBuffer.GetData();
			bool bMoreGlasses = true;
			do
			{
				const int32 Length = FCStringAnsi::Strlen(CurrentPosition);

				if (Length > 0)
				{
					Glasses.Add(FString(CurrentPosition));
					CurrentPosition += Length + 1;
				}
				else
				{
					bMoreGlasses = false;
				}
			} while (bMoreGlasses);

			if (Result == T5_ERROR_SERVICE_INCOMPATIBLE)
			{
				bVersionCompatible = false;
				UE_LOG(LogTiltFive, Error, TEXT("Version incompatible, needs service upgrade"));
			}
			bVersionCompatible = true;
			return Result == T5_SUCCESS && BufferSize > 0 && Glasses.Num() > DeviceId;
		}
	}
}

bool FTiltFiveHMD::IsHMDEnabled() const
{
	return CurrentExclusiveGlasses != nullptr;
}

void FTiltFiveHMD::EnableHMD(bool bEnable /*= true*/)
{
	unimplemented();
}

bool FTiltFiveHMD::GetHMDMonitorInfo(MonitorInfo& OutMonitorInfo)
{
	OutMonitorInfo.MonitorName = FString("Tilt Five Window");
	OutMonitorInfo.MonitorId = 0;
	OutMonitorInfo.DesktopX = 0;
	OutMonitorInfo.DesktopY = 0;

	const FIntPoint RenderTargetSize = GetIdealRenderTargetSize();
	OutMonitorInfo.ResolutionX = RenderTargetSize.X;
	OutMonitorInfo.ResolutionY = RenderTargetSize.Y;
	OutMonitorInfo.WindowSizeX = RenderTargetSize.X;
	OutMonitorInfo.WindowSizeY = RenderTargetSize.Y;

	return true;
}

void FTiltFiveHMD::GetFieldOfView(float& InOutHFOVInDegrees, float& InOutVFOVInDegrees) const
{
	InOutHFOVInDegrees = FOV;
	InOutVFOVInDegrees = FOV;
}

void FTiltFiveHMD::SetInterpupillaryDistance(float NewInterpupillaryDistance)
{
	// Most XR plugins appear to ignore requests to set the IPD. Not supporting having this override the IPD that the user has set
	// in the Tilt Five Control Panel simplifies making the CachedTiltFiveIPD safe-ish to access from different threads. We should
	// really be snapping it once per frame on the Game thread and then plumbing it through to the Render thread each frame, but
	// stuffing it in an atomic should be good enough for now.
}

float FTiltFiveHMD::GetInterpupillaryDistance() const
{
	// The interpupillary distance this is supposed to return appears to be in physical meters rather than Unreal units. It's not
	// entirely clear that's the case, but SetInterpupillaryDistance is documented as "Accessors to modify the interpupillary
	// distance (meters)" and many plugins default to or are hard-coded to return 0.064.
	return CachedTiltFiveIPD;
}

bool FTiltFiveHMD::GetHMDDistortionEnabled(EShadingPath ShadingPath) const
{
	// No (software) distortion necessary for tilt five
	return false;
}

FIntPoint FTiltFiveHMD::GetIdealRenderTargetSize() const
{
	return FIntPoint(2432, 3072);
}

#if UE_VERSION_NEWER_THAN(4, 20, 3)
FIntPoint FTiltFiveHMD::GetIdealDebugCanvasRenderTargetSize() const
{
	FIntPoint Result = GetIdealRenderTargetSize();
	// Only one eye
	Result.X /= 2;
	Result.Y /= 4;
	return Result;
}
#endif

bool FTiltFiveHMD::IsChromaAbCorrectionEnabled() const
{
	return false;
}

void FTiltFiveHMD::UseImplicitHMDPosition(bool bInImplicitHMDPosition) {
	bUseImplicitHMDPosition = true;
}

bool FTiltFiveHMD::GetUseImplicitHMDPosition() const {
	return true;
}

void FTiltFiveHMD::OverrideFOV(float& InOutFOV) {
	FOV = InOutFOV;
}

void FTiltFiveHMD::SetupLateUpdate(const FTransform& ParentToWorld, USceneComponent* Component, bool bSkipLateUpdate) {
	LateUpdate.Setup(ParentToWorld, Component, bSkipLateUpdate);
}

float FTiltFiveHMD::GetWorldToMetersScale() const
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

void FTiltFiveHMD::ApplyHMDRotation(APlayerController* PC, FRotator& ViewRotation) {
	UE_LOG(LogTiltFive, VeryVerbose, TEXT("This is being called"));
	ViewRotation.Normalize();
	FQuat DeviceOrientation;
	FVector DevicePosition;
	if (TrackingSystem->GetCurrentPose(DeviceId, DeviceOrientation, DevicePosition))
	{
		const FRotator DeltaRot = ViewRotation - PC->GetControlRotation();
		DeltaControlRotation = (DeltaControlRotation + DeltaRot).GetNormalized();

		// Pitch from other sources is never good, because there is an absolute up and down that must be respected to avoid motion sickness.
		// Same with roll.
		DeltaControlRotation.Pitch = 0;
		DeltaControlRotation.Roll = 0;
		DeltaControlOrientation = DeltaControlRotation.Quaternion();

		ViewRotation = FRotator(DeltaControlOrientation * DeviceOrientation);
	}
}

bool FTiltFiveHMD::UpdatePlayerCamera(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	FQuat DeviceOrientation;
	FVector DevicePosition;
	if (!TrackingSystem->GetCurrentPose(DeviceId, DeviceOrientation, DevicePosition))
	{
		return false;
	}

	if (GEnableVREditorHacks && !bUseImplicitHMDPosition)
	{
		DeltaControlOrientation = CurrentOrientation;
		DeltaControlRotation = DeltaControlOrientation.Rotator();
	}

	CurrentPosition = DevicePosition;
	CurrentOrientation = DeviceOrientation;
	
	return true;
}

void FTiltFiveHMD::CalculateStereoCameraOffset(const int32 ViewIndex, FRotator& ViewRotation, FVector& ViewLocation)
{
	FQuat EyeOrientation;
	FVector EyeOffset;

	if (TrackingSystem->GetRelativeEyePose(DeviceId, ViewIndex, EyeOrientation, EyeOffset))
	{
		ViewLocation += ViewRotation.Quaternion().RotateVector(EyeOffset);
		ViewRotation = FRotator(ViewRotation.Quaternion() * EyeOrientation);
	}
	else if (ViewIndex == EStereoscopicEye::eSSE_MONOSCOPIC && TrackingSystem->GetHMDDevice())
	{
		float HFov, VFov;
		TrackingSystem->GetHMDDevice()->GetFieldOfView(HFov, VFov);
		if (HFov > 0.0f)
		{
			const float CenterOffset = GNearClippingPlane + (TrackingSystem->GetHMDDevice()->GetInterpupillaryDistance() / 2.0f) * (1.0f / FMath::Tan(FMath::DegreesToRadians(HFov)));
			ViewLocation += ViewRotation.Quaternion().RotateVector(FVector(CenterOffset, 0, 0));
		}
	}
	else
	{
		return;
	}

	if (!bUseImplicitHMDPosition)
	{
		FQuat DeviceOrientation; // Unused
		FVector DevicePosition;
		TrackingSystem->GetCurrentPose(DeviceId, DeviceOrientation, DevicePosition);
		ViewLocation += DeltaControlOrientation.RotateVector(DevicePosition);
	}
}

#if UE_VERSION_OLDER_THAN(4, 26, 0)
void FTiltFiveHMD::UpdateSplashScreen()
{
}
#endif

FT5GlassesPtr FTiltFiveHMD::GetCurrentExclusiveGlasses() const
{
	return CurrentExclusiveGlasses;
}

FT5GameboardType FTiltFiveHMD::GetCurrentGameboardType() const
{
	return CurrentGameboardType;
}

void FTiltFiveHMD::UpdateCachedGlassesPose_RenderThread()
{
	check(IsInRenderingThread());

	if (!RenderThreadWorldState.IsValid())
	{
		CachedGlassesPoseIsValid_RenderThread = false;
		return;
	}

	FScopeLock ScopeLock(&ExclusiveGroup1CriticalSection);

	FT5GlassesPose Pose;

	const FT5Result Result = t5GetGlassesPose(CurrentExclusiveGlasses, kT5_GlassesPoseUsage_GlassesPresentation, &Pose);

	if (Result == T5_SUCCESS)
	{
		CachedGlassesPoseIsValid_RenderThread = true;
		CachedGlassesOrientation_RenderThread = ConvertRotationFromHardware(Pose.rotToGLS_GBD);
		CachedGlassesPosition_RenderThread =
			ConvertPositionFromHardware(Pose.posGLS_GBD, RenderThreadWorldState->WorldToMetersScale);
		CachedGameboardType_RenderThread = Pose.gameboardType;
	}
	else
	{
		CachedGlassesPoseIsValid_RenderThread = false;
	}
}

void FTiltFiveHMD::UpdateCachedGlassesPose_GameThread()
{
	check(IsInGameThread());

	if (!CurrentWorldState.IsValid())
	{
		CachedGlassesPoseIsValid_GameThread = false;
		return;
	}

	FScopeLock ScopeLock(&ExclusiveGroup1CriticalSection);

	FT5GlassesPose Pose;

	const FT5Result Result = t5GetGlassesPose(CurrentExclusiveGlasses, kT5_GlassesPoseUsage_GlassesPresentation, &Pose);

	if (Result == T5_SUCCESS)
	{
		CachedGlassesPoseIsValid_GameThread = true;
		CachedGlassesOrientation_GameThread = ConvertRotationFromHardware(Pose.rotToGLS_GBD);
		CachedGlassesPosition_GameThread = ConvertPositionFromHardware(Pose.posGLS_GBD, CurrentWorldState->WorldToMetersScale);
		CachedGameboardType_GameThread = Pose.gameboardType;
	}
	else
	{
		CachedGlassesPoseIsValid_GameThread = false;
	}
}

FT5GlassesPtr FTiltFiveHMD::RetrieveGlasses() const
{
	FScopeLock ScopeLock(&ExclusiveGroup1CriticalSection);
	static int32 CurrentGlassesIndex = 0;
	const FTiltFiveModule& TiltFiveModule = FTiltFiveModule::Get();

	FT5GlassesPtr NewGlasses = nullptr;
	TArray<char, TFixedAllocator<FTiltFiveXRBase::GMaxNumTiltFiveGlasses * T5_MAX_STRING_PARAM_LEN>> GlassesStringBuffer;
	SIZE_T BufferSize = FTiltFiveXRBase::GMaxNumTiltFiveGlasses * T5_MAX_STRING_PARAM_LEN;

	FT5Result Result = t5ListGlasses(TiltFiveModule.GetContext(), GlassesStringBuffer.GetData(), &BufferSize);

	if (Result == T5_SUCCESS)
	{
		
		bVersionCompatible = true;
		// Now figure out the number of glasses of this string mess
		GlassesStringBuffer.SetNumUnsafeInternal(BufferSize);

		TArray<FString> Glasses;

		char* CurrentPosition = GlassesStringBuffer.GetData();
		bool bMoreGlasses = true;
		do
		{
			const int32 Length = FCStringAnsi::Strlen(CurrentPosition);

			if (Length > 0)
			{
				Glasses.Add(FString(CurrentPosition));
				CurrentPosition += Length + 1;
			}
			else
			{
				bMoreGlasses = false;
			}
		} while (bMoreGlasses);
		if (DeviceId < Glasses.Num())
		{
			Result = t5CreateGlasses(
				TiltFiveModule.GetContext(), TCHAR_TO_UTF8(*Glasses[CurrentGlassesIndex % Glasses.Num()]), &NewGlasses);

			if (Result == T5_SUCCESS)
			{
				bVersionCompatible = true;
				UE_LOG(LogTiltFive, Log, TEXT("Reserving glasses with ID: %s"), *Glasses[CurrentGlassesIndex % Glasses.Num()]);

				Result = t5ReserveGlasses(NewGlasses, TiltFiveModule.GetApplicationDisplayName());

				if (Result == T5_SUCCESS)
				{
					UE_LOG(LogTiltFive, Log, TEXT("Reserved glasses with ID: %s"), *Glasses[CurrentGlassesIndex % Glasses.Num()]);
				}
				else
				{
					//UE_LOG(LogTiltFive, Error, TEXT("Failed to reserve glasses %S: %S"), *Glasses[CurrentGlassesIndex % Glasses.Num()], t5GetResultMessage(Result));

					// Destroy them and re-try from the beginning, potentially with other glasses
					t5DestroyGlasses(&NewGlasses);
					NewGlasses = nullptr;
				}
			}
			else if (Result == T5_ERROR_SERVICE_INCOMPATIBLE)
			{
				bVersionCompatible = false;
				UE_LOG(LogTiltFive, Error, TEXT("Version incompatible, needs service upgrade"));
			}
		}
		else
		{
			UE_LOG(LogTiltFive, VeryVerbose, TEXT("No glasses connected..."));
		}
	}
	else if (Result == T5_ERROR_SERVICE_INCOMPATIBLE)
	{
		bVersionCompatible = false;
		UE_LOG(LogTiltFive, Error, TEXT("Version incompatible, needs service upgrade"));
	}
	else
	{
		// T5_ERROR_NO_SERVICE or T5_ERROR_IO_FAILURE apparently means it is starting up internally,
		// and we get it soon(tm)
		UE_CLOG(Result != T5_ERROR_NO_SERVICE && Result != T5_ERROR_IO_FAILURE,
			LogTiltFive,
			Error,
			TEXT("Failed to retrieve glasses: %S"),
			t5GetResultMessage(Result));
	}

	// Next time, try the next index
	++CurrentGlassesIndex;

	return NewGlasses;
}
