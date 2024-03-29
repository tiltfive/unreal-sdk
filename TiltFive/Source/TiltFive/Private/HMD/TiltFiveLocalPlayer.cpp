// Fill out your copyright notice in the Description page of Project Settings.


#include "HMD/TiltFiveLocalPlayer.h"
#include "HMD/TiltFiveHMD.h"
#include "Engine/ChildConnection.h"
#include "IXRTrackingSystem.h"
#include "IXRCamera.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#include "Net/DataChannel.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "UObject/UObjectAnnotation.h"
#include "Logging/LogScopedCategoryAndVerbosityOverride.h"
#include "Math/InverseRotationMatrix.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/PlayerState.h"
#include "Engine/DebugCameraController.h"
#include "Misc/FileHelper.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"

#include "TiltFiveXRBase.h"

#include "GameDelegates.h"

#if !UE_BUILD_SHIPPING

static TAutoConsoleVariable<int32> CVarViewportTest(
	TEXT("r.Test.ConstrainedView"),
	0,
	TEXT("Allows to test different viewport rectangle configuations (in game only) as they can happen when using cinematics/Editor.\n")
	TEXT("0: off(default)\n")
	TEXT("1..7: Various Configuations"),
	ECVF_RenderThreadSafe);

#endif // !UE_BUILD_SHIPPING

