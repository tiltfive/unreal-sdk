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

#include "TiltFiveInputDevice.h"

#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "HMD/TiltFiveHMD.h"
#include "Misc/CoreDelegates.h"
#if UE_VERSION_NEWER_THAN(5, 1, 0)
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#endif
#include "TiltFive.h"
#include "TiltFiveXRBase.h"
#include "TiltFiveInput.h"
#include "TiltFiveKeys.h"
#include "TiltFiveTypes.h"

#ifndef UE_ARRAY_COUNT
#define UE_ARRAY_COUNT ARRAY_COUNT
#endif


FTiltFiveInputDevice::FTiltFiveInputDevice(const TSharedPtr<FTiltFiveXRBase, ESPMode::ThreadSafe>& InHMD,
	const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) : HMD(InHMD)
	, MessageHandler(InMessageHandler)
{
	IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);

	FMemory::Memzero(WandStates, UE_ARRAY_COUNT(WandStates));
}

FTiltFiveInputDevice::~FTiltFiveInputDevice()
{
	IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
}


void FTiltFiveInputDevice::Tick(float DeltaTime)
{
}

const int32 WandButtonOffsetT5 = 0;
const int32 WandButtonOffsetOne = 1;
const int32 WandButtonOffsetTwo = 2;
const int32 WandButtonOffsetThree = 3;
const int32 WandButtonOffsetY = 4;
const int32 WandButtonOffsetB = 5;
const int32 WandButtonOffsetA = 6;
const int32 WandButtonOffsetX = 7;

static const uint32 WandMasks[] = {1 << WandButtonOffsetT5,
	1 << WandButtonOffsetOne,
	1 << WandButtonOffsetTwo,
	1 << WandButtonOffsetThree,
	1 << WandButtonOffsetY,
	1 << WandButtonOffsetB,
	1 << WandButtonOffsetA,
	1 << WandButtonOffsetX};

