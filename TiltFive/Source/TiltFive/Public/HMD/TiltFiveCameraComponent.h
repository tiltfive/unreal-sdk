// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "TiltFiveCameraComponent.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class TILTFIVE_API UTiltFiveCameraComponent : public UCameraComponent
{
	GENERATED_BODY()
	
	virtual void HandleXRCamera() override;
};
