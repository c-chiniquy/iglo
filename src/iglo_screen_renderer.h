
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
	public:
		ScreenRenderer() = default;
		~ScreenRenderer() { Unload(); }

		ScreenRenderer& operator=(ScreenRenderer&) = delete;
		ScreenRenderer(ScreenRenderer&) = delete;

		void Unload();
		bool IsLoaded() const { return isLoaded; }

		bool Load(IGLOContext&, const RenderTargetDesc&);

		// This function sets a pipeline state and then issues a draw command.
		// You are required to set a rendertarget, scissor rect, and viewport before calling this function.
		void DrawFullscreenQuad(CommandList&, const Texture& source, const Texture& destTarget,
			ScreenRendererBlend blend = ScreenRendererBlend::BlendDisabled);

	private:
		bool isLoaded = false;
		const IGLOContext* context = nullptr;
		Pipeline pipeline[3]; // [blend]
		Sampler samplerPoint;
		Sampler samplerAnisotropicClamp;

	};

}