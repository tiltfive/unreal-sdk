// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
//#include "TiltFiveXRBase.h"
#include "TiltFiveManager.generated.h"

UCLASS()
class TILTFIVE_API ATiltFiveManager : public AActor
{
	GENERATED_BODY()

public:	
	UPROPERTY(EditAnywhere, Category="Tilt Five Manager")
	TSubclassOf<APawn> localPlayer1Pawn;

	UPROPERTY(EditAnywhere, Category = "Tilt Five Manager")
	TSubclassOf<APawn> localPlayer2Pawn;

	UPROPERTY(EditAnywhere, Category = "Tilt Five Manager")
	TSubclassOf<APawn> localPlayer3Pawn;

	UPROPERTY(EditAnywhere, Category = "Tilt Five Manager")
	TSubclassOf<APawn> localPlayer4Pawn;

	UPROPERTY(EditAnywhere, Category = "Tilt Five Manager")
	float player1FOV;

	UPROPERTY(EditAnywhere, Category = "Tilt Five Manager")
	float player2FOV;

	UPROPERTY(EditAnywhere, Category = "Tilt Five Manager")
	float player3FOV;

	UPROPERTY(EditAnywhere, Category = "Tilt Five Manager")
	float player4FOV;

	UPROPERTY(EditAnywhere, Category = "Tilt Five Manager")
	uint8 spectatedPlayer;

	// Sets default values for this actor's properties
	ATiltFiveManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void TrackPlayers();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
