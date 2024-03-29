// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TiltFiveMultiplayerGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class TILTFIVE_API ATiltFiveMultiplayerGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

	ATiltFiveMultiplayerGameModeBase(const FObjectInitializer& ObjectInitializer);
	
	void TrackPlayers();

	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginPlay() override;
};
