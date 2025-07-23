
#include "iglo.h"
#include "StencilTextureView.h"

namespace ig
{

	void StencilTextureView::Unload()
	{
#ifdef IGLO_VULKAN
		if (stencilImageView) vkDestroyImageView(context->GetVulkanDevice(), stencilImageView, nullptr);
		stencilImageView = VK_NULL_HANDLE;
#endif

		if (!stencilDescriptor.IsNull()) context->GetDescriptorHeap().FreePersistent(stencilDescriptor);
		stencilDescriptor.SetToNull();

		wrappedTexture.Unload();

		context = nullptr;
	}

	bool StencilTextureView::Load(const IGLOContext& context, const Texture& depthBufferWithStencil)
	{
		Unload();

		const char* errStr = "Failed to load stencil texture view. Reason: ";

		this->context = &context;

		bool isValid = false;
		if (depthBufferWithStencil.IsLoaded())
		{
			if (GetFormatInfo(depthBufferWithStencil.GetFormat()).hasStencilComponent)
			{
				isValid = true;
			}
		}

		if (!isValid)
		{
			Log(LogType::Error, ToString(errStr, "Invalid depth buffer texture."));
			Unload();
			return false;
		}

		this->stencilDescriptor = context.GetDescriptorHeap().AllocatePersistent(DescriptorType::Texture_SRV);
		if (this->stencilDescriptor.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate descriptor."));
			Unload();
			return false;
		}

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
				heap.GetD3D12CPUHandle(this->stencilDescriptor));

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
			viewInfo.format = ConvertToVulkanFormat(depthBufferWithStencil.GetFormat());
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;
			viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &this->stencilImageView);
			if (result != VK_SUCCESS)
			{
				Log(LogType::Error, ToString(errStr, "Failed to create image view."));
				Unload();
				return false; 
			}

			heap.WriteImageDescriptor(this->stencilDescriptor, this->stencilImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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
		desc.srvDescriptor = this->stencilDescriptor;
		desc.impl = impl;
		return wrappedTexture.LoadAsWrapped(context, desc);
	}

}