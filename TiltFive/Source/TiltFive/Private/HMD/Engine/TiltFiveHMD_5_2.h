// Copyright 2022 Tilt Five, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "RHIStaticStates.h"

void FTiltFiveXRBase::CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList,
	FRHITexture2D* SrcTexture,
	FIntRect SrcRect,
	FRHITexture2D* DstTexture,
	FIntRect DstRect,
	bool bClearBlack,
	bool bNoAlpha) const
{
	check(IsInRenderingThread());

	const uint32 ViewportWidth = DstRect.Width();
	const uint32 ViewportHeight = DstRect.Height();
	const FIntPoint TargetSize(ViewportWidth, ViewportHeight);

	const float SrcTextureWidth = SrcTexture->GetSizeX();
	const float SrcTextureHeight = SrcTexture->GetSizeY();

	float U = 0.f, V = 0.f, USize = 1.f, VSize = 1.f;
	if (!SrcRect.IsEmpty())
	{
		U = SrcRect.Min.X / SrcTextureWidth;
		V = SrcRect.Min.Y / SrcTextureHeight;
		USize = SrcRect.Width() / SrcTextureWidth;
		VSize = SrcRect.Height() / SrcTextureHeight;
	}

	RHICmdList.Transition(FRHITransitionInfo(SrcTexture, ERHIAccess::Unknown, ERHIAccess::SRVGraphics));

	// #todo-renderpasses Possible optimization here - use DontLoad if we will immediately clear the entire target
	FRHIRenderPassInfo RPInfo(DstTexture, ERenderTargetActions::Load_Store);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("TiltFiveHMD_CopyTexture"));
	{
		if (bClearBlack)
		{
			const FIntRect ClearRect(0, 0, DstTexture->GetSizeX(), DstTexture->GetSizeY());
			RHICmdList.SetViewport(ClearRect.Min.X, ClearRect.Min.Y, 0, ClearRect.Max.X, ClearRect.Max.Y, 1.0f);
			DrawClearQuad(RHICmdList, FLinearColor::Black);
		}

		RHICmdList.SetViewport(DstRect.Min.X, DstRect.Min.Y, 0, DstRect.Max.X, DstRect.Max.Y, 1.0f);

		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.BlendState =
			bNoAlpha
				? TStaticBlendState<>::GetRHI()
				: TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::
					  GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		const auto FeatureLevel = GMaxRHIFeatureLevel;
		auto ShaderMap = GetGlobalShaderMap(FeatureLevel);

		TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();

		const bool bSameSize = DstRect.Size() == SrcRect.Size();
		FRHISamplerState* PixelSampler =
			bSameSize ? TStaticSamplerState<SF_Point>::GetRHI() : TStaticSamplerState<SF_Bilinear>::GetRHI();

		if (EnumHasAnyFlags(SrcTexture->GetFlags(), TexCreate_SRGB))
		{
			TShaderMapRef<FScreenPSsRGBSource> PixelShader(ShaderMap);
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
			PixelShader->SetParameters(RHICmdList, PixelSampler, SrcTexture);
		}
		else
		{
			TShaderMapRef<FScreenPS> PixelShader(ShaderMap);
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
			PixelShader->SetParameters(RHICmdList, PixelSampler, SrcTexture);
		}

		RendererModule->DrawRectangle(RHICmdList,
			0,
			0,
			ViewportWidth,
			ViewportHeight,
			U,
			V,
			USize,
			VSize,
			TargetSize,
			FIntPoint(1, 1),
			VertexShader,
			EDRF_Default);
	}
	RHICmdList.EndRenderPass();
}