void FTiltFiveInputDevice::SendControllerEvents()
{
	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> Hmd : HMD->GlassesList) {
		if (!FTiltFiveModule::Get().IsValid())
		{
			continue;
		}

		if (Hmd.IsValid())
		{
			if (!Hmd->IsHMDEnabled())
			{
				continue;
			}
		}

		FT5GlassesPtr CurrentGlasses = Hmd->GetCurrentExclusiveGlasses();

		// We did not establish a pair of glasses yet, so we cannot fetch input
		if (!CurrentGlasses)
		{
			continue;
		}

		// Note(marvin@lab132.com): Do we need to un-configure the old glasses in case they change
		// during runtime ?
		if (!ConfiguredInputGlasses || CurrentGlasses != ConfiguredInputGlasses)
		{
			FScopeLock ScopeLock(&Hmd->ExclusiveGroup1CriticalSection);

			FT5WandStreamConfig WandStreamConfig;
			WandStreamConfig.enabled = true;
			const FT5Result Result = t5ConfigureWandStreamForGlasses(CurrentGlasses, &WandStreamConfig);
			UE_CLOG(Result != T5_SUCCESS,
				LogTiltFiveInput,
				Error,
				TEXT("Failed to configure wand stream for TiltFiveWand: %S"),
				t5GetResultMessage(Result));
			if (Result == T5_SUCCESS)
			{
				ConfiguredInputGlasses = CurrentGlasses;
			}
			else
			{
				ConfiguredInputGlasses = nullptr;
			}
		}

		if (!ConfiguredInputGlasses)
		{
			return;
		}

		FTiltFiveControllerState OldWandStates[T5_MAX_NUM_CONTROLLER];
		for (uint32 WandIndex = 0; WandIndex < T5_MAX_NUM_CONTROLLER; ++WandIndex)
		{
			OldWandStates[WandIndex] = WandStates[Hmd->DeviceId][WandIndex];
		}

		{
			FScopeLock ScopeLock(&Hmd->ExclusiveGroup1CriticalSection);

			TArray<FT5WandHandle> ConnectedWands;
			ConnectedWands.SetNumUninitialized(T5_MAX_NUM_CONTROLLER);

			uint8 NumberOfWands = ConnectedWands.Num();

			FT5Result Result = t5ListWandsForGlasses(ConfiguredInputGlasses, ConnectedWands.GetData(), &NumberOfWands);
			// We currently (silently) drop the extra wands that may be connected, since we only support
			// up to T5_MAX_NUM_CONTROLLER wands anyway.
			UE_CLOG(Result != T5_SUCCESS && Result != T5_ERROR_OVERFLOW,
				LogTiltFiveInput,
				Warning,
				TEXT("Failed to list wands: %S"),
				t5GetResultMessage(Result));

			ConnectedWands.SetNumUnsafeInternal(NumberOfWands);

			for (uint32 WandIndex = 0; WandIndex < T5_MAX_NUM_CONTROLLER; ++WandIndex)
			{
				if (ConnectedWands.IsValidIndex(WandIndex))
				{
					WandStates[Hmd->DeviceId][WandIndex].WandHandle = ConnectedWands[WandIndex];
					WandStates[Hmd->DeviceId][WandIndex].bConnected = true;
				}
				else
				{
					WandStates[Hmd->DeviceId][WandIndex].WandHandle.Reset();
					WandStates[Hmd->DeviceId][WandIndex].bConnected = false;
				}
			}
		}

		// Currently, we only support the first user to have any controller, as we don't support
		// multiple glasses at the moment
		const int32 ControllerIndex = Hmd->DeviceId;
#if UE_VERSION_NEWER_THAN(5, 1, 0)
		const FInputDeviceId deviceId = FInputDeviceId::CreateFromInternalId(ControllerIndex);
#endif

		int32 NumEvent = 0;
		// Fail-safe to not get stuck in an infinite loop
		const int32 MaxNumEvents = 1000;

		FT5Result WandStreamResult = T5_SUCCESS;

		// First of gather all events happened since last frame.
		// This is necessary as the UPlayerInput does not seem to like
		// multiple analog axis value changes per frame and gets confused
		do
		{
			int32 WandIndex = INDEX_NONE;
			FT5WandStreamEvent StreamEvent;

			{
				FScopeLock ScopeLock(&HMD->GlassesList[0]->ExclusiveGroup2CriticalSection);

				// We don't want to spend any time waiting
				const uint32 TimeoutMs = 0;

				WandStreamResult = t5ReadWandStreamForGlasses(ConfiguredInputGlasses, &StreamEvent, TimeoutMs);

				UE_CLOG(WandStreamResult != T5_SUCCESS && WandStreamResult != T5_TIMEOUT,
					LogTiltFiveInput,
					Error,
					TEXT("Failed to retrieve wand stream event: %S"),
					t5GetResultMessage(WandStreamResult));
			}

			if (WandStreamResult != T5_SUCCESS)
			{
				break;
			}

			// TODO(marvin@lab132.com): What does this event supposed to mean?
			if (StreamEvent.type == kT5_WandStreamEventType_Desync)
			{
				continue;
			}

			for (int32 PotentialWandIndex = 0; PotentialWandIndex < T5_MAX_NUM_CONTROLLER; ++PotentialWandIndex)
			{
				if (WandStates[Hmd->DeviceId][PotentialWandIndex].WandHandle.IsSet() &&
					WandStates[Hmd->DeviceId][PotentialWandIndex].WandHandle == StreamEvent.wandId)
				{
					WandIndex = PotentialWandIndex;
					break;
				}
			}

			if (WandIndex == INDEX_NONE)
			{
				continue;
			}

			switch (StreamEvent.type)
			{
			case kT5_WandStreamEventType_Connect:
			{
				WandStates[Hmd->DeviceId][WandIndex].bConnected = true;
			}
			break;
			case kT5_WandStreamEventType_Disconnect:
			{
				WandStates[Hmd->DeviceId][WandIndex].bConnected = false;
			}
			break;
			case kT5_WandStreamEventType_Report:
			{
				FTiltFiveControllerState NewState;
				FMemory::Memzero(NewState);

				NewState.InitFromReport(StreamEvent.report);
				WandStates[Hmd->DeviceId][WandIndex].UpdateFromState(NewState);
			}
			break;
			default:
			{
				checkNoEntry();
			}
			break;
			}
		} while (++NumEvent < MaxNumEvents);

		UE_CLOG(NumEvent >= MaxNumEvents,
			LogTiltFiveInput,
			Warning,
			TEXT("Throttled wand stream input, read a maximum of %d events."),
			NumEvent);

		for (int32 WandIndex = 0; WandIndex < T5_MAX_NUM_CONTROLLER; ++WandIndex)
		{
			const bool bIsRightWand = WandIndex == 0;

			FTiltFiveControllerState& NewState = WandStates[Hmd->DeviceId][WandIndex];
			FTiltFiveControllerState& OldState = OldWandStates[WandIndex];

			if (NewState.bConnected != OldState.bConnected)
			{
				UE_CLOG(NewState.bConnected, LogTiltFiveInput, Verbose, TEXT("Wand connected with index %d"), WandIndex);
				UE_CLOG(!NewState.bConnected, LogTiltFiveInput, Verbose, TEXT("Wand disconnected with index %d"), WandIndex);
#if UE_VERSION_NEWER_THAN(5, 1, 0)
				((IPlatformInputDeviceMapper::Get()).GetOnInputDeviceConnectionChange())
					.Broadcast(NewState.bConnected ? EInputDeviceConnectionState::Connected : EInputDeviceConnectionState::Disconnected,
						FPlatformMisc::GetPlatformUserForUserIndex(Hmd->DeviceId),
						deviceId);
#else
				FCoreDelegates::OnControllerConnectionChange.Broadcast(NewState.bConnected, -1, ControllerIndex);
#endif
			}

			if (OldState.HasInputChanged(NewState))
			{
				static const FKey WandKeysLeft[] = { ETiltFiveKeys::WandL_T5,
					ETiltFiveKeys::WandL_One,
					ETiltFiveKeys::WandL_Two,
					ETiltFiveKeys::WandL_Three,
					ETiltFiveKeys::WandL_Y,
					ETiltFiveKeys::WandL_B,
					ETiltFiveKeys::WandL_A,
					ETiltFiveKeys::WandL_X };

				static const FKey WandKeysRight[] = { ETiltFiveKeys::WandR_T5,
					ETiltFiveKeys::WandR_One,
					ETiltFiveKeys::WandR_Two,
					ETiltFiveKeys::WandR_Three,
					ETiltFiveKeys::WandR_Y,
					ETiltFiveKeys::WandR_B,
					ETiltFiveKeys::WandR_A,
					ETiltFiveKeys::WandR_X };

				const FKey* ActiveWandKeys = bIsRightWand ? WandKeysRight : WandKeysLeft;

				const FKey& Wand_StickRight = bIsRightWand ? ETiltFiveKeys::WandR_StickRight : ETiltFiveKeys::WandL_StickRight;
				const FKey& Wand_StickLeft = bIsRightWand ? ETiltFiveKeys::WandR_StickLeft : ETiltFiveKeys::WandL_StickLeft;
				const FKey& Wand_StickUp = bIsRightWand ? ETiltFiveKeys::WandR_StickUp : ETiltFiveKeys::WandL_StickUp;
				const FKey& Wand_StickDown = bIsRightWand ? ETiltFiveKeys::WandR_StickDown : ETiltFiveKeys::WandL_StickDown;
				const FKey& Wand_StickX = bIsRightWand ? ETiltFiveKeys::WandR_StickX : ETiltFiveKeys::WandL_StickX;
				const FKey& Wand_StickY = bIsRightWand ? ETiltFiveKeys::WandR_StickY : ETiltFiveKeys::WandL_StickY;
				const FKey& Wand_Trigger = bIsRightWand ? ETiltFiveKeys::WandR_Trigger : ETiltFiveKeys::WandL_Trigger;
				const FKey& Wand_TriggerAxis = bIsRightWand ? ETiltFiveKeys::WandR_TriggerAxis : ETiltFiveKeys::WandL_TriggerAxis;

				if (NewState.Buttons.IsSet())
				{
					for (int32 ButtonIndex = 0; ButtonIndex < UE_ARRAY_COUNT(WandKeysLeft); ++ButtonIndex)
					{
						const bool bWasPressed = !!(OldState.Buttons.Get(0) & WandMasks[ButtonIndex]);
						const bool bIsPressed = !!(NewState.Buttons.GetValue() & WandMasks[ButtonIndex]);
						if (bWasPressed ^ bIsPressed)
						{
							if (bIsPressed)
							{
								UE_LOG(LogTiltFiveInput,
									Verbose,
									TEXT("%s Pressed Wand %d"),
									*ActiveWandKeys[ButtonIndex].ToString(),
									WandIndex);
#if UE_VERSION_NEWER_THAN(5, 1, 0)
								MessageHandler->OnControllerButtonPressed(ActiveWandKeys[ButtonIndex].GetFName(),
									FPlatformMisc::GetPlatformUserForUserIndex(Hmd->DeviceId),
									deviceId,
									false);
#else
								MessageHandler->OnControllerButtonPressed(
									ActiveWandKeys[ButtonIndex].GetFName(), ControllerIndex, false);
#endif
							}
							else
							{
								UE_LOG(LogTiltFiveInput,
									Verbose,
									TEXT("%s Released Wand %d"),
									*ActiveWandKeys[ButtonIndex].ToString(),
									WandIndex);
#if UE_VERSION_NEWER_THAN(5, 1, 0)
								MessageHandler->OnControllerButtonReleased(ActiveWandKeys[ButtonIndex].GetFName(),
									FPlatformMisc::GetPlatformUserForUserIndex(Hmd->DeviceId),
									deviceId,
									false);
#else
								MessageHandler->OnControllerButtonReleased(
									ActiveWandKeys[ButtonIndex].GetFName(), ControllerIndex, false);
#endif
							}
						}
					}
				}
				const float AxisEpsilon = 1e-3f;
				const float AxisDeadZone = 0.15f;
				const float AxisButtonTriggerThreshold = 0.3f;
				const float TriggerButtonTriggerThreshold = 0.5f;
				const float TriggerEpsilon = 1e-3f;
				const float TriggerDeadZone = 0.07f;

				if (NewState.Stick.IsSet())
				{
					SendAxisKeysEvent(OldState.Stick.Get(FVector2D::ZeroVector).X,
						NewState.Stick->X,
						true,
						Wand_StickRight,
						AxisButtonTriggerThreshold,
						ControllerIndex);
					SendAxisKeysEvent(OldState.Stick.Get(FVector2D::ZeroVector).X,
						NewState.Stick->X,
						false,
						Wand_StickLeft,
						AxisButtonTriggerThreshold,
						ControllerIndex);
					SendAxisKeysEvent(OldState.Stick.Get(FVector2D::ZeroVector).Y,
						NewState.Stick->Y,
						true,
						Wand_StickUp,
						AxisButtonTriggerThreshold,
						ControllerIndex);
					SendAxisKeysEvent(OldState.Stick.Get(FVector2D::ZeroVector).Y,
						NewState.Stick->Y,
						false,
						Wand_StickDown,
						AxisButtonTriggerThreshold,
						ControllerIndex);

					if (FMath::Abs(NewState.Stick->X) < AxisDeadZone)
					{
						NewState.Stick->X = 0.0f;
					}
					if (FMath::Abs(NewState.Stick->Y) < AxisDeadZone)
					{
						NewState.Stick->Y = 0.0f;
					}

					if (!FMath::IsNearlyEqual(OldState.Stick.Get(FVector2D::ZeroVector).X, NewState.Stick->X, AxisEpsilon) ||
						FMath::Abs(NewState.Stick->X) > 0.0f)
					{
						UE_LOG(LogTiltFiveInput,
							VeryVerbose,
							TEXT("%s Analog Value Changed: %s Wand %d"),
							*Wand_StickX.ToString(),
							*LexToSanitizedString(NewState.Stick->X),
							WandIndex);
#if UE_VERSION_NEWER_THAN(5, 1, 0)
						MessageHandler->OnControllerAnalog(
							Wand_StickX.GetFName(), FPlatformMisc::GetPlatformUserForUserIndex(Hmd->DeviceId), deviceId, NewState.Stick->X);
#else
						MessageHandler->OnControllerAnalog(Wand_StickX.GetFName(), ControllerIndex, NewState.Stick->X);
#endif
					}
					if (!FMath::IsNearlyEqual(OldState.Stick.Get(FVector2D::ZeroVector).Y, NewState.Stick->Y, AxisEpsilon) ||
						FMath::Abs(NewState.Stick->Y) > 0.0f)
					{
						UE_LOG(LogTiltFiveInput,
							VeryVerbose,
							TEXT("%s Analog Value Changed: %s Wand %d"),
							*Wand_StickY.ToString(),
							*LexToSanitizedString(NewState.Stick->Y),
							WandIndex);
#if UE_VERSION_NEWER_THAN(5, 1, 0)
						MessageHandler->OnControllerAnalog(
							Wand_StickY.GetFName(), FPlatformMisc::GetPlatformUserForUserIndex(Hmd->DeviceId), deviceId, NewState.Stick->Y);
#else
						MessageHandler->OnControllerAnalog(Wand_StickY.GetFName(), ControllerIndex, NewState.Stick->Y);
#endif
					}
				}
				if (NewState.Trigger.IsSet())
				{
					SendAxisKeysEvent(OldState.Trigger.Get(0.0f),
						NewState.Trigger.GetValue(),
						true,
						Wand_Trigger,
						TriggerButtonTriggerThreshold,
						ControllerIndex);

					if (FMath::Abs(NewState.Trigger.GetValue()) < TriggerDeadZone)
					{
						NewState.Trigger = 0.0f;
					}

					if (OldState.Trigger.IsSet()) 
					{
						if (!FMath::IsNearlyEqual(OldState.Trigger.Get(0.0f), NewState.Trigger.GetValue(), TriggerEpsilon) ||
							FMath::Abs(NewState.Trigger.GetValue()) > 0.0f)
						{
							UE_LOG(LogTiltFiveInput,
								VeryVerbose,
								TEXT("%s Analog Value Changed: %s Wand %d"),
								*Wand_TriggerAxis.ToString(),
								*LexToSanitizedString(NewState.Trigger.GetValue()),
								WandIndex);
#if UE_VERSION_NEWER_THAN(5, 1, 0)
							MessageHandler->OnControllerAnalog(Wand_TriggerAxis.GetFName(),
								FPlatformMisc::GetPlatformUserForUserIndex(Hmd->DeviceId),
								deviceId,
								NewState.Trigger.GetValue());
#else
							MessageHandler->OnControllerAnalog(Wand_TriggerAxis.GetFName(), ControllerIndex, NewState.Trigger.GetValue());
#endif
						}
					}
				}
			}
		}
	}
}

