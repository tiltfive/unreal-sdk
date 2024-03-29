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

#include "IHeadMountedDisplayModule.h"
#include "Modules/ModuleManager.h"
#include "ThirdParty/TiltFiveLibrary/include/TiltFiveNative.h"
#include "TiltFiveTypes.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTiltFive, Log, All);

class TILTFIVE_API FTiltFiveModule : public IHeadMountedDisplayModule
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual FString GetModuleKeyName() const override;
	virtual TSharedPtr<class IXRTrackingSystem, ESPMode::ThreadSafe> CreateTrackingSystem() override;

	bool IsValid() const;

	static FTiltFiveModule& Get()
	{
		return FModuleManager::Get().LoadModuleChecked<FTiltFiveModule>("TiltFive");
	}

	TSharedPtr<class FTiltFiveXRBase, ESPMode::ThreadSafe> GetHMD() const;
	const char* GetApplicationDisplayName() const;
	const FT5ClientInfo& GetClientInfo() const;
	FT5ContextPtr GetContext() const;

private:
	void DisablePlugin();

	void* TiltFiveDLLHandle = nullptr;
	FT5ContextPtr TiltFiveContext = nullptr;

	char ApplicationDisplayNameBuffer[T5_MAX_STRING_PARAM_LEN + 1]{};
	FT5ClientInfo ClientInfo{};
	const uint8_t sdkType = 0b00100000;	   // Unreal SDK type
	TSharedPtr<class FTiltFiveXRBase, ESPMode::ThreadSafe> tiltFiveXRBase;
};