bool UTiltFiveLocalPlayer::GetProjectionData(FViewport* Viewport, FSceneViewProjectionData& ProjectionData, int32 StereoViewIndex) const
{
	// If the actor
	if ((Viewport == NULL) || (PlayerController == NULL) || (Viewport->GetSizeXY().X == 0) || (Viewport->GetSizeXY().Y == 0) || (Size.X == 0) || (Size.Y == 0))
	{
		return false;
	}

	int32 X = FMath::TruncToInt(Origin.X * Viewport->GetSizeXY().X);
	int32 Y = FMath::TruncToInt(Origin.Y * Viewport->GetSizeXY().Y);

	X += Viewport->GetInitialPositionXY().X;
	Y += Viewport->GetInitialPositionXY().Y;

	uint32 SizeX = FMath::TruncToInt(Size.X * Viewport->GetSizeXY().X);
	uint32 SizeY = FMath::TruncToInt(Size.Y * Viewport->GetSizeXY().Y);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	// We expect some size to avoid problems with the view rect manipulation
	if (SizeX > 50 && SizeY > 50)
	{
		int32 Value = CVarViewportTest.GetValueOnGameThread();

		if (Value)
		{
			int InsetX = SizeX / 4;
			int InsetY = SizeY / 4;

			// this allows to test various typical view port situations (todo: split screen)
			switch (Value)
			{
			case 1: X += InsetX; Y += InsetY; SizeX -= InsetX * 2; SizeY -= InsetY * 2; break;
			case 2: Y += InsetY; SizeY -= InsetY * 2; break;
			case 3: X += InsetX; SizeX -= InsetX * 2; break;
			case 4: SizeX /= 2; SizeY /= 2; break;
			case 5: SizeX /= 2; SizeY /= 2; X += SizeX;	break;
			case 6: SizeX /= 2; SizeY /= 2; Y += SizeY; break;
			case 7: SizeX /= 2; SizeY /= 2; X += SizeX; Y += SizeY; break;
			}
		}
	}
#endif

	FIntRect UnconstrainedRectangle = FIntRect(X, Y, X + SizeX, Y + SizeY);

	ProjectionData.SetViewRectangle(UnconstrainedRectangle);

	// Get the viewpoint.
	FMinimalViewInfo ViewInfo;
	GetViewPoint(/*out*/ ViewInfo);

	// If stereo rendering is enabled, update the size and offset appropriately for this pass
	const bool bNeedStereo = StereoViewIndex != INDEX_NONE && GEngine->IsStereoscopic3D();
	const bool bIsHeadTrackingAllowed =
		GEngine->XRSystem.IsValid() &&
		(GetWorld() != nullptr ? GEngine->XRSystem->IsHeadTrackingAllowedForWorld(*GetWorld()) : GEngine->XRSystem->IsHeadTrackingAllowed());
	if (bNeedStereo)
	{
		TSharedPtr<class FTiltFiveXRBase, ESPMode::ThreadSafe> TiltFiveStereoRenderer = StaticCastSharedPtr<class FTiltFiveXRBase, class IStereoRendering, ESPMode::ThreadSafe>(GEngine->StereoRenderingDevice);
		TiltFiveStereoRenderer->AdjustViewRect(StereoViewIndex, X, Y, SizeX, SizeY, GetPlatformUserIndex());
	}

	// scale distances for cull distance purposes by the ratio of our current FOV to the default FOV
	if (GCalcLocalPlayerCachedLODDistanceFactor != 0)
	{
		PlayerController->LocalPlayerCachedLODDistanceFactor = ViewInfo.FOV / FMath::Max<float>(0.01f, (PlayerController->PlayerCameraManager != NULL) ? PlayerController->PlayerCameraManager->DefaultFOV : 90.f);
	}
	else // This should be removed in the final version. Leaving in so this can be toggled on and off in order to evaluate it.
	{
		PlayerController->LocalPlayerCachedLODDistanceFactor = 1.f;
	}

	FVector StereoViewLocation = ViewInfo.Location;
	if (bNeedStereo || bIsHeadTrackingAllowed)
	{
		auto XRCamera = GEngine->XRSystem.IsValid() ? GEngine->XRSystem->GetXRCamera(GetPlatformUserIndex()) : nullptr;
		if (XRCamera.IsValid())
		{
			AActor* ViewTarget = PlayerController->GetViewTarget();
			const bool bHasActiveCamera = ViewTarget && ViewTarget->HasActiveCameraComponent();
			XRCamera->UseImplicitHMDPosition(bHasActiveCamera);
		}

		if (GEngine->StereoRenderingDevice.IsValid())
		{
			TSharedPtr<class FTiltFiveXRBase, ESPMode::ThreadSafe> TiltFiveStereoRenderer = StaticCastSharedPtr<class FTiltFiveXRBase, class IStereoRendering, ESPMode::ThreadSafe>(GEngine->StereoRenderingDevice);
			TiltFiveStereoRenderer->CalculateStereoViewOffset(StereoViewIndex, ViewInfo.Rotation, GetWorld()->GetWorldSettings()->WorldToMeters, StereoViewLocation, GetPlatformUserIndex());
		}
	}

	// Create the view matrix
	ProjectionData.ViewOrigin = StereoViewLocation;
	ProjectionData.ViewRotationMatrix = FInverseRotationMatrix(ViewInfo.Rotation) * FMatrix(
		FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1));

	// @todo viewext this use case needs to be revisited
	if (!bNeedStereo)
	{
		// Create the projection matrix (and possibly constrain the view rectangle)
		FMinimalViewInfo::CalculateProjectionMatrixGivenView(ViewInfo, AspectRatioAxisConstraint, Viewport, /*inout*/ ProjectionData);

		for (auto& ViewExt : GEngine->ViewExtensions->GatherActiveExtensions(FSceneViewExtensionContext(Viewport)))
		{
			ViewExt->SetupViewProjectionMatrix(ProjectionData);
		};
	}
	else
	{
		// Let the stereoscopic rendering device handle creating its own projection matrix, as needed
		TSharedPtr<class FTiltFiveXRBase, ESPMode::ThreadSafe> TiltFiveStereoRenderer = StaticCastSharedPtr<class FTiltFiveXRBase, class IStereoRendering, ESPMode::ThreadSafe>(GEngine->StereoRenderingDevice);
		ProjectionData.ProjectionMatrix = TiltFiveStereoRenderer->GetStereoProjectionMatrix(StereoViewIndex, GetPlatformUserIndex());

		// calculate the out rect
		ProjectionData.SetViewRectangle(FIntRect(X, Y, X + SizeX, Y + SizeY));
	}

	return true;
}