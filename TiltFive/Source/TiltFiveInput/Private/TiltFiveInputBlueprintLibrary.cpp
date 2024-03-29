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

#include "TiltFiveInputBlueprintLibrary.h"

#include "Framework/Application/SlateApplication.h"
#include "TiltFiveKeys.h"

FTiltFiveNavigationConfig::FTiltFiveNavigationConfig()
{
	KeyEventRules.Emplace(ETiltFiveKeys::WandL_StickLeft, EUINavigation::Left);
	KeyEventRules.Emplace(ETiltFiveKeys::WandR_StickLeft, EUINavigation::Left);

	KeyEventRules.Emplace(ETiltFiveKeys::WandL_StickRight, EUINavigation::Right);
	KeyEventRules.Emplace(ETiltFiveKeys::WandR_StickRight, EUINavigation::Right);

	KeyEventRules.Emplace(ETiltFiveKeys::WandL_StickUp, EUINavigation::Up);
	KeyEventRules.Emplace(ETiltFiveKeys::WandR_StickUp, EUINavigation::Up);

	KeyEventRules.Emplace(ETiltFiveKeys::WandL_StickDown, EUINavigation::Down);
	KeyEventRules.Emplace(ETiltFiveKeys::WandR_StickDown, EUINavigation::Down);

#if UE_VERSION_NEWER_THAN(4, 22, 99)
	NavigationActionRules.Emplace(ETiltFiveKeys::WandL_One, EUINavigationAction::Accept);
	NavigationActionRules.Emplace(ETiltFiveKeys::WandR_One, EUINavigationAction::Accept);

	NavigationActionRules.Emplace(ETiltFiveKeys::WandL_Two, EUINavigationAction::Back);
	NavigationActionRules.Emplace(ETiltFiveKeys::WandR_Two, EUINavigationAction::Back);

#endif
}

#if UE_VERSION_NEWER_THAN(4, 22, 99) && UE_VERSION_OLDER_THAN(4, 24, 0)
EUINavigationAction FTiltFiveNavigationConfig::GetNavigationActionForKey(const FKey& InKey) const
{
	if (const EUINavigationAction* Action = NavigationActionRules.Find(InKey))
	{
		return *Action;
	}
	return FNavigationConfig::GetNavigationActionForKey(InKey);
}
#elif UE_VERSION_NEWER_THAN(4, 23, 99)
EUINavigationAction FTiltFiveNavigationConfig::GetNavigationActionFromKey(const FKeyEvent& InKey) const
{
	if (const EUINavigationAction* Action = NavigationActionRules.Find(InKey.GetKey()))
	{
		return *Action;
	}
	return FNavigationConfig::GetNavigationActionFromKey(InKey);
}
#endif

bool UTiltFiveInputBlueprintLibrary::RegisterTiltFiveNavigation()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication& SlateApplication = FSlateApplication::Get();
		OriginalNavigation = SlateApplication.GetNavigationConfig();
		if (!TiltFiveNavigation.IsValid())
		{
			TiltFiveNavigation = MakeShared<FTiltFiveNavigationConfig>();
		}
		SlateApplication.SetNavigationConfig(TiltFiveNavigation.ToSharedRef());
		return true;
	}
	return false;
}

bool UTiltFiveInputBlueprintLibrary::UnregisterTiltFiveNavigation()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication& SlateApplication = FSlateApplication::Get();
		if (OriginalNavigation)
		{
			SlateApplication.SetNavigationConfig(OriginalNavigation.ToSharedRef());
			OriginalNavigation.Reset();
			TiltFiveNavigation.Reset();
			return true;
		}
	}
	return false;
}

TSharedPtr<FNavigationConfig> UTiltFiveInputBlueprintLibrary::OriginalNavigation;
TSharedPtr<FTiltFiveNavigationConfig> UTiltFiveInputBlueprintLibrary::TiltFiveNavigation;