void FTiltFiveInputDevice::SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
{
	MessageHandler = InMessageHandler;
}

bool FTiltFiveInputDevice::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	return false;
}

void FTiltFiveInputDevice::SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value)
{
}

void FTiltFiveInputDevice::SetChannelValues(int32 ControllerId, const FForceFeedbackValues& values)
{
}

ETrackingStatus FTiltFiveInputDevice::GetControllerTrackingStatus(const int32 ControllerIndex,
	const FName MotionSource
) const
{
	if (MotionSource == FName("Right") || MotionSource == FName("Left"))
	{
		if (FTiltFiveModule::Get().IsValid())
		{
			// TODO(marvin@lab132.com): Can we detect if the wand is currently actively tracked (not
			// just available, but actively seen by the glasses?)
			const int32 WandIndex = MotionSource == FName("Right") ? 0 : 1;
			const bool bWandAvailable = WandStates[ControllerIndex][WandIndex].WandHandle.IsSet() && WandStates[ControllerIndex][WandIndex].bConnected;
			const bool bPoseAvailable = WandStates[ControllerIndex][WandIndex].WandPose.IsSet();
			// NOTE(marvin@lab132.com): Not entirely sure where to use InertialOnly and Tracked
			// status, from other usages it seems like InertialOnly is used, when the device in in
			// principle tracked but currently not visible to any trackers or so
			return bWandAvailable ? (bPoseAvailable ? ETrackingStatus::Tracked : ETrackingStatus::InertialOnly)
				: ETrackingStatus::NotTracked;
		}
	}
	return ETrackingStatus::NotTracked;
}

