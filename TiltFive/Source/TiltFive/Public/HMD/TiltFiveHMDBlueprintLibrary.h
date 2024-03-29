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

#include "Kismet/BlueprintFunctionLibrary.h"

#include "TiltFiveHMDBlueprintLibrary.generated.h"

UCLASS(BlueprintType)
class UT5CamImage : public UObject, public T5_CamImage
{
	GENERATED_BODY()
};

UCLASS(BlueprintType)
class UT5CamData : public UObject
{
	GENERATED_BODY()

public:
	/// \brief The image buffer being filled by the Tilt Five service.
	uint8_t* pixelData;
};

UCLASS()
class TILTFIVE_API UTiltFiveHMDBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "Tilt Five|HMD")
	bool GetFilledCamImageBuffer(UT5CamImage* image, int32 playerIndex);

	UFUNCTION(BlueprintCallable, Category = "Tilt Five|HMD")
	bool SubmitEmptyCamImageBuffer(UT5CamImage* image, int32 playerIndex);

	UFUNCTION(BlueprintCallable, Category = "Tilt Five|HMD")
	bool CancelCamImageBuffer(UT5CamData* buffer, int32 playerIndex);

	UFUNCTION(BlueprintCallable, Category = "Tilt Five|HMD")
	void SetSpectatedPlayer(int32 playerIndex);

	UFUNCTION(BlueprintCallable, Category = "Tilt Five|HMD")
	bool IsTiltFiveUiRequestingAttention();
};
