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

#include "TiltFiveInput.h"

#include "TiltFive.h"
#include "TiltFiveInputDevice.h"
#include "TiltFiveKeys.h"
#include "Misc/EngineVersionComparison.h"

#define LOCTEXT_NAMESPACE "TiltFiveInput"

DEFINE_LOG_CATEGORY(LogTiltFiveInput);

void FTiltFiveInputModule::StartupModule()
{
	IInputDeviceModule::StartupModule();

	ETiltFiveKeys::Init();
}

void FTiltFiveInputModule::ShutdownModule()
{
	IInputDeviceModule::ShutdownModule();
}

TSharedPtr<class IInputDevice> FTiltFiveInputModule::CreateInputDevice(
	const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
{
#if UE_VERSION_OLDER_THAN(5, 3, 0)
	return MakeShared<FTiltFiveInputDevice>(FTiltFiveModule::Get().GetHMD(), InMessageHandler);
#else
	TSharedPtr<FTiltFiveInputDevice> inputDevice (new FTiltFiveInputDevice(FTiltFiveModule::Get().GetHMD(), InMessageHandler));
	return inputDevice;
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTiltFiveInputModule, TiltFiveInput)
