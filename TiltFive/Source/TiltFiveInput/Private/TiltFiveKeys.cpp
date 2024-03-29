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

#include "TiltFiveKeys.h"

#include "Misc/EngineVersionComparison.h"

#define LOCTEXT_NAMESPACE "TiltFive"

const FKey ETiltFiveKeys::WandR_StickX("WandR_StickX");
const FKey ETiltFiveKeys::WandR_StickY("WandR_StickY");
const FKey ETiltFiveKeys::WandR_Stick2D("WandR_Stick2D");
const FKey ETiltFiveKeys::WandR_TriggerAxis("WandR_TriggerAxis");
const FKey ETiltFiveKeys::WandR_T5("WandR_T5");
const FKey ETiltFiveKeys::WandR_One("WandR_One");
const FKey ETiltFiveKeys::WandR_Two("WandR_Two");
const FKey ETiltFiveKeys::WandR_Three("WandR_Three");
const FKey ETiltFiveKeys::WandR_A("WandR_A");
const FKey ETiltFiveKeys::WandR_B("WandR_B");
const FKey ETiltFiveKeys::WandR_X("WandR_X");
const FKey ETiltFiveKeys::WandR_Y("WandR_Y");
const FKey ETiltFiveKeys::WandR_Trigger("WandR_Trigger");
const FKey ETiltFiveKeys::WandR_StickLeft("WandR_StickLeft");
const FKey ETiltFiveKeys::WandR_StickRight("WandR_StickRight");
const FKey ETiltFiveKeys::WandR_StickUp("WandR_StickUp");
const FKey ETiltFiveKeys::WandR_StickDown("WandR_StickDown");

const FKey ETiltFiveKeys::WandL_StickX("WandL_StickX");
const FKey ETiltFiveKeys::WandL_StickY("WandL_StickY");
const FKey ETiltFiveKeys::WandL_Stick2D("WandL_Stick2D");
const FKey ETiltFiveKeys::WandL_TriggerAxis("WandL_TriggerAxis");
const FKey ETiltFiveKeys::WandL_T5("WandL_T5");
const FKey ETiltFiveKeys::WandL_One("WandL_One");
const FKey ETiltFiveKeys::WandL_Two("WandL_Two");
const FKey ETiltFiveKeys::WandL_Three("WandL_Three");
const FKey ETiltFiveKeys::WandL_A("WandL_A");
const FKey ETiltFiveKeys::WandL_B("WandL_B");
const FKey ETiltFiveKeys::WandL_X("WandL_X");
const FKey ETiltFiveKeys::WandL_Y("WandL_Y");
const FKey ETiltFiveKeys::WandL_Trigger("WandL_Trigger");
const FKey ETiltFiveKeys::WandL_StickLeft("WandL_StickLeft");
const FKey ETiltFiveKeys::WandL_StickRight("WandL_StickRight");
const FKey ETiltFiveKeys::WandL_StickUp("WandL_StickUp");
const FKey ETiltFiveKeys::WandL_StickDown("WandL_StickDown");

