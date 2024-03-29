// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LocalPlayer.h"
#include "TiltFiveLocalPlayer.generated.h"

/**
 * 
 */
UCLASS()
class TILTFIVE_API UTiltFiveLocalPlayer : public ULocalPlayer
{
	GENERATED_BODY()

	int32 GCalcLocalPlayerCachedLODDistanceFactor = 1;
	bool GetProjectionData(FViewport* Viewport, FSceneViewProjectionData& ProjectionData, int32 StereoViewIndex) const override;
};
