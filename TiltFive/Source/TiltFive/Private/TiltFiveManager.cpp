// Fill out your copyright notice in the Description page of Project Settings.


#include "TiltFiveManager.h"

#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TiltFiveXRBase.h"
#include "HMD/TiltFiveHMD.h"
#include "HMD/TiltFivePlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "MotionControllerComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
ATiltFiveManager::ATiltFiveManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	USceneComponent *SceneRootComponent = CreateDefaultSubobject<USceneComponent>("Root Scene Component");
	SetRootComponent(SceneRootComponent);
}

// Called when the game starts or when spawned
void ATiltFiveManager::BeginPlay()
{
	Super::BeginPlay();
	TSharedPtr<class FTiltFiveXRBase, ESPMode::ThreadSafe> TiltFiveXRSystem = StaticCastSharedPtr<class FTiltFiveXRBase, class IXRTrackingSystem, ESPMode::ThreadSafe>(GEngine->XRSystem);
	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> Hmd : TiltFiveXRSystem->GlassesList) {
		Hmd->controlledPawn = nullptr;
	}
}

void ATiltFiveManager::TrackPlayers() {
	TSharedPtr<class FTiltFiveXRBase, ESPMode::ThreadSafe> TiltFiveXRSystem = StaticCastSharedPtr<class FTiltFiveXRBase, class IXRTrackingSystem, ESPMode::ThreadSafe>(GEngine->XRSystem);
	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> Hmd : TiltFiveXRSystem->GlassesList) {
		if (Hmd->IsHMDEnabled()) {
			if (!IsValid(Hmd->controlledPawn)) {
				if (UGameplayStatics::GetPlayerControllerFromID(GetWorld(), Hmd->DeviceId)) {
					UGameplayStatics::RemovePlayer(UGameplayStatics::GetPlayerControllerFromID(GetWorld(), Hmd->DeviceId), true);
				}
				APawn* newPawn;
				if (Hmd->DeviceId == 0) {
					newPawn = GetWorld()->SpawnActor<APawn>(localPlayer1Pawn);
				}
				else if (Hmd->DeviceId == 1) {
					newPawn = GetWorld()->SpawnActor<APawn>(localPlayer2Pawn);
				}
				else if (Hmd->DeviceId == 2) {
					newPawn = GetWorld()->SpawnActor<APawn>(localPlayer3Pawn);
				}
				else if (Hmd->DeviceId == 3) {
					newPawn = GetWorld()->SpawnActor<APawn>(localPlayer4Pawn);
				}
				else {
					continue;
				}
				UGameplayStatics::CreatePlayer(GetWorld(), Hmd->DeviceId, true);
				AController* controller = UGameplayStatics::GetPlayerControllerFromID(GetWorld(), Hmd->DeviceId);
				controller->Possess(newPawn);
				Hmd->controlledPawn = controller->GetPawn();
				UMotionControllerComponent* wandComponent = Hmd->controlledPawn->FindComponentByClass<UMotionControllerComponent>();
				if (wandComponent) {
					wandComponent->PlayerIndex = Hmd->DeviceId;
				}
			}
		}
		else {
			if (IsValid(Hmd->controlledPawn)) {
				UGameplayStatics::RemovePlayer(UGameplayStatics::GetPlayerControllerFromID(GetWorld(), Hmd->DeviceId), true);
				Hmd->controlledPawn = nullptr;
			}
		}
	}
	return;
}
// Called every frame
void ATiltFiveManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TrackPlayers();
	TSharedPtr<class FTiltFiveXRBase, ESPMode::ThreadSafe> TiltFiveXRSystem = StaticCastSharedPtr<class FTiltFiveXRBase, class IXRTrackingSystem, ESPMode::ThreadSafe>(GEngine->XRSystem);
	TiltFiveXRSystem->SetSpectatedPlayer(spectatedPlayer);
	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> Hmd : TiltFiveXRSystem->GlassesList) {
		if (Hmd->DeviceId == 0) {
			Hmd->OverrideFOV(player1FOV);
		}
		else if (Hmd->DeviceId == 1) {
			Hmd->OverrideFOV(player2FOV);
		}
		else if (Hmd->DeviceId == 2) {
			Hmd->OverrideFOV(player3FOV);
		}
		else if (Hmd->DeviceId == 3) {
			Hmd->OverrideFOV(player4FOV);
		}
		else {
			continue;
		}
	}
}

