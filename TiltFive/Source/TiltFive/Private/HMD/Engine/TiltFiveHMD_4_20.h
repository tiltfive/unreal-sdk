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

void FTiltFiveXRBase::CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList,
	FRHITexture2D* SrcTexture,
	FIntRect SrcRect,
	FRHITexture2D* DstTexture,
	FIntRect DstRect,
	bool bClearBlack) const
{
	check(IsInRenderingThread());

	FTexture2DRHIParamRef DstTexture2D = DstTexture->GetTexture2D();
	FTexture2DRHIParamRef SrcTexture2D = SrcTexture->GetTexture2D();
	FIntPoint DstSize;
	FIntPoint SrcSize;

	if (DstTexture2D && SrcTexture2D)
	{
		DstSize = FIntPoint(DstTexture2D->GetSizeX(), DstTexture2D->GetSizeY());
		SrcSize = FIntPoint(SrcTexture2D->GetSizeX(), SrcTexture2D->GetSizeY());
	}
	else
	{
		return;
	}

	if (DstRect.IsEmpty())
	{
		DstRect = FIntRect(FIntPoint::ZeroValue, DstSize);
	}

	if (SrcRect.IsEmpty())
	{
		SrcRect = FIntRect(FIntPoint::ZeroValue, SrcSize);
	}

	const uint32 ViewportWidth = DstRect.Width();
	const uint32 ViewportHeight = DstRect.Height();
	const FIntPoint TargetSize(ViewportWidth, ViewportHeight);
	float U = SrcRect.Min.X / (float) SrcSize.X;
	float V = SrcRect.Min.Y / (float) SrcSize.Y;
	float USize = SrcRect.Width() / (float) SrcSize.X;
	float VSize = SrcRect.Height() / (float) SrcSize.Y;

	FRHITexture* SrcTextureRHI = SrcTexture;
	RHICmdList.TransitionResources(EResourceTransitionAccess::EReadable, &SrcTextureRHI, 1);
	FGraphicsPipelineStateInitializer GraphicsPSOInit;

	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();

	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	const auto FeatureLevel = GMaxRHIFeatureLevel;
	auto ShaderMap = GetGlobalShaderMap(FeatureLevel);
	TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = RendererModule->GetFilterVertexDeclaration().VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);

	if (DstTexture2D)
	{
		SetRenderTarget(RHICmdList, DstTexture, FTextureRHIRef());

		if (bClearBlack)
		{
			DrawClearQuad(RHICmdList, FLinearColor::Black);
		}

		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

		TShaderMapRef<FScreenPS> PixelShader(ShaderMap);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
		FSamplerStateRHIParamRef SamplerState =
			DstRect.Size() == SrcRect.Size() ? TStaticSamplerState<SF_Point>::GetRHI() : TStaticSamplerState<SF_Bilinear>::GetRHI();
		PixelShader->SetParameters(RHICmdList, SamplerState, SrcTextureRHI);

		RHICmdList.SetViewport(DstRect.Min.X, DstRect.Min.Y, 0.0f, DstRect.Max.X, DstRect.Max.Y, 1.0f);

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
			*VertexShader,
			EDRF_Default);
	}
}
