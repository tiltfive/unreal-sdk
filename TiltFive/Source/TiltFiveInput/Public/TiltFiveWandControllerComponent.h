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
#include "MotionControllerComponent.h"

#include "TiltFiveWandControllerComponent.generated.h"

/**
 *
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class TILTFIVEINPUT_API UTiltFiveWandControllerComponent : public UMotionControllerComponent
{
	GENERATED_BODY()
public:
	// If true, this component will be scaled so it always matches the real world wand, relative to the world scale.
	// Set this to true, if components attached to the wand should always remain the same size in the real world.
	// Set this to false, if components attached to the wand should always remain the same in-game size
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TiltFive")
	bool ScaleWandRelativeToWorldScale = true;

	// The reference scale for the wand to scale it relative to the real world. Usually the default
	// value should suffice.
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, meta = (Units = "Multiplier"), Category = "TiltFive")
	float ReferenceWorldToMetersScale = 100.0f;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
