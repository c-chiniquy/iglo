
#include "iglo.h"
#include "iglo_screen_renderer.h"

#include "shaders/VS_FullscreenQuad.h"
#include "shaders/PS_FullscreenQuad.h"

#define SHADER_VS(a) Shader(a, sizeof(a), "VSMain")
#define SHADER_PS(a) Shader(a, sizeof(a), "PSMain")

namespace ig
{
	void ScreenRenderer::Unload()
	{
		isLoaded = false;
		context = nullptr;

		for (uint32_t i = 0; i < 3; i++)
		{
			pipeline[i].Unload();
		}

		samplerPoint.Unload();
		samplerAnisotropicClamp.Unload();
	}

	bool ScreenRenderer::Load(IGLOContext& context, const RenderTargetDesc& renderTargetDesc)
	{
		Unload();

		const char* errStr = "Failed to load screen renderer.";

		this->context = &context;

		std::array<BlendDesc, 3> blendDesc =
		{
			BlendDesc::BlendDisabled,
			BlendDesc::StraightAlpha,
			BlendDesc::PremultipliedAlpha,
		};

		for (uint32_t i = 0; i < 3; i++)
		{
			if (!this->pipeline[i].Load(context,
				SHADER_VS(g_VS_FullscreenQuad), SHADER_PS(g_PS_FullscreenQuad),
				renderTargetDesc, {}, PrimitiveTopology::TriangleStrip,
				DepthDesc::DepthDisabled, RasterizerDesc::NoCull, { blendDesc[i] }))
			{
				Log(LogType::Error, errStr);
				Unload();
				return false;
			}
		}

		{
			SamplerDesc desc;
			desc.filter = TextureFilter::Point;
			desc.wrapU = TextureWrapMode::Clamp;
			desc.wrapV = TextureWrapMode::Clamp;
			desc.wrapW = TextureWrapMode::Clamp;
			if (!this->samplerPoint.Load(context, desc))
			{
				Log(LogType::Error, errStr);
				Unload();
				return false;
			}
		}

		{
			SamplerDesc desc;
			desc.filter = TextureFilter::AnisotropicX16;
			desc.wrapU = TextureWrapMode::Clamp;
			desc.wrapV = TextureWrapMode::Clamp;
			desc.wrapW = TextureWrapMode::Clamp;
			if (!this->samplerAnisotropicClamp.Load(context, desc))
			{
				Log(LogType::Error, errStr);
				Unload();
				return false;
			}
		}

		this->isLoaded = true;
		return true;
	}

	void ScreenRenderer::DrawFullscreenQuad(CommandList& cmd, const Texture& source, const Texture& destTarget, ScreenRendererBlend blend)
	{
		if (!source.IsLoaded() || !source.GetDescriptor() || !destTarget.IsLoaded())
		{
			Log(LogType::Error, "Failed to draw fullscreen quad. Reason: Invalid source and dest textures.");
			return;
		}

		bool sameSize =
			(source.GetWidth() == destTarget.GetWidth()) &&
			(source.GetHeight() == destTarget.GetHeight());

		// If source and dest texture are the same size, use point sampling (fastest).
		const Sampler& chosenSampler = sameSize ? samplerPoint : samplerAnisotropicClamp;

		struct PushConstants
		{
			uint32_t textureIndex = IGLO_UINT32_MAX;
			uint32_t samplerIndex = IGLO_UINT32_MAX;
		};
		PushConstants pushConstants;
		pushConstants.textureIndex = source.GetDescriptor()->heapIndex;
		pushConstants.samplerIndex = chosenSampler.GetDescriptor()->heapIndex;

		cmd.SetPipeline(pipeline[(uint32_t)blend]);
		cmd.SetPushConstants(&pushConstants, sizeof(pushConstants));
		cmd.Draw(4);
	}

}