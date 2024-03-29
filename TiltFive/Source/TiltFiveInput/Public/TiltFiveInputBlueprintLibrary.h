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

#include "Framework/Application/NavigationConfig.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/EngineVersionComparison.h"

#include "TiltFiveInputBlueprintLibrary.generated.h"

class FTiltFiveNavigationConfig : public FNavigationConfig
{
public:
	FTiltFiveNavigationConfig();

#if UE_VERSION_NEWER_THAN(4, 22, 99) && UE_VERSION_OLDER_THAN(4, 24, 0)
	virtual EUINavigationAction GetNavigationActionForKey(const FKey& InKey) const override;
#elif UE_VERSION_NEWER_THAN(4, 23, 99)
	virtual EUINavigationAction GetNavigationActionFromKey(const FKeyEvent& InKey) const override;
#endif

#if UE_VERSION_NEWER_THAN(4, 22, 99)
private:
	TMap<FKey, EUINavigationAction> NavigationActionRules;
#endif
};

/**
 *
 */
UCLASS()
class TILTFIVEINPUT_API UTiltFiveInputBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "Tilt Five|Input")
	static bool RegisterTiltFiveNavigation();

	UFUNCTION(BlueprintCallable, Category = "Tilt Five|Input")
	static bool UnregisterTiltFiveNavigation();

private:
	static TSharedPtr<FNavigationConfig> OriginalNavigation;
	static TSharedPtr<FTiltFiveNavigationConfig> TiltFiveNavigation;
};
