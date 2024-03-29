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

#include "GenericPlatform/IInputInterface.h"
#include "IHapticDevice.h"
#include "IInputDevice.h"
#include "InputCoreTypes.h"
#include "TiltFiveTypes.h"
#include "XRMotionControllerBase.h"
#include "Misc/EngineVersionComparison.h"

class FTiltFiveHMD;

struct FTiltFiveWandPose
{
	FVector Position;
	FQuat Rotation;

	bool operator==(const FTiltFiveWandPose& Other) const
	{
		return Position == Other.Position && Rotation == Other.Rotation;
	}
};

struct FTiltFiveControllerState
{
	bool bConnected;
	TOptional<uint32> Buttons;
	TOptional<FVector2D> Stick;
	TOptional<float> Trigger;
	// Wand pose in raw unreal coordinates, but unscaled relative to the world scale
	TOptional<FTiltFiveWandPose> WandPose;

	TOptional<FT5WandHandle> WandHandle;

	bool HasInputChanged(const FTiltFiveControllerState& Other) const;
	void InitFromReport(const FT5WandReport& Report);
	void UpdateFromState(const FTiltFiveControllerState& NewState);
};

#define T5_MAX_NUM_CONTROLLER (2)

/**
 * Implements the regular unreal input interface and the motion controller for the tilt five wand
 */
class FTiltFiveInputDevice : public IInputDevice, public FXRMotionControllerBase, public IHapticDevice
{
public:
	FTiltFiveInputDevice(const TSharedPtr<FTiltFiveXRBase, ESPMode::ThreadSafe>& HMD,
		const TSharedRef<FGenericApplicationMessageHandler>& MessageHandler);
	virtual ~FTiltFiveInputDevice();

	// IInputDevice
	virtual void Tick(float DeltaTime) override;
	virtual void SendControllerEvents() override;
	virtual void SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) override;
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	virtual void SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value) override;
	virtual void SetChannelValues(int32 ControllerId, const FForceFeedbackValues& values) override;
	// /IInputDevice

	// MotionController
	virtual ETrackingStatus GetControllerTrackingStatus(
		const int32 ControllerIndex,
		const FName MotionSource
	) const override;
	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex,
		const FName MotionSource,
		FRotator& OutOrientation,
		FVector& OutPosition,
		float WorldToMetersScale) const override;

#if UE_VERSION_OLDER_THAN(5, 3, 0)
	virtual ETrackingStatus GetControllerTrackingStatus(
		const int32 ControllerIndex,
		const EControllerHand DeviceHand
	) const override;
	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex,
		const EControllerHand DeviceHand,
		FRotator& OutOrientation,
		FVector& OutPosition,
		float WorldToMetersScale) const override;
#endif

#if UE_VERSION_NEWER_THAN(5, 2, 0)
	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const FName MotionSource, FRotator& OutOrientation, FVector& OutPosition, bool& OutbProvidedLinearVelocity, FVector& OutLinearVelocity, bool& OutbProvidedAngularVelocity, FVector& OutAngularVelocityAsAxisAndLength, bool& OutbProvidedLinearAcceleration, FVector& OutLinearAcceleration, float WorldToMetersScale) const override;
	virtual bool GetControllerOrientationAndPositionForTime(const int32 ControllerIndex, const FName MotionSource, FTimespan Time, bool& OutTimeWasUsed, FRotator& OutOrientation, FVector& OutPosition, bool& OutbProvidedLinearVelocity, FVector& OutLinearVelocity, bool& OutbProvidedAngularVelocity, FVector& OutAngularVelocityAsAxisAndLength, bool& OutbProvidedLinearAcceleration, FVector& OutLinearAcceleration, float WorldToMetersScale) const override;

	virtual void EnumerateSources(TArray<FMotionControllerSource>& SourcesOut) const override;
	virtual float GetCustomParameterValue(const FName MotionSource, FName ParameterName, bool& bValueFound) const override { bValueFound = false;  return 0.f; }
	virtual bool GetHandJointPosition(const FName MotionSource, int jointIndex, FVector& OutPosition) const override { return false; }
#endif
	virtual FName GetMotionControllerDeviceTypeName() const override;
	// /MotionController

	// IHapticDevice
	virtual void SetHapticFeedbackValues(int32 ControllerId, int32 Hand, const FHapticFeedbackValues& Values) override;
	virtual void GetHapticFrequencyRange(float& MinFrequency, float& MaxFrequency) const override;
	virtual float GetHapticAmplitudeScale() const override;
	// /IHapticDevice

private:
	TSharedPtr<FTiltFiveXRBase, ESPMode::ThreadSafe> HMD;
	TSharedRef<FGenericApplicationMessageHandler> MessageHandler;

	FTiltFiveControllerState WandStates[4][T5_MAX_NUM_CONTROLLER];
	void SendAxisKeysEvent(float OldValue,
		float NewValue,
		bool bPositiveAxis,
		const FKey& Key,
		float AxisButtonTriggerThreshold,
		int32 ControllerIndex) const;

	FT5GlassesPtr ConfiguredInputGlasses = nullptr;
};
;
