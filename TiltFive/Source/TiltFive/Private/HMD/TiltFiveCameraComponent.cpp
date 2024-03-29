// Fill out your copyright notice in the Description page of Project Settings.


#include "HMD/TiltFiveCameraComponent.h"
#include "Engine/Engine.h"
#include "IXRTrackingSystem.h"
#include "IXRCamera.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

void UTiltFiveCameraComponent::HandleXRCamera()
{
	IXRTrackingSystem* XRSystem = GEngine->XRSystem.Get();

	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	const AController* OwningController = OwningPawn ? OwningPawn->GetController() : nullptr;
	if (OwningController && OwningController->IsLocalPlayerController())
	{
		const APlayerController *PlayerController = Cast<APlayerController>(OwningController);
		const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
		if (LocalPlayer) {
			auto XRCamera = XRSystem->GetXRCamera(LocalPlayer->GetPlatformUserIndex());

			if (!XRCamera.IsValid())
			{
				return;
			}

			const FTransform ParentWorld = CalcNewComponentToWorld(FTransform());

			XRCamera->SetupLateUpdate(ParentWorld, this, bLockToHmd == 0);

			if (bLockToHmd)
			{
				FQuat Orientation;
				FVector Position;
				if (XRCamera->UpdatePlayerCamera(Orientation, Position))
				{
					SetRelativeTransform(FTransform(Orientation, Position));
				}
				else
				{
					ResetRelativeTransform();
				}
			}

			XRCamera->OverrideFOV(this->FieldOfView);
		}
	}
}