bool FTiltFiveInputDevice::GetControllerOrientationAndPosition(const int32 ControllerIndex,
	const FName MotionSource,
	FRotator& OutOrientation,
	FVector& OutPosition,
	float WorldToMetersScale) const
{
	if (MotionSource == FName("Right") || MotionSource == FName("Left"))
	{
		if (FTiltFiveModule::Get().IsValid())
		{
			bool bWandAvailable = false;
			const int32 WandIndex = MotionSource == FName("Right") ? 0 : 1;

			// TODO(marvin@lab132.com): We should try to fetch the latest position from the wand
			// stream here to reduce the wand latency However, due to the wand stream setup, we
			// would also need to cache and handle all other button events and send them later on on
			// the game thread
			if (WandStates[ControllerIndex][WandIndex].bConnected && WandStates[ControllerIndex][WandIndex].WandPose)
			{
				const FTiltFiveWandPose& WandPose = WandStates[ControllerIndex][WandIndex].WandPose.GetValue();

				const FQuat WandOrientation_WorldSpace = WandPose.Rotation * FQuat(FVector::RightVector, PI);
				const FVector WandPosition_WorldSpace = WandPose.Position * WorldToMetersScale;
				if (WandOrientation_WorldSpace.ContainsNaN() || WandPosition_WorldSpace.ContainsNaN())
				{
					UE_LOG(LogTiltFiveInput, VeryVerbose, TEXT("Got NaN in a Wand Position!"));

					return false;
				}
				OutPosition = WandPosition_WorldSpace;
				OutOrientation = WandOrientation_WorldSpace.Rotator();
				return true;
			}
		}
		else
		{
			UE_LOG(LogTiltFiveInput, VeryVerbose, TEXT("Tilt Five Module not available"));
		}
		return false;
	}
	return false;
}

