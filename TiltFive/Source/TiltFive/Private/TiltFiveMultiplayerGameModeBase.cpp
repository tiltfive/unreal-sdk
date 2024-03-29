// Fill out your copyright notice in the Description page of Project Settings.

#include "TiltFiveMultiplayerGameModeBase.h"

#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TiltFiveXRBase.h"
#include "HMD/TiltFiveHMD.h"
#include "HMD/TiltFivePlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "MotionControllerComponent.h"


ATiltFiveMultiplayerGameModeBase::ATiltFiveMultiplayerGameModeBase(const FObjectInitializer& ObjectInitializer):
Super(ObjectInitializer){
	PlayerControllerClass = ATiltFivePlayerController::StaticClass();
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
}

void ATiltFiveMultiplayerGameModeBase::TrackPlayers() {
	TSharedPtr<class FTiltFiveXRBase, ESPMode::ThreadSafe> TiltFiveXRSystem = StaticCastSharedPtr<class FTiltFiveXRBase, class IXRTrackingSystem, ESPMode::ThreadSafe>(GEngine->XRSystem);
	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> Hmd : TiltFiveXRSystem->GlassesList) {
		if (Hmd->IsHMDEnabled()) {
			if (!IsValid(Hmd->controlledPawn)) {
				if (Hmd->DeviceId == 0) {
					Hmd->controlledPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
				}
				else {
					APlayerController* controller = UGameplayStatics::CreatePlayer(GetWorld(), Hmd->DeviceId, true);
					APawn* pawn = controller->GetPawn();
					Hmd->controlledPawn = pawn;
				}
				UMotionControllerComponent* wandComponent = Hmd->controlledPawn->FindComponentByClass<UMotionControllerComponent>();
				if (wandComponent) {
					wandComponent->PlayerIndex = Hmd->DeviceId;
				}
			}
		}
		else {
			if (IsValid(Hmd->controlledPawn)) {
				UGameplayStatics::RemovePlayer(UGameplayStatics::GetPlayerController(GetWorld(), Hmd->DeviceId), true);
				Hmd->controlledPawn = nullptr;
			}
		}
	}
	return;
}

void ATiltFiveMultiplayerGameModeBase::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);
	TrackPlayers();
}

void ATiltFiveMultiplayerGameModeBase::BeginPlay() {
	TSharedPtr<class FTiltFiveXRBase, ESPMode::ThreadSafe> TiltFiveXRSystem = StaticCastSharedPtr<class FTiltFiveXRBase, class IXRTrackingSystem, ESPMode::ThreadSafe>(GEngine->XRSystem);
	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> Hmd : TiltFiveXRSystem->GlassesList) {
		Hmd->controlledPawn = nullptr;
	}
}