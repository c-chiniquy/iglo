
#include "iglo.h"
#include "StencilTextureView.h"

namespace ig
{

	StencilTextureView::~StencilTextureView()
	{
#ifdef IGLO_VULKAN
		if (stencilImageView) vkDestroyImageView(context.GetVulkanDevice(), stencilImageView, nullptr);
		stencilImageView = VK_NULL_HANDLE;
#endif

		if (stencilDescriptor) context.GetDescriptorHeap().FreePersistent(stencilDescriptor);
		stencilDescriptor.SetToNull();

		wrappedTexture = nullptr;
	}

	std::unique_ptr<StencilTextureView> StencilTextureView::Create(const IGLOContext& context, const Texture& depthBufferWithStencil)
	{
		const char* errStr = "Failed to create stencil texture view. Reason: ";

		bool isValid = GetFormatInfo(depthBufferWithStencil.GetFormat()).hasStencilComponent;
		if (!isValid)
		{
			Log(LogType::Error, ToString(errStr, "Invalid depth buffer texture."));
			return nullptr;
		}

		std::unique_ptr<StencilTextureView> out = std::unique_ptr<StencilTextureView>(new StencilTextureView(context));

		out->stencilDescriptor = context.GetDescriptorHeap().AllocatePersistent(DescriptorType::Texture_SRV);

		Impl_Texture impl;

#ifdef IGLO_D3D12
		{
			auto device = context.GetD3D12Device();
			DescriptorHeap& heap = context.GetDescriptorHeap();

			D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
			srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			if (depthBufferWithStencil.GetMSAA() == MSAA::Disabled)
			{
				srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srv.Texture2D.MipLevels = 1;
				srv.Texture2D.PlaneSlice = 1;
			}
			else
			{
				srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
			}
			FormatInfoDXGI formatInfoDXGI = GetFormatInfoDXGI(depthBufferWithStencil.GetFormat());
			if (formatInfoDXGI.dxgiFormat == DXGI_FORMAT_D24_UNORM_S8_UINT)
			{
				srv.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
			}
			else
			{
				srv.Format = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
			}

			device->CreateShaderResourceView(depthBufferWithStencil.GetD3D12Resource(), &srv,
				heap.GetD3D12CPUHandle(out->stencilDescriptor));

			impl.resource = { nullptr }; // This vector must have a size of 1
		}
#endif
#ifdef IGLO_VULKAN
		{
			VkDevice device = context.GetVulkanDevice();
			DescriptorHeap& heap = context.GetDescriptorHeap();

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = depthBufferWithStencil.GetVulkanImage();
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = ToVulkanFormat(depthBufferWithStencil.GetFormat());
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;
			viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &out->stencilImageView);
			if (result != VK_SUCCESS)
			{
				Log(LogType::Error, ToString(errStr, "Failed to create image view."));
				return nullptr;
			}

			heap.WriteImageDescriptor(out->stencilDescriptor, out->stencilImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			// These vectors must have a size of 1
			impl.image = { VK_NULL_HANDLE };
			impl.memory = { VK_NULL_HANDLE };
		}
#endif

		WrappedTextureDesc desc;
		desc.textureDesc.extent = depthBufferWithStencil.GetExtent();
		desc.textureDesc.format = depthBufferWithStencil.GetFormat();
		desc.textureDesc.msaa = depthBufferWithStencil.GetMSAA();
		desc.textureDesc.usage = depthBufferWithStencil.GetUsage();
		desc.srvDescriptor = out->stencilDescriptor;
		desc.impl = impl;

		out->wrappedTexture = Texture::CreateWrapped(context, desc);
		if (!out->wrappedTexture)
		{
			Log(LogType::Error, ToString(errStr, "Failed to create wrapped texture."));
			return nullptr;
		}

		return out;
	}

}