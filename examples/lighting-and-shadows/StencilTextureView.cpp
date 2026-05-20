
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

	std::unique_ptr<StencilTextureView> StencilTextureView::Create(const IGLOContext& context, const Texture& depthStencil)
	{
		const char* errStr = "Failed to create stencil texture view. Reason: ";

		bool isValid = GetFormatInfo(depthStencil.GetFormat()).hasStencilComponent;
		if (!isValid)
		{
			Log(LogType::Error, ToString(errStr, "Invalid depth buffer texture."));
			return nullptr;
		}

		DescriptorHeap& heap = context.GetDescriptorHeap();
		std::unique_ptr<StencilTextureView> out = std::unique_ptr<StencilTextureView>(new StencilTextureView(context));
		out->stencilDescriptor = heap.AllocatePersistent(DescriptorType::Resource);

		Impl_Texture impl;
		impl.srv = out->stencilDescriptor;

#ifdef IGLO_D3D12
		{
			auto device = context.GetD3D12Device();

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = Texture::GenerateD3D12Desc_SRV_AllMips(Format::None,
				depthStencil.GetMSAA(), depthStencil.GetMipLevels(), depthStencil.GetNumFaces(), false);

			// Use a format that selects the stencil component
			const FormatInfoDXGI formatInfoDXGI = GetFormatInfoDXGI(depthStencil.GetFormat());
			if (formatInfoDXGI.dxgiFormat == DXGI_FORMAT_D24_UNORM_S8_UINT)
			{
				srvDesc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
			}
			else
			{
				srvDesc.Format = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
			}

			// Stencil lives in plane 1 (depth is plane 0)
			switch (srvDesc.ViewDimension)
			{
			case D3D12_SRV_DIMENSION_TEXTURE2D:
				srvDesc.Texture2D.PlaneSlice = 1;
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
				srvDesc.Texture2DArray.PlaneSlice = 1;
				break;

			default:
				break;
			}

			device->CreateShaderResourceView(depthStencil.GetD3D12Resource(), &srvDesc, heap.GetD3D12CPUHandle(out->stencilDescriptor));
		}
#endif
#ifdef IGLO_VULKAN
		{
			VkDevice device = context.GetVulkanDevice();

			out->stencilImageView = Texture::CreateImageView(device, depthStencil.GetVulkanImage(), VK_IMAGE_ASPECT_STENCIL_BIT,
				depthStencil.GetFormat(), false, depthStencil.GetNumFaces());
			if (!out->stencilImageView)
			{
				Log(LogType::Error, ToString(errStr, "Failed to create image view."));
				return nullptr;
			}
			heap.WriteImageDescriptor(out->stencilDescriptor, VulkanDescriptorType::Texture_SRV,
				out->stencilImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
#endif

		out->wrappedTexture = Texture::CreateWrapped(context, depthStencil.GetDesc(), impl);
		if (!out->wrappedTexture)
		{
			Log(LogType::Error, ToString(errStr, "Failed to create wrapped texture."));
			return nullptr;
		}

		return out;
	}

}