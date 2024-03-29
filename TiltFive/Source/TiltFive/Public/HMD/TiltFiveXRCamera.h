// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Misc/EngineVersionComparison.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SceneViewExtension.h"
#include "RenderGraphBuilder.h"
#include "IXRTrackingSystem.h"
#include "TiltFiveXRBase.h"
#include "TiltFiveHMD.h"
#include "LateUpdateManager.h"

class TILTFIVE_API FTiltFiveSceneViewExtension : public FHMDSceneViewExtension
{
public:
	FTiltFiveSceneViewExtension(const FAutoRegister& AutoRegister, IXRTrackingSystem* InTrackingSystem);
	virtual ~FTiltFiveSceneViewExtension()
	{}

	virtual int32 GetPriority() const override;
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void PreRenderView_RenderThread(
#if UE_VERSION_OLDER_THAN(5, 1, 0)
		FRHICommandListImmediate& RHICmdList,
#else
		FRDGBuilder& GraphBuilder,
#endif
		FSceneView& InView) override;
	virtual void PreRenderViewFamily_RenderThread(
#if UE_VERSION_OLDER_THAN(5, 1, 0)
		FRHICommandListImmediate& RHICmdList,
#else
		FRDGBuilder& GraphBuilder,
#endif
		FSceneViewFamily& InViewFamily) override;

#if UE_VERSION_OLDER_THAN(5, 1, 0)
	virtual bool IsActiveThisFrame(class FViewport* InViewport) const override;
#endif

	virtual void PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;


	IXRTrackingSystem* TrackingSystem;

	FLateUpdateManager LateUpdate;

	class IRendererModule* RendererModule;

};
