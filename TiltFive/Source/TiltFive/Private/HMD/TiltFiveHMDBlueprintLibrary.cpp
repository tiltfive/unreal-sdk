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

#include "HMD/TiltFiveHMDBlueprintLibrary.h"

#include "TiltFive.h"
#include "HMD/TiltFiveHMD.h"
#include "TiltFiveXRBase.h"

bool UTiltFiveHMDBlueprintLibrary::GetFilledCamImageBuffer(UT5CamImage* incomingImageBuffer, int32 playerIndex)
{
	if (FTiltFiveModule::Get().GetHMD()->GlassesList[playerIndex]->IsHMDEnabled()) {
		return false;
	}
	T5_Result err = t5GetFilledCamImageBuffer(FTiltFiveModule::Get().GetHMD()->GlassesList[playerIndex]->GetCurrentExclusiveGlasses(), incomingImageBuffer);
	if (!err)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool UTiltFiveHMDBlueprintLibrary::SubmitEmptyCamImageBuffer(UT5CamImage* incomingImageBuffer, int32 playerIndex)
{
	if (FTiltFiveModule::Get().GetHMD()->GlassesList[playerIndex]->IsHMDEnabled()) {
		return false;
	}
	// Make sure to allocate and pin the memory here for the image buffer
	T5_Result err = t5SubmitEmptyCamImageBuffer(FTiltFiveModule::Get().GetHMD()->GlassesList[playerIndex]->GetCurrentExclusiveGlasses(), incomingImageBuffer);
	if (!err)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool UTiltFiveHMDBlueprintLibrary::CancelCamImageBuffer(UT5CamData* buffer, int32 playerIndex)
{
	if (FTiltFiveModule::Get().GetHMD()->GlassesList[playerIndex]->IsHMDEnabled()) {
		return false;
	}
	T5_Result err = t5CancelCamImageBuffer(FTiltFiveModule::Get().GetHMD()->GlassesList[playerIndex]->GetCurrentExclusiveGlasses(), buffer->pixelData);
	if (!err)
	{
		return true;
	}
	return false;
}

void UTiltFiveHMDBlueprintLibrary::SetSpectatedPlayer(int32 playerIndex)
{
	FTiltFiveModule::Get().GetHMD()->SetSpectatedPlayer(playerIndex);
	return;
}

bool UTiltFiveHMDBlueprintLibrary::IsTiltFiveUiRequestingAttention()
{
	int64_t value = 0;

	T5_Result result = t5GetSystemIntegerParam(FTiltFiveModule::Get().GetContext(), kT5_ParamSys_Integer_CPL_AttRequired, &value);
	if (result == T5_SUCCESS)
	{
		return value != 0;
	}
	else
	{
		return false;
	}
}
