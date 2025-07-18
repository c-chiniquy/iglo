
#pragma once

namespace ig
{

	// Attempting to draw a depthbuffer stencil with BatchRenderer the usual way will fail.
	// This is because the shader requries a stencil SRV to access the stencil component.
	// This class creates a stencil SRV (wrapped in a Texture class)
	// allowing you to draw the stencil component of any depthbuffer using BatchRenderer.
	class StencilTextureView
	{
	public:
		StencilTextureView() = default;
		~StencilTextureView() { Unload(); }

		StencilTextureView& operator=(StencilTextureView&) = delete;
		StencilTextureView(StencilTextureView&) = delete;

		void Unload();
		bool IsLoaded() const { return wrappedTexture.IsLoaded(); }

		bool Load(const IGLOContext&, const Texture& depthBufferWithStencil);

		const Texture& GetWrappedTexture() const { return wrappedTexture; }

	private:
		const IGLOContext* context = nullptr;
		Texture wrappedTexture;
		Descriptor stencilDescriptor;

#ifdef IGLO_VULKAN
		VkImageView stencilImageView = VK_NULL_HANDLE;
#endif
	};

}