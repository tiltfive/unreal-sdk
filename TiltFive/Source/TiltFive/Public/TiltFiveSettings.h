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

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "TiltFiveSettings.generated.h"

/**
 *
 */
UCLASS(Config = Game, DefaultConfig, DisplayName = "Tilt Five")
class TILTFIVE_API UTiltFiveSettings : public UDeveloperSettings
{
private:
	GENERATED_BODY()

public:
	UTiltFiveSettings();
	virtual FName GetCategoryName() const override;

	// ApplicationId for the TiltFiveAPI, if empty project name will be used
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Applicaaiton Info", meta = (ConfigRestartRequired = true))
	FString ApplicationId;

	// ApplicationVersion for the TiltFiveAPI, if empty project version will be used
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Applicaaiton Info", meta = (ConfigRestartRequired = true))
	FString ApplicationVersion;

	// ApplicationDisplayName for the TiltFiveAPI aka The display name shown in control panel, if
	// empty project name will be used
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Applicaaiton Info", meta = (ConfigRestartRequired = true))
	FString ApplicationDisplayName;
};
