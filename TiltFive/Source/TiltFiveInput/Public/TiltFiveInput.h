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
#include "IInputDeviceModule.h"
#include "Modules/ModuleManager.h"
#include "XRMotionControllerBase.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTiltFiveInput, Log, All);

class TILTFIVEINPUT_API FTiltFiveInputModule : public IInputDeviceModule
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual TSharedPtr<class IInputDevice> CreateInputDevice(
		const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) override;

private:
};