#if UE_VERSION_OLDER_THAN(5, 3, 0)
ETrackingStatus FTiltFiveInputDevice::GetControllerTrackingStatus(const int32 ControllerIndex,

	const EControllerHand DeviceHand
) const
{
	if (DeviceHand == EControllerHand::Right || DeviceHand == EControllerHand::Left)
	{
		if (FTiltFiveModule::Get().IsValid())
		{
			const int32 WandIndex = DeviceHand == EControllerHand::Right ? 0 : 1;
			const bool bWandAvailable = WandStates[WandIndex].WandHandle.IsSet() && WandStates[WandIndex].bConnected;
			const bool bPoseAvailable = WandStates[WandIndex].WandPose.IsSet();
			return bWandAvailable ? (bPoseAvailable ? ETrackingStatus::Tracked : ETrackingStatus::InertialOnly)
				: ETrackingStatus::NotTracked;
		}
	}
	return ETrackingStatus::NotTracked;
}

bool FTiltFiveInputDevice::GetControllerOrientationAndPosition(const int32 ControllerIndex,
	const EControllerHand DeviceHand,
	FRotator& OutOrientation,
	FVector& OutPosition,
	float WorldToMetersScale) const
{
	if (ControllerIndex != 0)
	{
		return false;
	}
	if (DeviceHand == EControllerHand::Right || DeviceHand == EControllerHand::Left)
	{
		if (FTiltFiveModule::Get().IsValid())
		{
			bool bWandAvailable = false;
			const int32 WandIndex = DeviceHand == EControllerHand::Right ? 0 : 1;

			if (WandStates[WandIndex].bConnected && WandStates[WandIndex].WandPose)
			{
				const FTiltFiveWandPose& WandPose = WandStates[WandIndex].WandPose.GetValue();

				const FQuat WandOrientation_WorldSpace = WandPose.Rotation * FQuat(FVector::RightVector, PI);
				const FVector WandPosition_WorldSpace = WandPose.Position * WorldToMetersScale;
				if (WandOrientation_WorldSpace.ContainsNaN() || WandPosition_WorldSpace.ContainsNaN())
				{
					UE_LOG(LogTiltFiveInput, VeryVerbose, TEXT("Got NaN in a Wand Position!"));

					return false;
				}
				OutPosition = WandPosition_WorldSpace;
				OutOrientation = WandOrientation_WorldSpace.Rotator();
				return true;
			}
		}
		else
		{
			UE_LOG(LogTiltFiveInput, VeryVerbose, TEXT("Tilt Five Module not available"));
		}
		return false;
	}
	return false;
}
#endif

