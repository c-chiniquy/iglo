
#pragma once

namespace ig
{
	enum class ScreenRendererBlend
	{
		BlendDisabled = 0,
		StraightAlpha,
		PremultipliedAlpha
	};

	class ScreenRenderer
	{
	private:
		ScreenRenderer(const IGLOContext& context) : context(context) {}

		ScreenRenderer& operator=(const ScreenRenderer&) = delete;
		ScreenRenderer(const ScreenRenderer&) = delete;

	public:
		static std::unique_ptr<ScreenRenderer> Create(const IGLOContext&, const RenderTargetDesc&);

		// This function sets a pipeline state and then issues a draw command.
		// You are required to set a rendertarget, scissor rect, and viewport before calling this function.
		void DrawFullscreenQuad(CommandList&, const Texture& source, const Texture& destTarget,
			ScreenRendererBlend blend = ScreenRendererBlend::BlendDisabled);

	private:
		const IGLOContext& context;
		std::array<std::unique_ptr<Pipeline>, 3> pipelines; // [blend]
		std::unique_ptr<Sampler> samplerPoint;
		std::unique_ptr<Sampler> samplerAnisotropicClamp;

	};

}