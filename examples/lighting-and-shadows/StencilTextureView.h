
#pragma once

namespace ig
{

	// Attempting to draw a depthbuffer stencil with BatchRenderer the usual way will fail.
	// This is because the shader requries a special stencil SRV to access the stencil component.
	// This class creates a stencil SRV (wrapped in a Texture class)
	// allowing you to draw the stencil component of any depthbuffer using BatchRenderer.
	class StencilTextureView
	{
	private:
		StencilTextureView(const IGLOContext& context) :context(context) {};

		StencilTextureView& operator=(const StencilTextureView&) = delete;
		StencilTextureView(const StencilTextureView&) = delete;

	public:
		~StencilTextureView();

		static std::unique_ptr<StencilTextureView> Create(const IGLOContext&, const Texture& depthBufferWithStencil);

		const Texture& GetWrappedTexture() const { return *wrappedTexture; }

	private:
		const IGLOContext& context;
		std::unique_ptr<Texture> wrappedTexture;
		Descriptor stencilDescriptor;

#ifdef IGLO_VULKAN
		VkImageView stencilImageView = VK_NULL_HANDLE;
#endif
	};

}