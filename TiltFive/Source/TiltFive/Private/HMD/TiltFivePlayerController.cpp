// Fill out your copyright notice in the Description page of Project Settings.


#include "HMD/TiltFivePlayerController.h"
#include "Engine/Engine.h"
#include "IXRTrackingSystem.h"
#include "IXRCamera.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/SpectatorPawn.h"

void ATiltFivePlayerController::UpdateRotation(float DeltaTime)
{
	// Calculate Delta to be applied on ViewRotation
	FRotator DeltaRot(RotationInput);

	FRotator ViewRotation = GetControlRotation();

	if (PlayerCameraManager)
	{
		PlayerCameraManager->ProcessViewRotation(DeltaTime, ViewRotation, DeltaRot);
	}
	AActor* ViewTarget = GetViewTarget();
	if (!PlayerCameraManager || !ViewTarget || !ViewTarget->HasActiveCameraComponent() || ViewTarget->HasActivePawnControlCameraComponent())
	{
		if (IsLocalPlayerController() && GEngine->XRSystem.IsValid() && GetWorld() != nullptr && GEngine->XRSystem->IsHeadTrackingAllowedForWorld(*GetWorld()))
		{
			auto XRCamera = GEngine->XRSystem->GetXRCamera(GetLocalPlayer()->GetLocalPlayerIndex());
			if (XRCamera.IsValid())
			{
				XRCamera->ApplyHMDRotation(this, ViewRotation);
			}
		}
	}

	SetControlRotation(ViewRotation);

	APawn* const P = GetPawnOrSpectator();
	if (P)
	{
		P->FaceRotation(ViewRotation, DeltaTime);
	}
}