#if UE_VERSION_NEWER_THAN(5, 2, 0)
bool FTiltFiveInputDevice::GetControllerOrientationAndPosition(const int32 ControllerIndex, const FName MotionSource, FRotator& OutOrientation, FVector& OutPosition, bool& OutbProvidedLinearVelocity, FVector& OutLinearVelocity, bool& OutbProvidedAngularVelocity, FVector& OutAngularVelocityAsAxisAndLength, bool& OutbProvidedLinearAcceleration, FVector& OutLinearAcceleration, float WorldToMetersScale) const {
	return GetControllerOrientationAndPosition(ControllerIndex, MotionSource, OutOrientation, OutPosition, WorldToMetersScale);
}
bool FTiltFiveInputDevice::GetControllerOrientationAndPositionForTime(const int32 ControllerIndex, const FName MotionSource, FTimespan Time, bool& OutTimeWasUsed, FRotator& OutOrientation, FVector& OutPosition, bool& OutbProvidedLinearVelocity, FVector& OutLinearVelocity, bool& OutbProvidedAngularVelocity, FVector& OutAngularVelocityAsAxisAndLength, bool& OutbProvidedLinearAcceleration, FVector& OutLinearAcceleration, float WorldToMetersScale) const {
	return GetControllerOrientationAndPosition(ControllerIndex, MotionSource, OutOrientation, OutPosition, WorldToMetersScale);
}