void ETiltFiveKeys::Init()
{
	static bool bInitialized = false;
	if (bInitialized)
	{
		return;
	}
	bInitialized = true;

	EKeys::AddMenuCategoryDisplayInfo(
		"TiltFiveWand", LOCTEXT("TiltFiveWandInputCategory", "Tilt Five Wand"), TEXT("GraphEditor.PadEvent_16x"));

#if UE_VERSION_NEWER_THAN(4, 25, 4)
#define AXIS_1D FKeyDetails::Axis1D
#define AXIS_2D FKeyDetails::Axis2D
#else
#define AXIS_1D FKeyDetails::FloatAxis
#define AXIS_2D FKeyDetails::VectorAxis
#endif

	// Right Wand
	EKeys::AddKey(FKeyDetails(WandR_StickX,
		LOCTEXT("TiltFiveWandRInputStickXLong", "Tilt Five Wand (R) Stick X Axis"),
		LOCTEXT("TiltFiveWandInputStickXShort", "Stick X Axis"),
		FKeyDetails::GamepadKey | AXIS_1D,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandR_StickY,
		LOCTEXT("TiltFiveWandRInputStickYLong", "Tilt Five Wand (R) Stick Y Axis"),
		LOCTEXT("TiltFiveWandInputStickYShort", "Stick Y Axis"),
		FKeyDetails::GamepadKey | AXIS_1D,
		"TiltFiveWand"));
#if UE_VERSION_NEWER_THAN(4, 25, 4)
	EKeys::AddPairedKey(FKeyDetails(WandR_Stick2D,
							LOCTEXT("TiltFiveWandRInputStick2DLong", "Tilt Five Wand (R) Stick 2D Axis"),
							LOCTEXT("TiltFiveWandInputStick2DShort", "Stick 2D Axis"),
							FKeyDetails::GamepadKey | AXIS_2D,
							"TiltFiveWand"),
		WandR_StickX,
		WandR_StickY);
#endif

	EKeys::AddKey(FKeyDetails(WandR_TriggerAxis,
		LOCTEXT("TiltFiveWandInputWandR_TriggerAxisLong", "Tilt Five Wand (R) Trigger Axis"),
		LOCTEXT("TiltFiveWandInputWand_TriggerAxisShort", "Trigger Axis"),
		FKeyDetails::GamepadKey | AXIS_1D,
		"TiltFiveWand"));

	EKeys::AddKey(FKeyDetails(WandR_T5,
		LOCTEXT("TiltFiveWandInputWandR_T5Long", "Tilt Five Wand (R) T5"),
		LOCTEXT("TiltFiveWandInputWand_SystemShort", "T5"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandR_One,
		LOCTEXT("TiltFiveWandInputWandR_OneLong", "Tilt Five Wand (R) One"),
		LOCTEXT("TiltFiveWandInputWand_OneShort", "One"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandR_Two,
		LOCTEXT("TiltFiveWandInputWandR_TwoLong", "Tilt Five Wand (R) Two"),
		LOCTEXT("TiltFiveWandInputWand_TwoShort", "Two"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandR_Three,
		LOCTEXT("TiltFiveWandInputWandR_ThreeLong", "Tilt Five Wand (R) Three"),
		LOCTEXT("TiltFiveWandInputWand_ThreeShort", "Three"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandR_A,
		LOCTEXT("TiltFiveWandInputWandR_ALong", "Tilt Five Wand (R) A"),
		LOCTEXT("TiltFiveWandInputWand_AShort", "A"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandR_B,
		LOCTEXT("TiltFiveWandInputWandR_BLong", "Tilt Five Wand (R) B"),
		LOCTEXT("TiltFiveWandInputWand_BShort", "B"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandR_X,
		LOCTEXT("TiltFiveWandInputWandR_XLong", "Tilt Five Wand (R) X"),
		LOCTEXT("TiltFiveWandInputWand_XShort", "X"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandR_Y,
		LOCTEXT("TiltFiveWandInputWandR_YLong", "Tilt Five Wand (R) Y"),
		LOCTEXT("TiltFiveWandInputWand_YShort", "Y"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandR_Trigger,
		LOCTEXT("TiltFiveWandInputWandR_TriggerLong", "Tilt Five Wand (R) Trigger"),
		LOCTEXT("TiltFiveWandInputWand_TriggerShort", "Trigger"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandR_StickLeft,
		LOCTEXT("TiltFiveWandInputWandR_StickLeftLong", "Tilt Five Wand (R) Stick Left"),
		LOCTEXT("TiltFiveWandInputWand_StickLeftShort", "Left"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandR_StickRight,
		LOCTEXT("TiltFiveWandInputWandR_StickRightLong", "Tilt Five Wand (R) Stick Right"),
		LOCTEXT("TiltFiveWandInputWand_StickRightShort", "Right"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandR_StickUp,
		LOCTEXT("TiltFiveWandInputWandR_StickUpLong", "Tilt Five Wand (R) Stick Up"),
		LOCTEXT("TiltFiveWandInputWand_StickUpShort", "Up"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandR_StickDown,
		LOCTEXT("TiltFiveWandInputWandR_StickDownLong", "Tilt Five Wand (R) Stick Down"),
		LOCTEXT("TiltFiveWandInputWand_StickDownShort", "Down"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));

	// Left Wand
	EKeys::AddKey(FKeyDetails(WandL_StickX,
		LOCTEXT("TiltFiveWandLInputStickXLong", "Tilt Five Wand (L) Stick X Axis"),
		LOCTEXT("TiltFiveWandInputStickXShort", "Stick X Axis"),
		FKeyDetails::GamepadKey | AXIS_1D,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandL_StickY,
		LOCTEXT("TiltFiveWandLInputStickYLong", "Tilt Five Wand (L) Stick Y Axis"),
		LOCTEXT("TiltFiveWandInputStickYShort", "Stick Y Axis"),
		FKeyDetails::GamepadKey | AXIS_1D,
		"TiltFiveWand"));
#if UE_VERSION_NEWER_THAN(4, 25, 4)
	EKeys::AddPairedKey(FKeyDetails(WandL_Stick2D,
							LOCTEXT("TiltFiveWandLInputStick2DLong", "Tilt Five Wand (L) Stick 2D Axis"),
							LOCTEXT("TiltFiveWandInputStick2DShort", "Stick 2D Axis"),
							FKeyDetails::GamepadKey | AXIS_2D,
							"TiltFiveWand"),
		WandL_StickX,
		WandL_StickY);
#endif
	EKeys::AddKey(FKeyDetails(WandL_TriggerAxis,
		LOCTEXT("TiltFiveWandInputWandL_TriggerAxisLong", "Tilt Five Wand (L) Trigger Axis"),
		LOCTEXT("TiltFiveWandInputWand_TriggerAxisShort", "Trigger Axis"),
		FKeyDetails::GamepadKey | AXIS_1D,
		"TiltFiveWand"));

	EKeys::AddKey(FKeyDetails(WandL_T5,
		LOCTEXT("TiltFiveWandInputWandL_T5Long", "Tilt Five Wand (L) T5"),
		LOCTEXT("TiltFiveWandInputWand_T5Short", "T5"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandL_One,
		LOCTEXT("TiltFiveWandInputWandL_OneLong", "Tilt Five Wand (L) One"),
		LOCTEXT("TiltFiveWandInputWand_OneShort", "One"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandL_Two,
		LOCTEXT("TiltFiveWandInputWandL_TwoLong", "Tilt Five Wand (L) Two"),
		LOCTEXT("TiltFiveWandInputWand_TwoShort", "Two"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandL_Three,
		LOCTEXT("TiltFiveWandInputWandL_ThreeLong", "Tilt Five Wand (L) Three"),
		LOCTEXT("TiltFiveWandInputWand_ThreeShort", "Three"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandL_A,
		LOCTEXT("TiltFiveWandInputWandL_ALong", "Tilt Five Wand (L) A"),
		LOCTEXT("TiltFiveWandInputWand_AShort", "A"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandL_B,
		LOCTEXT("TiltFiveWandInputWandL_BLong", "Tilt Five Wand (L) B"),
		LOCTEXT("TiltFiveWandInputWand_BShort", "B"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandL_X,
		LOCTEXT("TiltFiveWandInputWandL_XLong", "Tilt Five Wand (L) X"),
		LOCTEXT("TiltFiveWandInputWand_XShort", "X"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandL_Y,
		LOCTEXT("TiltFiveWandInputWandL_YLong", "Tilt Five Wand (L) Y"),
		LOCTEXT("TiltFiveWandInputWand_YShort", "Y"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandL_Trigger,
		LOCTEXT("TiltFiveWandInputWandL_TriggerLong", "Tilt Five Wand (L) Trigger"),
		LOCTEXT("TiltFiveWandInputWand_TriggerShort", "Trigger"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandL_StickLeft,
		LOCTEXT("TiltFiveWandInputWandL_StickLeftLong", "Tilt Five Wand (L) Stick Left"),
		LOCTEXT("TiltFiveWandInputWand_StickLeftShort", "Left"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandL_StickRight,
		LOCTEXT("TiltFiveWandInputWandL_StickRightLong", "Tilt Five Wand (L) Stick Right"),
		LOCTEXT("TiltFiveWandInputWand_StickRightShort", "Right"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandL_StickUp,
		LOCTEXT("TiltFiveWandInputWandL_StickUpLong", "Tilt Five Wand (L) Stick Up"),
		LOCTEXT("TiltFiveWandInputWand_StickUpShort", "Up"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
	EKeys::AddKey(FKeyDetails(WandL_StickDown,
		LOCTEXT("TiltFiveWandInputWandL_StickDownLong", "Tilt Five Wand (L) Stick Down"),
		LOCTEXT("TiltFiveWandInputWand_StickDownShort", "Down"),
		FKeyDetails::GamepadKey,
		"TiltFiveWand"));
}

#undef LOCTEXT_NAMESPACE
