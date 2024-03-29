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

#pragma once

#include "CoreMinimal.h"
#include "HeadMountedDisplayBase.h"
#include "IXRCamera.h"
#include "Misc/EngineVersionComparison.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SceneViewExtension.h"

#include "TiltFive.h"

#include <atomic>

constexpr int32 NumEyeRenderTargets = 2;

struct FTiltFiveEyeInfo
{
	FTexture2DRHIRef BufferedRTRHI;
	FTexture2DRHIRef BufferedSRVRHI;
	FIntRect SourceEyeRect;
	FIntRect DestEyeRect;
#if !UE_BUILD_SHIPPING
	FString DebugName;
#endif
};

struct FTiltFiveWorldState
{
	float WorldToMetersScale;
};

typedef TSharedPtr<FTiltFiveWorldState, ESPMode::ThreadSafe> FTiltFiveWorldStatePtr;

/**
 * Tilt Five Head Mounted Display Interface Implementation
 */
class TILTFIVE_API FTiltFiveHMD : public IHeadMountedDisplay,
								  public IXRCamera,
								  public TSharedFromThis<FTiltFiveHMD, ESPMode::ThreadSafe>
{
public:
	FTiltFiveHMD(IXRTrackingSystem *base, int32 deviceId);
	virtual ~FTiltFiveHMD() override;

	void ConditionalInitializeGlasses();

	virtual class IHeadMountedDisplay* GetHMDDevice();
	virtual FName GetHMDName() const override;

	virtual int32 GetSystemDeviceId() const override
	{
		return DeviceId;
	}

	static FQuat ConvertFromHardware(const FQuat& Rotation);
	static FQuat ConvertToHardware(const FQuat& Rotation);
	static FVector ConvertFromHardware(const FVector& Rotation, float WorldToMetersScale);
	static FVector ConvertToHardware(const FVector& Rotation, float WorldToMetersScale);

	// Someone thought anonymous structs were a good idea.
	template <typename T> static FQuat ConvertRotationFromHardware(const T& Rotation)
	{
		const FQuat HardwareRotation(Rotation.x, Rotation.y, Rotation.z, Rotation.w);
		return ConvertFromHardware(HardwareRotation);
	}

	template <typename T> static FVector ConvertPositionFromHardware(const T& Position, float WorldToMetersScale)
	{
		const FVector HardwarePosition(Position.x, Position.y, Position.z);
		return ConvertFromHardware(HardwarePosition, WorldToMetersScale);
	}

	template <typename T> static void ConvertRotationToHardware(const FQuat& SourceRotation, T& OutHardwareRotation)
	{
		const FQuat HardwareRotation(ConvertToHardware(SourceRotation));
		OutHardwareRotation.x = HardwareRotation.X;
		OutHardwareRotation.y = HardwareRotation.Y;
		OutHardwareRotation.z = HardwareRotation.Z;
		OutHardwareRotation.w = HardwareRotation.W;
	}

	template <typename T>
	static void ConvertPositionToHardware(const FVector& SourcePosition, T& OutHardwarePosition, float WorldToMetersScale)
	{
		const FVector HardwarePosition(ConvertToHardware(SourcePosition, WorldToMetersScale));
		OutHardwarePosition.x = HardwarePosition.X;
		OutHardwarePosition.y = HardwarePosition.Y;
		OutHardwarePosition.z = HardwarePosition.Z;
	}

	// IHeadMountedDisplay Interface
	virtual bool IsHMDConnected() override;
	virtual bool IsHMDEnabled() const override;
	virtual void EnableHMD(bool bEnable = true) override;
	virtual bool GetHMDMonitorInfo(MonitorInfo&) override;
	virtual void GetFieldOfView(float& InOutHFOVInDegrees, float& InOutVFOVInDegrees) const override;
	virtual void SetInterpupillaryDistance(float NewInterpupillaryDistance) override;
	virtual float GetInterpupillaryDistance() const override;
	virtual bool GetHMDDistortionEnabled(EShadingPath ShadingPath) const override;
	virtual FIntPoint GetIdealRenderTargetSize() const override;
#if UE_VERSION_NEWER_THAN(4, 20, 3)
	virtual FIntPoint GetIdealDebugCanvasRenderTargetSize() const override;
#endif
	virtual bool IsChromaAbCorrectionEnabled() const override;
	// /IHeadMountedDisplay interface

	virtual float GetWorldToMetersScale() const;

	// IXRCamera
	virtual FName GetSystemName() const override
	{
		return TrackingSystem->GetSystemName();
	}
	virtual void UseImplicitHMDPosition(bool bInImplicitHMDPosition) override;
	virtual bool GetUseImplicitHMDPosition() const override;
	virtual void OverrideFOV(float& InOutFOV) override;
	virtual void SetupLateUpdate(const FTransform& ParentToWorld, USceneComponent* Component, bool bSkipLateUpdate) override;
	virtual void ApplyHMDRotation(APlayerController* PC, FRotator& ViewRotation) override;
	virtual bool UpdatePlayerCamera(FQuat& CurrentOrientation, FVector& CurrentPosition) override;
	virtual void CalculateStereoCameraOffset(const int32 ViewIndex, FRotator& ViewRotation, FVector& ViewLocation) override;
	// /IXRCamera

	virtual class TSharedPtr<class IXRCamera, ESPMode::ThreadSafe> GetXRCamera();

	static const FName TiltFiveSystemName;
	IXRTrackingSystem* TrackingSystem;

	const int32 DeviceId;

	// Critical section for the exclusive groups inside the tilt five API
	mutable FCriticalSection ExclusiveGroup1CriticalSection;
	mutable FCriticalSection ExclusiveGroup2CriticalSection;

	FRotator DeltaControlRotation;
	FQuat DeltaControlOrientation;
	
	// Render thread cached glasses pose.
	bool CachedGlassesPoseIsValid_RenderThread = false;
	FQuat CachedGlassesOrientation_RenderThread;
	FVector CachedGlassesPosition_RenderThread;
	FT5GameboardType CachedGameboardType_RenderThread = FT5GameboardType::kT5_GameboardType_None;

	// Game thread cached glasses pose.
	bool CachedGlassesPoseIsValid_GameThread = false;
	FQuat CachedGlassesOrientation_GameThread;
	FVector CachedGlassesPosition_GameThread;
	FT5GameboardType CachedGameboardType_GameThread = FT5GameboardType::kT5_GameboardType_None;

	// The UWRLD pose of the glasses relative to its parent (i.e. the AR root) that is being used to render the current frame. This
	// gets set to an 'early' pose by OnStartGameFrame and may get updated by OnLateUpdateApplied_RenderThread if a late update is
	// done by the DefaultXRCamera. If no glasses pose was available at the time this gets set then it is in the "unset" state.
	TOptional<FTransform> MaybeRelativeGlassesTransform_RenderThread;
	std::atomic<float> CachedTiltFiveIPD = 0.064f;

	mutable bool bVersionCompatible;

#if UE_VERSION_OLDER_THAN(4, 26, 0)
	virtual void UpdateSplashScreen() override;
#endif

	FT5GlassesPtr GetCurrentExclusiveGlasses() const;
	FT5GameboardType GetCurrentGameboardType() const;

	void UpdateCachedGlassesPose_RenderThread();
	void UpdateCachedGlassesPose_GameThread();

	FTiltFiveEyeInfo EyeInfos[2];
	float FOV = 70.0f;
	float WidthToHeight = 1.0f;

	float GNearClippingPlane = 0.0f;

	APawn *controlledPawn;

	FT5GlassesPtr CurrentReservedGlasses = nullptr;
	FT5GlassesPtr CurrentExclusiveGlasses = nullptr;
	FT5GlassesPtr GraphicsInitializedGlasses = nullptr;

	FTiltFiveWorldStatePtr CurrentWorldState;
	FTiltFiveWorldStatePtr RenderThreadWorldState;

private:
	FLateUpdateManager LateUpdate;
	bool bUseImplicitHMDPosition;
	mutable bool bCurrentFrameIsStereoRendering;

	FT5GameboardType CurrentGameboardType = FT5GameboardType::kT5_GameboardType_None;

	FT5GlassesPtr RetrieveGlasses() const;
};