void FTiltFiveInputDevice::EnumerateSources(TArray<FMotionControllerSource>& SourcesOut) const {
	return;
}
#endif

const FName TiltFiveWandName("TiltFiveWand");

FName FTiltFiveInputDevice::GetMotionControllerDeviceTypeName() const
{
	return TiltFiveWandName;
}

void FTiltFiveInputDevice::SetHapticFeedbackValues(int32 ControllerId, int32 Hand, const FHapticFeedbackValues& Values)
{
	if (ControllerId < 4 && ControllerId >=0) {
		if (Hand < 2 && Hand >= 0) {
			if (HMD->GlassesList[ControllerId]->CurrentExclusiveGlasses) {
				if (WandStates[ControllerId][Hand].WandHandle.IsSet()) {
					t5SendImpulse(HMD->GlassesList[ControllerId]->CurrentExclusiveGlasses, WandStates[ControllerId][Hand].WandHandle.GetValue(), Values.Amplitude, Values.Frequency);
				}
			}
		}
	}
}

void FTiltFiveInputDevice::GetHapticFrequencyRange(float& MinFrequency, float& MaxFrequency) const
{
	MinFrequency = 0.0f;
	MaxFrequency = 0.320f;
}

float FTiltFiveInputDevice::GetHapticAmplitudeScale() const
{
	return 1.0f;
}

