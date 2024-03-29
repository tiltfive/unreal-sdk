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
	bool bClearBlack,
	bool bNoAlpha) const
{
	check(IsInRenderingThread());

	FRHITexture2D* DstTexture2D = DstTexture->GetTexture2D();
	FRHITexture2D* SrcTexture2D = SrcTexture->GetTexture2D();

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

	if (bNoAlpha)
	{
		GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB>::GetRHI();
	}
	else
	{
		// for mirror window, write RGBA, RGB = src.rgb * src.a + dst.rgb * (1 - src.a), A = src.a *
		// 1 + dst.a * (1 - src a)
		GraphicsPSOInit.BlendState =
			TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::
				GetRHI();
	}

	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	const auto FeatureLevel = GMaxRHIFeatureLevel;
	auto ShaderMap = GetGlobalShaderMap(FeatureLevel);
	TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();

	if (DstTexture2D)
	{
		const bool sRGBSource = ((SrcTexture->GetFlags() & TexCreate_SRGB) != 0);

		uint32 NumMips = 1;

		for (uint32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
		{
			FRHIRenderPassInfo RPInfo(DstTexture, ERenderTargetActions::Load_Store);
			RPInfo.ColorRenderTargets[0].MipIndex = MipIndex;

			RHICmdList.BeginRenderPass(RPInfo, TEXT("CopyTexture"));
			{
				const uint32 MipViewportWidth = ViewportWidth >> MipIndex;
				const uint32 MipViewportHeight = ViewportHeight >> MipIndex;
				const FIntPoint MipTargetSize(MipViewportWidth, MipViewportHeight);

				if (bClearBlack)
				{
					RHICmdList.SetViewport(DstRect.Min.X, DstRect.Min.Y, 0.0f, DstRect.Max.X, DstRect.Max.Y, 1.0f);
					DrawClearQuad(RHICmdList, FLinearColor::Black);
				}

				RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
				FRHISamplerState* SamplerState = DstRect.Size() == SrcRect.Size() ? TStaticSamplerState<SF_Point>::GetRHI()
																				  : TStaticSamplerState<SF_Bilinear>::GetRHI();

				if (!sRGBSource)
				{
					TShaderMapRef<FScreenPSMipLevel> PixelShader(ShaderMap);
					GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
					SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
					PixelShader->SetParameters(RHICmdList, SamplerState, SrcTextureRHI, MipIndex);
				}
				else
				{
					TShaderMapRef<FScreenPSsRGBSourceMipLevel> PixelShader(ShaderMap);
					GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
					SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
					PixelShader->SetParameters(RHICmdList, SamplerState, SrcTextureRHI, MipIndex);
				}

				RHICmdList.SetViewport(
					DstRect.Min.X, DstRect.Min.Y, 0.0f, DstRect.Min.X + MipViewportWidth, DstRect.Min.Y + MipViewportHeight, 1.0f);

				RendererModule->DrawRectangle(RHICmdList,
					0,
					0,
					MipViewportWidth,
					MipViewportHeight,
					U,
					V,
					USize,
					VSize,
					MipTargetSize,
					FIntPoint(1, 1),
					VertexShader,
					EDRF_Default);
			}
			RHICmdList.EndRenderPass();
		}
	}
}
