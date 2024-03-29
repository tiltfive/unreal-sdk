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
#include "InputCoreTypes.h"

struct TILTFIVEINPUT_API ETiltFiveKeys
{
	static const FKey WandR_StickX;
	static const FKey WandR_StickY;
	static const FKey WandR_Stick2D;

	static const FKey WandR_TriggerAxis;

	static const FKey WandR_Trigger;
	static const FKey WandR_T5;
	static const FKey WandR_One;
	static const FKey WandR_Two;
	static const FKey WandR_Three;
	static const FKey WandR_A;
	static const FKey WandR_B;
	static const FKey WandR_X;
	static const FKey WandR_Y;
	static const FKey WandR_StickLeft;
	static const FKey WandR_StickRight;
	static const FKey WandR_StickUp;
	static const FKey WandR_StickDown;

	static const FKey WandL_StickX;
	static const FKey WandL_StickY;
	static const FKey WandL_Stick2D;

	static const FKey WandL_TriggerAxis;

	static const FKey WandL_Trigger;
	static const FKey WandL_T5;
	static const FKey WandL_One;
	static const FKey WandL_Two;
	static const FKey WandL_Three;
	static const FKey WandL_A;
	static const FKey WandL_B;
	static const FKey WandL_X;
	static const FKey WandL_Y;
	static const FKey WandL_StickLeft;
	static const FKey WandL_StickRight;
	static const FKey WandL_StickUp;
	static const FKey WandL_StickDown;

	// Initializes the Tilt Five keys and adds them to the Unreal Input system
	static void Init();
};
