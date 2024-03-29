// Fill out your copyright notice in the Description page of Project Settings.


#include "HMD/TiltFiveXRCamera.h"
#include "GameFramework/PlayerController.h"

#include "ClearQuad.h"
#include "PipelineStateCache.h"
#include "ScreenRendering.h"
#if UE_VERSION_NEWER_THAN(4, 21, 2)
#include "CommonRenderResources.h"
#endif

FTiltFiveSceneViewExtension::FTiltFiveSceneViewExtension(const FAutoRegister& AutoRegister, IXRTrackingSystem *InTrackingSystem):
	FHMDSceneViewExtension(AutoRegister)
	, TrackingSystem(InTrackingSystem)
{
}

int32 FTiltFiveSceneViewExtension::GetPriority() const
{
	// We want to run before the FDefaultXRCamera's view extension, as well as the SteamVR and Oculus VR headsets
	return 1;
}

void FTiltFiveSceneViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	check(IsInGameThread());
}

void FTiltFiveSceneViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	check(IsInGameThread());

	FQuat DeviceOrientation;
	FVector DevicePosition;

	if (TrackingSystem->GetCurrentPose(InView.PlayerIndex, DeviceOrientation, DevicePosition))
	{
		InView.BaseHmdOrientation = DeviceOrientation;
		InView.BaseHmdLocation = DevicePosition;
	}

	TSharedPtr<class FTiltFiveXRBase, ESPMode::ThreadSafe> TiltFiveXRSystem = StaticCastSharedPtr<class FTiltFiveXRBase, class IXRTrackingSystem, ESPMode::ThreadSafe>(GEngine->XRSystem);
	uint32 RTWidth = TiltFiveXRSystem->GlassesList[InView.PlayerIndex]->EyeInfos[InView.StereoViewIndex].BufferedRTRHI->GetSizeX();
	uint32 RTHeight = TiltFiveXRSystem->GlassesList[InView.PlayerIndex]->EyeInfos[InView.StereoViewIndex].BufferedRTRHI->GetSizeY();
	InView.UnconstrainedViewRect = FIntRect(InView.StereoViewIndex * RTWidth, InView.PlayerIndex * RTHeight, (InView.StereoViewIndex * RTWidth) + RTWidth, (InView.PlayerIndex * RTHeight) + RTHeight);
}

void FTiltFiveSceneViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	check(IsInGameThread());
	TSharedPtr<class FTiltFiveXRBase, ESPMode::ThreadSafe> TiltFiveXRSystem = StaticCastSharedPtr<class FTiltFiveXRBase, class IXRTrackingSystem, ESPMode::ThreadSafe>(GEngine->XRSystem);
	if (TiltFiveXRSystem->SpectatorScreenController != nullptr)
	{
		TiltFiveXRSystem->SpectatorScreenController->BeginRenderViewFamily();
	}
}

void FTiltFiveSceneViewExtension::PreRenderViewFamily_RenderThread(
#if UE_VERSION_OLDER_THAN(5, 1, 0)
	FRHICommandListImmediate& RHICmdList,
#else
	FRDGBuilder& GraphBuilder,
#endif
	FSceneViewFamily& InViewFamily)
{
	check(IsInRenderingThread());

	TrackingSystem->OnBeginRendering_RenderThread(GraphBuilder.RHICmdList, InViewFamily);

	TSharedPtr<class FTiltFiveXRBase, ESPMode::ThreadSafe> TiltFiveXRSystem = StaticCastSharedPtr<class FTiltFiveXRBase, class IXRTrackingSystem, ESPMode::ThreadSafe>(GEngine->XRSystem);

	for (TSharedPtr<class FTiltFiveHMD, ESPMode::ThreadSafe> HMD : TiltFiveXRSystem->GlassesList) {
		if (HMD->IsHMDEnabled()) {
			FQuat CurrentOrientation;
			FVector CurrentPosition;
			if (TrackingSystem->DoesSupportLateUpdate() && TrackingSystem->GetCurrentPose(HMD->DeviceId, CurrentOrientation, CurrentPosition))
			{
				const FSceneView* MainView = InViewFamily.Views[HMD->DeviceId];
				check(MainView);

				const FTransform OldRelativeTransform(MainView->BaseHmdOrientation, MainView->BaseHmdLocation);
				const FTransform CurrentRelativeTransform(CurrentOrientation, CurrentPosition);

				LateUpdate.Apply_RenderThread(InViewFamily.Scene, OldRelativeTransform, CurrentRelativeTransform);
				TrackingSystem->OnLateUpdateApplied_RenderThread(GraphBuilder.RHICmdList, CurrentRelativeTransform);
			}
		}
	}

	if (!InViewFamily.RenderTarget->GetRenderTargetTexture())
	{
		return;
	}
}

void FTiltFiveSceneViewExtension::PreRenderView_RenderThread(
#if UE_VERSION_OLDER_THAN(5, 1, 0)
	FRHICommandListImmediate& RHICmdList,
#else
	FRDGBuilder& GraphBuilder,
#endif
	FSceneView& InView)
{
	check(IsInRenderingThread());
}

void FTiltFiveSceneViewExtension::PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
	check(IsInRenderingThread());
}
#if UE_VERSION_OLDER_THAN(5, 1, 0)
bool FTiltFiveSceneViewExtension::IsActiveThisFrame(class FViewport* InViewport) const
{
	return IsStereoEnabled();
}
#endif