void FTiltFiveInputDevice::SendAxisKeysEvent(float OldValue,
	float NewValue,
	bool bPositiveAxis,
	const FKey& Key,
	float AxisButtonTriggerThreshold,
	int32 ControllerIndex) const
{
	if (!bPositiveAxis)
	{
		OldValue = -OldValue;
		NewValue = -NewValue;
	}

	const bool bWasPressed = OldValue >= AxisButtonTriggerThreshold;
	const bool bIsPressed = NewValue >= AxisButtonTriggerThreshold;

	if (bWasPressed != bIsPressed)
	{
		if (bIsPressed)
		{
			UE_LOG(LogTiltFiveInput, Verbose, TEXT("%s Pressed"), *Key.ToString());
#if UE_VERSION_NEWER_THAN(5, 1, 0)
			MessageHandler->OnControllerButtonPressed(Key.GetFName(),
				FPlatformMisc::GetPlatformUserForUserIndex(ControllerIndex),
				FInputDeviceId::CreateFromInternalId(ControllerIndex),
				false);
#else
			MessageHandler->OnControllerButtonPressed(Key.GetFName(), ControllerIndex, false);
#endif
		}
		else
		{
			UE_LOG(LogTiltFiveInput, Verbose, TEXT("%s Released"), *Key.ToString());
#if UE_VERSION_NEWER_THAN(5, 1, 0)
			MessageHandler->OnControllerButtonReleased(Key.GetFName(),
				FPlatformMisc::GetPlatformUserForUserIndex(ControllerIndex),
				FInputDeviceId::CreateFromInternalId(ControllerIndex),
				false);
#else
			MessageHandler->OnControllerButtonReleased(Key.GetFName(), ControllerIndex, false);
#endif
		}
	}
}

bool FTiltFiveControllerState::HasInputChanged(const FTiltFiveControllerState& Other) const
{
	return Buttons != Other.Buttons || Stick != Other.Stick || Trigger != Other.Trigger || WandPose != Other.WandPose;
}

void FTiltFiveControllerState::InitFromReport(const FT5WandReport& Report)
{
	// NOTE(marvin@lab132.com): Assuming this means all float values
	if (Report.analogValid)
	{
		Stick = FVector2D(Report.stick.x, Report.stick.y);
		Trigger = Report.trigger;
	}

	if (Report.buttonsValid)
	{
		int32 NewButtons = 0;
		NewButtons |= (Report.buttons.t5 ? 1 : 0) << WandButtonOffsetT5;
		NewButtons |= (Report.buttons.one ? 1 : 0) << WandButtonOffsetOne;
		NewButtons |= (Report.buttons.two ? 1 : 0) << WandButtonOffsetTwo;
		NewButtons |= (Report.buttons.three ? 1 : 0) << WandButtonOffsetThree;
		NewButtons |= (Report.buttons.y ? 1 : 0) << WandButtonOffsetY;
		NewButtons |= (Report.buttons.b ? 1 : 0) << WandButtonOffsetB;
		NewButtons |= (Report.buttons.a ? 1 : 0) << WandButtonOffsetA;
		NewButtons |= (Report.buttons.x ? 1 : 0) << WandButtonOffsetX;
		Buttons = NewButtons;
	}

	if (Report.poseValid)
	{
		// These are unrelated to any world, so do not scale them (yet)
		WandPose = {FTiltFiveHMD::ConvertPositionFromHardware(Report.posGrip_GBD, 1.0f),
			FTiltFiveHMD::ConvertRotationFromHardware(Report.rotToWND_GBD)};
	}
}

void FTiltFiveControllerState::UpdateFromState(const FTiltFiveControllerState& NewState)
{
	if (NewState.Buttons)
	{
		Buttons = NewState.Buttons;
	}
	if (NewState.Stick)
	{
		Stick = NewState.Stick;
	}
	if (NewState.Trigger)
	{
		Trigger = NewState.Trigger;
	}
	if (NewState.WandPose)
	{
		WandPose = NewState.WandPose;
	}
}
