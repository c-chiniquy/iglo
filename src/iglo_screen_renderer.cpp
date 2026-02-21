
#include "iglo.h"
#include "iglo_screen_renderer.h"

#include "shaders/VS_FullscreenQuad.h"
#include "shaders/PS_FullscreenQuad.h"

#define SHADER_VS(a) Shader(a, sizeof(a), "VSMain")
#define SHADER_PS(a) Shader(a, sizeof(a), "PSMain")

namespace ig
{
	std::unique_ptr<ScreenRenderer> ScreenRenderer::Create(const IGLOContext& context, const RenderTargetDesc& renderTargetDesc)
	{
		const char* errStr = "Failed to create screen renderer.";

		std::unique_ptr<ScreenRenderer> out = std::unique_ptr<ScreenRenderer>(new ScreenRenderer(context));

		std::array<BlendDesc, 3> blendDesc =
		{
			BlendDesc::BlendDisabled,
			BlendDesc::StraightAlpha,
			BlendDesc::PremultipliedAlpha,
		};

		for (uint32_t i = 0; i < 3; i++)
		{
			out->pipelines.at(i) = Pipeline::CreateGraphics(context,
				SHADER_VS(g_VS_FullscreenQuad),
				SHADER_PS(g_PS_FullscreenQuad),
				renderTargetDesc, {}, PrimitiveTopology::TriangleStrip,
				DepthDesc::DepthDisabled, RasterizerDesc::NoCull, { blendDesc[i] });
			if (!out->pipelines.at(i))
			{
				Log(LogType::Error, errStr);
				return nullptr;
			}
		}

		{
			SamplerDesc desc;
			desc.filter = TextureFilter::Point;
			desc.wrapU = TextureWrapMode::Clamp;
			desc.wrapV = TextureWrapMode::Clamp;
			desc.wrapW = TextureWrapMode::Clamp;
			out->samplerPoint = Sampler::Create(context, desc);
			if (!out->samplerPoint)
			{
				Log(LogType::Error, errStr);
				return nullptr;
			}
		}

		{
			SamplerDesc desc;
			desc.filter = TextureFilter::AnisotropicX16;
			desc.wrapU = TextureWrapMode::Clamp;
			desc.wrapV = TextureWrapMode::Clamp;
			desc.wrapW = TextureWrapMode::Clamp;
			out->samplerAnisotropicClamp = Sampler::Create(context, desc);
			if (!out->samplerAnisotropicClamp)
			{
				Log(LogType::Error, errStr);
				return nullptr;
			}
		}

		return out;
	}

	void ScreenRenderer::DrawFullscreenQuad(CommandList& cmd, const Texture& source, const Texture& destTarget, ScreenRendererBlend blend)
	{
		if (!source.GetDescriptor()) throw std::runtime_error("No SRV.");

		bool sameSize =
			(source.GetWidth() == destTarget.GetWidth()) &&
			(source.GetHeight() == destTarget.GetHeight());

		// If source and dest texture are the same size, use point sampling (fastest).
		const Sampler& chosenSampler = sameSize ? *samplerPoint : *samplerAnisotropicClamp;

		struct PushConstants
		{
			uint32_t textureIndex = IGLO_UINT32_MAX;
			uint32_t samplerIndex = IGLO_UINT32_MAX;
		};
		PushConstants pushConstants;
		pushConstants.textureIndex = source.GetDescriptor().heapIndex;
		pushConstants.samplerIndex = chosenSampler.GetDescriptor().heapIndex;

		cmd.SetPipeline(*pipelines.at((uint32_t)blend));
		cmd.SetPushConstants(&pushConstants, sizeof(pushConstants));
		cmd.Draw(4);
	}

}