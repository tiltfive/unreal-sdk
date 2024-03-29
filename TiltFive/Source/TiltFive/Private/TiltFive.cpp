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

#include "TiltFive.h"

#include "Core.h"
#include "Engine.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "RHI.h"
#include "ThirdParty/TiltFiveLibrary/include/TiltFiveNative.h"

#if WITH_EDITOR
#include <MessageLogModule.h>
#include "PropertyEditorModule.h"
#endif
#include "TiltFiveXRBase.h"
#include "Logging/MessageLog.h"
#include "TiltFiveSettings.h"
#include "TiltFiveManager.h"
#include "TiltFiveManagerDetails.h"

#define LOCTEXT_NAMESPACE "FTiltFiveModule"

DEFINE_LOG_CATEGORY(LogTiltFive);

void FTiltFiveModule::StartupModule()
{
	IHeadMountedDisplayModule::StartupModule();

	if (IsRunningCommandlet())
	{
		return;
	}

	// Get the base directory of this plugin
	const FString BaseDir = IPluginManager::Get().FindPlugin("TiltFive")->GetBaseDir();

	// Add on the relative location of the third party dll and load it
	FString LibraryPath;
#if PLATFORM_WINDOWS
	LibraryPath = BaseDir / TEXT("Binaries/ThirdParty/TiltFiveLibrary/Win64/TiltFiveNative.dll");
#endif

#if WITH_EDITOR
	// create a message log for the asset tools to use
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	FMessageLogInitializationOptions InitOptions;
	InitOptions.bShowFilters = true;
	MessageLogModule.RegisterLogListing("TiltFive", LOCTEXT("TiltFiveMessageLog", "Tilt Five"), InitOptions);
#endif

	TiltFiveDLLHandle = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;

	if (TiltFiveDLLHandle)
	{
		const UGeneralProjectSettings* Settings = GetDefault<UGeneralProjectSettings>();
		const UTiltFiveSettings* TiltFiveSettings = GetDefault<UTiltFiveSettings>();
		const FString ApplicationId =
			TiltFiveSettings->ApplicationId.IsEmpty() ? FApp::GetProjectName() : TiltFiveSettings->ApplicationId;
		const FString ApplicationDisplayName =
			TiltFiveSettings->ApplicationDisplayName.IsEmpty() ? FApp::GetProjectName() : TiltFiveSettings->ApplicationDisplayName;
		const FString ApplicationVersion =
			TiltFiveSettings->ApplicationVersion.IsEmpty() ? Settings->ProjectVersion : TiltFiveSettings->ApplicationVersion;

		// Static to keep the memory around
		static char ApplicationIdBuffer[T5_MAX_STRING_PARAM_LEN + 1];
		static char ApplicationVersionBuffer[T5_MAX_STRING_PARAM_LEN + 1];

		FCStringAnsi::Strncpy(ApplicationIdBuffer, TCHAR_TO_ANSI(*ApplicationId), sizeof(ApplicationIdBuffer));
		FCStringAnsi::Strncpy(ApplicationVersionBuffer, TCHAR_TO_ANSI(*ApplicationVersion), sizeof(ApplicationVersionBuffer));
		FCStringAnsi::Strncpy(
			ApplicationDisplayNameBuffer, TCHAR_TO_ANSI(*ApplicationDisplayName), sizeof(ApplicationDisplayNameBuffer));

		ClientInfo.applicationId = ApplicationIdBuffer;
		ClientInfo.applicationVersion = ApplicationVersionBuffer;
		ClientInfo.sdkType = sdkType;

		void* PlatformContext = nullptr;
		const FT5Result Result = t5CreateContext(&TiltFiveContext, &ClientInfo, PlatformContext);

		if (Result != T5_SUCCESS)
		{
			UE_LOG(LogTiltFive, Error, TEXT("Failed to create context: %S"), t5GetResultMessage(Result));
			DisablePlugin();
			return;
		}
	}
	else
	{
		FMessageLog TiltFiveLog("TiltFive");
		TiltFiveLog.Error(LOCTEXT("TiltFiveLibraryNotFound", "Failed to load Tilt Five library."));
		TiltFiveLog.Open(EMessageSeverity::Error);
	}
#if WITH_EDITOR
	FPropertyEditorModule& PropertyEdModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEdModule.RegisterCustomClassLayout(
		ATiltFiveManager::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&TiltFiveManagerDetails::MakeInstance)
	);
#endif
}

void FTiltFiveModule::ShutdownModule()
{
#if WITH_EDITOR
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	MessageLogModule.UnregisterLogListing("TiltFive");
#endif
	DisablePlugin();

	IHeadMountedDisplayModule::ShutdownModule();
}

void FTiltFiveModule::DisablePlugin()
{
	if (tiltFiveXRBase.IsValid())
	{
		tiltFiveXRBase.Reset();
	}
	if (TiltFiveDLLHandle)
	{
		t5DestroyContext(&TiltFiveContext);
		// Free the dll handle
		FPlatformProcess::FreeDllHandle(TiltFiveDLLHandle);
		TiltFiveDLLHandle = nullptr;
	}
}

FString FTiltFiveModule::GetModuleKeyName() const
{
	return TEXT("TiltFiveHMD");
}

TSharedPtr<class IXRTrackingSystem, ESPMode::ThreadSafe> FTiltFiveModule::CreateTrackingSystem()
{
	if (!IsValid())
	{
		return nullptr;
	}
	tiltFiveXRBase = MakeShared< FTiltFiveXRBase, ESPMode::ThreadSafe>(nullptr);
	tiltFiveXRBase->Startup();
	return tiltFiveXRBase;
}

bool FTiltFiveModule::IsValid() const
{
	return !!TiltFiveDLLHandle;
}

TSharedPtr<class FTiltFiveXRBase, ESPMode::ThreadSafe> FTiltFiveModule::GetHMD() const
{
	return tiltFiveXRBase;
}

const char* FTiltFiveModule::GetApplicationDisplayName() const
{
	return ApplicationDisplayNameBuffer;
}

const T5_ClientInfo& FTiltFiveModule::GetClientInfo() const
{
	return ClientInfo;
}

FT5ContextPtr FTiltFiveModule::GetContext() const
{
	return TiltFiveContext;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTiltFiveModule, TiltFive)
