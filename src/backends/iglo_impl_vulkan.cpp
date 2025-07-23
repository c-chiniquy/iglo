
#include "iglo.h"
#include <set>
#include <unordered_map>


#ifdef IGLO_VULKAN

#include <vulkan/vulkan.hpp>

constexpr uint32_t vulkanVersion = VK_API_VERSION_1_3;

namespace ig
{
	std::string CreateVulkanErrorMsg(const char* functionName, VkResult result)
	{
		return ToString(functionName, " returned result: ", vk::to_string(vk::Result(result)), ".");
	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func)
		{
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func) func(instance, debugMessenger, pAllocator);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			Print(ToString("Vulkan: ", pCallbackData->pMessage, "\n"));
		}

		return VK_FALSE;
	}

	std::optional<uint32_t> FindVulkanMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProps;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

		for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		return std::nullopt;
	}

	DetailedResult IsVulkanPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
		const std::vector<const char*>& requiredDeviceExtensions)
	{
		// Queue families
		{
			VulkanQueueFamilies fams = GetVulkanQueueFamilies(physicalDevice, surface);

			// Graphics queue
			if (!fams.graphicsFamily)
			{
				return DetailedResult::MakeFail("The device lacks a graphics queue family.");
			}

			// Present queue
			if (!fams.presentFamily)
			{
				return DetailedResult::MakeFail("The device lacks a queue family that can present images to the screen.");
			}
		}

		// Properties
		{
			// Vulkan Version
			VkPhysicalDeviceProperties props = {};
			vkGetPhysicalDeviceProperties(physicalDevice, &props);
			if (props.apiVersion < vulkanVersion)
			{
				std::string vulkanVersionString = ToString(VK_API_VERSION_MAJOR(vulkanVersion), ".", VK_API_VERSION_MINOR(vulkanVersion));
				return DetailedResult::MakeFail(ToString(
					"Vulkan ", vulkanVersionString, " is not supported on this device."));
			}
		}

		// Required device extensions
		{
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
			std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());
			for (const auto& extension : availableExtensions)
			{
				requiredExtensions.erase(extension.extensionName);
			}
			if (!requiredExtensions.empty())
			{
				std::string errStr = "Unsupported device extensions:";
				for (std::string s : requiredExtensions)
				{
					errStr.append("\n");
					errStr.append(s);
				}
				return DetailedResult::MakeFail(errStr);
			}
		}

		// Features
		{
			VkPhysicalDeviceLineRasterizationFeaturesKHR lineFeats = {};
			lineFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_KHR;

			VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR dynamicLocalReadFeats = {};
			dynamicLocalReadFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_LOCAL_READ_FEATURES_KHR;
			dynamicLocalReadFeats.pNext = &lineFeats;

			VkPhysicalDeviceMaintenance5FeaturesKHR maintenance5Feats = {};
			maintenance5Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR;
			maintenance5Feats.pNext = &dynamicLocalReadFeats;

			VkPhysicalDeviceCustomBorderColorFeaturesEXT borderColorFeats = {};
			borderColorFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT;
			borderColorFeats.pNext = &maintenance5Feats;

			VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableFeats = {};
			mutableFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
			mutableFeats.pNext = &borderColorFeats;

			VkPhysicalDeviceVulkan13Features feats13 = {};
			feats13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
			feats13.pNext = &mutableFeats;

			VkPhysicalDeviceVulkan12Features feats12 = {};
			feats12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
			feats12.pNext = &feats13;

			VkPhysicalDeviceFeatures2 feats2 = {};
			feats2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			feats2.pNext = &feats12;
			vkGetPhysicalDeviceFeatures2(physicalDevice, &feats2);

			std::vector<const char*> notSupported;

			auto Check = [&](bool b, const char* featureName)
			{
				if (!b) notSupported.push_back(featureName);
			};

			Check(feats12.runtimeDescriptorArray &&
				feats12.descriptorBindingPartiallyBound &&
				feats12.descriptorBindingVariableDescriptorCount &&
				feats12.descriptorBindingUniformBufferUpdateAfterBind &&
				feats12.descriptorBindingStorageBufferUpdateAfterBind &&
				feats12.descriptorBindingSampledImageUpdateAfterBind &&
				feats12.descriptorBindingStorageImageUpdateAfterBind,
				"Bindless rendering");

			Check(lineFeats.rectangularLines &&
				lineFeats.bresenhamLines &&
				lineFeats.smoothLines &&
				lineFeats.stippledRectangularLines &&
				lineFeats.stippledBresenhamLines &&
				lineFeats.stippledSmoothLines, "LineRasterizationFeatures");

			Check(borderColorFeats.customBorderColors, "customBorderColors");
			Check(borderColorFeats.customBorderColorWithoutFormat, "customBorderColorWithoutFormat");
			Check(mutableFeats.mutableDescriptorType, "mutableDescriptorType");
			Check(feats2.features.independentBlend, "independentBlend");
			Check(feats2.features.samplerAnisotropy, "samplerAnisotropy");
			Check(feats12.timelineSemaphore, "timelineSemaphore");
			Check(feats13.dynamicRendering, "dynamicRendering");
			Check(feats13.synchronization2, "synchronization2");
			Check(maintenance5Feats.maintenance5, "maintenance5");
			Check(dynamicLocalReadFeats.dynamicRenderingLocalRead, "dynamicRenderingLocalRead");

			if (notSupported.size() > 0)
			{
				std::string str = "These features are not supported on this device: ";
				for (size_t i = 0; i < notSupported.size(); i++)
				{
					str.append(notSupported[i]);
					if (i != notSupported.size() - 1) str.append(", ");
				}
				str.append(".");
				return DetailedResult::MakeFail(str);
			}

		}

		// Swapchain support
		{
			// We use KHR extensions to retreive swap chain info.
			// We already know KHR extensions are supported at this point.
			VulkanSwapChainInfo swapChainInfo = GetVulkanSwapChainInfo(physicalDevice, surface);
			if (swapChainInfo.formats.empty() || swapChainInfo.presentModes.empty())
			{
				return DetailedResult::MakeFail("The swapchain for this device lacks the required features.");
			}
		}

		return DetailedResult::MakeSuccess();
	}

	VulkanQueueFamilies GetVulkanQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		uint32_t numQueueFams = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFams, nullptr);
		std::vector<VkQueueFamilyProperties> queueFams(numQueueFams);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFams, queueFams.data());
		VulkanQueueFamilies out;
		for (uint32_t i = 0; i < queueFams.size(); i++)
		{
			const auto& flags = queueFams[i].queueFlags;

			bool graphicsBit = (flags & VK_QUEUE_GRAPHICS_BIT);
			bool computeBit = (flags & VK_QUEUE_COMPUTE_BIT);
			bool transferBit = (flags & VK_QUEUE_TRANSFER_BIT);

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

			// Queues that appear first in the list have priority.

			if (presentSupport)
			{
				if (!out.presentFamily) out.presentFamily = i;
			}
			if (transferBit && !computeBit && !graphicsBit)
			{
				if (!out.transferFamily) out.transferFamily = i;
			}
			else if (computeBit && !graphicsBit)
			{
				if (!out.computeFamily) out.computeFamily = i;
			}
			else if (graphicsBit)
			{
				if (!out.graphicsFamily) out.graphicsFamily = i;
			}
		}
		return out;
	}

	VulkanSwapChainInfo GetVulkanSwapChainInfo(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		VulkanSwapChainInfo out;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &out.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			out.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, out.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			out.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, out.presentModes.data());
		}

		return out;
	}

	VkFormat ConvertToVulkanFormat(Format format)
	{
		switch (format)
		{
		case Format::FLOAT_FLOAT_FLOAT_FLOAT:			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case Format::FLOAT_FLOAT_FLOAT:					return VK_FORMAT_R32G32B32_SFLOAT;
		case Format::FLOAT_FLOAT:						return VK_FORMAT_R32G32_SFLOAT;
		case Format::FLOAT:								return VK_FORMAT_R32_SFLOAT;
		case Format::FLOAT16_FLOAT16_FLOAT16_FLOAT16:	return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Format::FLOAT16_FLOAT16:					return VK_FORMAT_R16G16_SFLOAT;
		case Format::FLOAT16:							return VK_FORMAT_R16_SFLOAT;
		case Format::BYTE_BYTE_BYTE_BYTE:				return VK_FORMAT_R8G8B8A8_UNORM;
		case Format::BYTE_BYTE:							return VK_FORMAT_R8G8_UNORM;
		case Format::BYTE:								return VK_FORMAT_R8_UNORM;
		case Format::INT8_INT8_INT8_INT8:				return VK_FORMAT_R8G8B8A8_SNORM;
		case Format::INT8_INT8:							return VK_FORMAT_R8G8_SNORM;
		case Format::INT8:								return VK_FORMAT_R8_SNORM;
		case Format::UINT16_UINT16_UINT16_UINT16:		return VK_FORMAT_R16G16B16A16_UNORM;
		case Format::UINT16_UINT16:						return VK_FORMAT_R16G16_UNORM;
		case Format::UINT16:							return VK_FORMAT_R16_UNORM;
		case Format::INT16_INT16_INT16_INT16:			return VK_FORMAT_R16G16B16A16_SNORM;
		case Format::INT16_INT16:						return VK_FORMAT_R16G16_SNORM;
		case Format::INT16:								return VK_FORMAT_R16_SNORM;
		case Format::UINT10_UINT10_UINT10_UINT2:		return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		case Format::UINT10_UINT10_UINT10_UINT2_NotNormalized: return VK_FORMAT_A2R10G10B10_UINT_PACK32;
		case Format::UFLOAT11_UFLOAT11_UFLOAT10:		return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case Format::UFLOAT9_UFLOAT9_UFLOAT9_UFLOAT5:	return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		case Format::BYTE_BYTE_BYTE_BYTE_sRGB:			return VK_FORMAT_R8G8B8A8_SRGB;
		case Format::BYTE_BYTE_BYTE_BYTE_BGRA_sRGB:		return VK_FORMAT_B8G8R8A8_SRGB;
		case Format::BYTE_BYTE_BYTE_BYTE_BGRA:			return VK_FORMAT_B8G8R8A8_UNORM;
		case Format::BYTE_BYTE_BYTE_BYTE_NotNormalized: return VK_FORMAT_R8G8B8A8_UINT;
		case Format::BYTE_BYTE_NotNormalized:			return VK_FORMAT_R8G8_UINT;
		case Format::BYTE_NotNormalized:				return VK_FORMAT_R8_UINT;
		case Format::BC1:				return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case Format::BC1_sRGB:			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case Format::BC2:				return VK_FORMAT_BC2_UNORM_BLOCK;
		case Format::BC2_sRGB:			return VK_FORMAT_BC2_SRGB_BLOCK;
		case Format::BC3:				return VK_FORMAT_BC3_UNORM_BLOCK;
		case Format::BC3_sRGB:			return VK_FORMAT_BC3_SRGB_BLOCK;
		case Format::BC4_UNSIGNED:		return VK_FORMAT_BC4_UNORM_BLOCK;
		case Format::BC4_SIGNED:		return VK_FORMAT_BC4_SNORM_BLOCK;
		case Format::BC5_UNSIGNED:		return VK_FORMAT_BC5_UNORM_BLOCK;
		case Format::BC5_SIGNED:		return VK_FORMAT_BC5_SNORM_BLOCK;
		case Format::BC6H_UFLOAT16:		return VK_FORMAT_BC6H_UFLOAT_BLOCK;
		case Format::BC6H_SFLOAT16:		return VK_FORMAT_BC6H_SFLOAT_BLOCK;
		case Format::BC7:				return VK_FORMAT_BC7_UNORM_BLOCK;
		case Format::BC7_sRGB:			return VK_FORMAT_BC7_SRGB_BLOCK;
		case Format::INT8_INT8_INT8_INT8_NotNormalized:			return VK_FORMAT_R8G8B8A8_SINT;
		case Format::INT8_INT8_NotNormalized:					return VK_FORMAT_R8G8_SINT;
		case Format::INT8_NotNormalized:						return VK_FORMAT_R8_SINT;
		case Format::UINT16_UINT16_UINT16_UINT16_NotNormalized:	return VK_FORMAT_R16G16B16A16_UINT;
		case Format::UINT16_UINT16_NotNormalized:				return VK_FORMAT_R16G16_UINT;
		case Format::UINT16_NotNormalized:						return VK_FORMAT_R16_UINT;
		case Format::INT16_INT16_INT16_INT16_NotNormalized:		return VK_FORMAT_R16G16B16A16_SINT;
		case Format::INT16_INT16_NotNormalized:					return VK_FORMAT_R16G16_SINT;
		case Format::INT16_NotNormalized:						return VK_FORMAT_R16_SINT;
		case Format::INT32_INT32_INT32_INT32_NotNormalized:		return VK_FORMAT_R32G32B32A32_SINT;
		case Format::INT32_INT32_INT32_NotNormalized:			return VK_FORMAT_R32G32B32_SINT;
		case Format::INT32_INT32_NotNormalized:					return VK_FORMAT_R32G32_SINT;
		case Format::INT32_NotNormalized:						return VK_FORMAT_R32_SINT;
		case Format::UINT32_UINT32_UINT32_UINT32_NotNormalized:	return VK_FORMAT_R32G32B32A32_UINT;
		case Format::UINT32_UINT32_UINT32_NotNormalized:		return VK_FORMAT_R32G32B32_UINT;
		case Format::UINT32_UINT32_NotNormalized:				return VK_FORMAT_R32G32_UINT;
		case Format::UINT32_NotNormalized:						return VK_FORMAT_R32_UINT;
		case Format::DEPTHFORMAT_FLOAT:							return VK_FORMAT_D32_SFLOAT;
		case Format::DEPTHFORMAT_UINT16:						return VK_FORMAT_D16_UNORM;
		case Format::DEPTHFORMAT_UINT24_BYTE:					return VK_FORMAT_D24_UNORM_S8_UINT;
		case Format::DEPTHFORMAT_FLOAT_BYTE:					return VK_FORMAT_D32_SFLOAT_S8_UINT;

		default:
			return VK_FORMAT_UNDEFINED;
		}

	}

	VulkanVertexLayout ConvertToVulkanVertexLayout(const std::vector<VertexElement>& elems)
	{
		VulkanVertexLayout layout;
		layout.attributes.reserve(elems.size());
		std::unordered_map<uint32_t, VkVertexInputBindingDescription> bindings;
		std::array<uint32_t, 32> offsetTracker = {};

		for (size_t i = 0; i < elems.size(); i++)
		{
			const VertexElement& element = elems[i];
			const uint32_t slot = element.inputSlot;

			if ((size_t)slot >= offsetTracker.size()) throw std::invalid_argument("inputSlot too high.");

			FormatInfo formatInfo = GetFormatInfo(element.format);

			VkVertexInputAttributeDescription attr = {};
			attr.binding = slot;
			attr.location = (uint32_t)i;
			attr.format = ConvertToVulkanFormat(element.format);
			attr.offset = offsetTracker[slot];
			layout.attributes.push_back(attr);

			offsetTracker[slot] += formatInfo.bytesPerPixel;

			if (bindings.find(slot) == bindings.end())
			{
				VkVertexInputBindingDescription bind = {};
				bind.binding = slot;
				bind.inputRate = (element.inputSlotClass == InputClass::PerInstance)
					? VK_VERTEX_INPUT_RATE_INSTANCE
					: VK_VERTEX_INPUT_RATE_VERTEX;
				bind.stride = 0; // Set later
				bindings[slot] = bind;
			}
		}

		// Set final strides
		for (auto& pair : bindings)
		{
			pair.second.stride = offsetTracker[pair.first];
			layout.bindings.push_back(pair.second);
		}

		return layout;
	}

	VulkanTextureFilter ConvertToVulkanTextureFilter(TextureFilter filter)
	{
		VulkanTextureFilter out;

		switch (filter)
		{
		case TextureFilter::_Comparison_Point:
		case TextureFilter::_Comparison_Bilinear:
		case TextureFilter::_Comparison_Trilinear:
		case TextureFilter::_Comparison_AnisotropicX2:
		case TextureFilter::_Comparison_AnisotropicX4:
		case TextureFilter::_Comparison_AnisotropicX8:
		case TextureFilter::_Comparison_AnisotropicX16:
			out.compareEnable = VK_TRUE;
			break;

		default:
			out.compareEnable = VK_FALSE;
			break;
		}

		switch (filter)
		{
		case TextureFilter::Point:
		case TextureFilter::_Comparison_Point:
		case TextureFilter::_Minimum_Point:
		case TextureFilter::_Maximum_Point:
			out.magFilter = VK_FILTER_NEAREST;
			out.minFilter = VK_FILTER_NEAREST;
			out.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			break;

		case TextureFilter::Bilinear:
		case TextureFilter::_Comparison_Bilinear:
		case TextureFilter::_Minimum_Bilinear:
		case TextureFilter::_Maximum_Bilinear:
			out.magFilter = VK_FILTER_LINEAR;
			out.minFilter = VK_FILTER_LINEAR;
			out.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			break;

		case TextureFilter::Trilinear:
		case TextureFilter::_Comparison_Trilinear:
		case TextureFilter::_Minimum_Trilinear:
		case TextureFilter::_Maximum_Trilinear:
			out.magFilter = VK_FILTER_LINEAR;
			out.minFilter = VK_FILTER_LINEAR;
			out.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			break;

			// Anisotropic
		default:
			out.magFilter = VK_FILTER_LINEAR;
			out.minFilter = VK_FILTER_LINEAR;
			out.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			out.anisotropyEnable = VK_TRUE;

			switch (filter)
			{
			case TextureFilter::AnisotropicX2:
			case TextureFilter::_Comparison_AnisotropicX2:
			case TextureFilter::_Minimum_AnisotropicX2:
			case TextureFilter::_Maximum_AnisotropicX2:
				out.maxAnisotropy = 2.0f;
				break;

			case TextureFilter::AnisotropicX4:
			case TextureFilter::_Comparison_AnisotropicX4:
			case TextureFilter::_Minimum_AnisotropicX4:
			case TextureFilter::_Maximum_AnisotropicX4:
				out.maxAnisotropy = 4.0f;
				break;

			case TextureFilter::AnisotropicX8:
			case TextureFilter::_Comparison_AnisotropicX8:
			case TextureFilter::_Minimum_AnisotropicX8:
			case TextureFilter::_Maximum_AnisotropicX8:
				out.maxAnisotropy = 8.0f;
				break;

			case TextureFilter::AnisotropicX16:
			case TextureFilter::_Comparison_AnisotropicX16:
			case TextureFilter::_Minimum_AnisotropicX16:
			case TextureFilter::_Maximum_AnisotropicX16:
				out.maxAnisotropy = 16.0f;
				break;

			default:
				throw std::invalid_argument("Invalid texture filter.");

			}

			break;
		}

		return out;
	}

	uint64_t GetRequiredUploadBufferSize(const Image& image, const BufferPlacementAlignments& alignments)
	{
		uint64_t size = 0;
		for (uint32_t face = 0; face < image.GetNumFaces(); face++)
		{
			for (uint32_t mip = 0; mip < image.GetMipLevels(); mip++)
			{
				uint64_t srcRowPitch = image.GetMipRowPitch(mip);
				uint64_t destRowPitch = AlignUp(srcRowPitch, alignments.textureRowPitch);
				for (uint64_t srcProgress = 0; srcProgress < (uint64_t)image.GetMipSize(mip); srcProgress += srcRowPitch)
				{
					size += destRowPitch;
#ifdef _DEBUG
					if (srcProgress + srcRowPitch > (uint64_t)image.GetMipSize(mip)) throw std::runtime_error("Something is wrong here.");
#endif
				}
			}
		}
		return size;
	}

	VkResult CreateVulkanImageViewForSwapChain(VkDevice device, VkImage swapChainImage, VkFormat swapChainFormat, VkImageView* out_pView)
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = swapChainImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = swapChainFormat;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		return vkCreateImageView(device, &viewInfo, nullptr, out_pView);
	}

	VkImageAspectFlags GetVulkanImageAspect(Format format)
	{
		FormatInfo formatInfo = GetFormatInfo(format);
		VkImageAspectFlags aspect = formatInfo.isDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		if (formatInfo.hasStencilComponent) aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
		return aspect;
	}

	void DescriptorHeap::Impl_Unload()
	{
		for (auto& tempViews : impl.tempImageViews)
		{
			for (VkImageView imageView : tempViews)
			{
				if (imageView) vkDestroyImageView(context->GetVulkanDevice(), imageView, nullptr);
			}
		}
		impl.tempImageViews.clear();
		impl.tempImageViews.shrink_to_fit();

		if (impl.descriptorPool)
		{
			vkDestroyDescriptorPool(context->GetVulkanDevice(), impl.descriptorPool, nullptr);
			impl.descriptorPool = VK_NULL_HANDLE;
		}
		if (impl.descriptorSetLayout)
		{
			vkDestroyDescriptorSetLayout(context->GetVulkanDevice(), impl.descriptorSetLayout, nullptr);
			impl.descriptorSetLayout = VK_NULL_HANDLE;
		}
		if (impl.bindlessPipelineLayout)
		{
			vkDestroyPipelineLayout(context->GetVulkanDevice(), impl.bindlessPipelineLayout, nullptr);
			impl.bindlessPipelineLayout = VK_NULL_HANDLE;
		}
		impl.descriptorSet = VK_NULL_HANDLE;
	}

	DetailedResult DescriptorHeap::Impl_Load(const IGLOContext& context, uint32_t maxPersistentResources,
		uint32_t maxTempResourcesPerFrame, uint32_t maxSamplers, uint32_t numFramesInFlight)
	{
		VkDevice device = context.GetVulkanDevice();
		uint32_t totalResDescriptors = CalcTotalResDescriptors(maxPersistentResources, maxTempResourcesPerFrame, numFramesInFlight);

		impl.tempImageViews.resize(numFramesInFlight, {});

		std::array<uint32_t, NUM_DESCRIPTOR_TYPES> propMax = GetVulkanPropsMaxDescriptors(context.GetVulkanPhysicalDevice());

		// Check max descriptor count
		for (uint32_t i = 0; i < NUM_DESCRIPTOR_TYPES; i++)
		{
			uint32_t totalAlloc = (i == (uint32_t)DescriptorType::Sampler) ? maxSamplers : totalResDescriptors;
			if (totalAlloc > propMax[i])
			{
				/*
					If the GPU supports bindless, it's very likely you can allocate atleast 500k descriptors of any type.

					Max values for GTX 1000-series NVIDIA graphics cards (doesn't support bindless):
					maxPerStageDescriptorUpdateAfterBindUniformBuffers = 15
					maxDescriptorSetUpdateAfterBindUniformBuffers = 90

					Max values for RTX 2060 (supports bindless):
					maxPerStageDescriptorUpdateAfterBindUniformBuffers = 1048576
					maxDescriptorSetUpdateAfterBindUniformBuffers = 1048576

				*/

				std::string descriptorName = vk::to_string((vk::DescriptorType)vkDescriptorTypeTable[i]);
				return DetailedResult::MakeFail(ToString(
					"Failed to allocate bindless descriptors:\n"
					"  - Requested: ", totalAlloc, " ", descriptorName, " descriptors\n"
					"  - Supported: ", propMax[i], " ", descriptorName, " descriptors\n"
					"This device does not support the requested number of descriptors."));
			}
		}

		// Create descriptor set layout with two bindings
		{
			std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};

			// Binding 0: Resources
			bindings[0].binding = 0;
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_MUTABLE_EXT;
			bindings[0].descriptorCount = totalResDescriptors;
			bindings[0].stageFlags = VK_SHADER_STAGE_ALL;

			// Binding 1: Samplers
			bindings[1].binding = 1;
			bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			bindings[1].descriptorCount = maxSamplers;
			bindings[1].stageFlags = VK_SHADER_STAGE_ALL;

			std::array<VkDescriptorBindingFlags, 2> bindingFlags = {};
			bindingFlags[0] =
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
				VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
			bindingFlags[1] =
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
				VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

			VkMutableDescriptorTypeListEXT typeList = {};
			typeList.descriptorTypeCount = 4; // The first 4 elements in the table are resource descriptors which we want mutable.
			typeList.pDescriptorTypes = vkDescriptorTypeTable;

			VkMutableDescriptorTypeCreateInfoEXT mutableInfo = {};
			mutableInfo.sType = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT;
			mutableInfo.mutableDescriptorTypeListCount = 1;
			mutableInfo.pMutableDescriptorTypeLists = &typeList;

			VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo = {};
			flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			flagsInfo.pNext = &mutableInfo;
			flagsInfo.bindingCount = (uint32_t)bindingFlags.size();
			flagsInfo.pBindingFlags = bindingFlags.data();

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.pNext = &flagsInfo;
			layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
			layoutInfo.bindingCount = (uint32_t)bindings.size();
			layoutInfo.pBindings = bindings.data();

			VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &impl.descriptorSetLayout);
			if (result != VK_SUCCESS)
			{
				return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreateDescriptorSetLayout", result));
			}
		}

		// Create descriptor pool
		{
			std::array<VkDescriptorPoolSize, 2> poolSizes = {};
			poolSizes[0] = { VK_DESCRIPTOR_TYPE_MUTABLE_EXT, totalResDescriptors };
			poolSizes[1] = { VK_DESCRIPTOR_TYPE_SAMPLER, maxSamplers };

			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
			poolInfo.maxSets = 1;
			poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
			poolInfo.pPoolSizes = poolSizes.data();

			VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &impl.descriptorPool);
			if (result != VK_SUCCESS)
			{
				return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreateDescriptorPool", result));
			}
		}

		// Allocate descriptor set
		{
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = impl.descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &impl.descriptorSetLayout;

			VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &impl.descriptorSet);
			if (result != VK_SUCCESS)
			{
				return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkAllocateDescriptorSets", result));
			}
		}

		// Create bindless pipeline layout
		{
			VkPushConstantRange pushConstantRange = {};
			pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL;
			pushConstantRange.offset = 0;
			pushConstantRange.size = MAX_PUSH_CONSTANTS_BYTE_SIZE;

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &impl.descriptorSetLayout;
			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

			VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &impl.bindlessPipelineLayout);
			if (result != VK_SUCCESS)
			{
				return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreatePipelineLayout", result));
			}
		}

		return DetailedResult::MakeSuccess();
	}

	void DescriptorHeap::Impl_NextFrame()
	{
		assert(frameIndex < impl.tempImageViews.size());
		auto& tempViews = impl.tempImageViews[frameIndex];
		for (VkImageView imageView : tempViews)
		{
			if (imageView) vkDestroyImageView(context->GetVulkanDevice(), imageView, nullptr);
		}
		tempViews.clear();
	}

	void DescriptorHeap::Impl_FreeAllTempResources()
	{
		for (auto& tempViews : impl.tempImageViews)
		{
			for (VkImageView imageView : tempViews)
			{
				if (imageView) vkDestroyImageView(context->GetVulkanDevice(), imageView, nullptr);
			}
			tempViews.clear();
		}
	}

	std::array<uint32_t, NUM_DESCRIPTOR_TYPES> DescriptorHeap::GetVulkanPropsMaxDescriptors(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties2 props2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		VkPhysicalDeviceDescriptorIndexingProperties indexingProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES };
		props2.pNext = &indexingProps;
		vkGetPhysicalDeviceProperties2(device, &props2);

		std::array<uint32_t, NUM_DESCRIPTOR_TYPES> out =
		{
			indexingProps.maxPerStageDescriptorUpdateAfterBindUniformBuffers,
			indexingProps.maxPerStageDescriptorUpdateAfterBindStorageBuffers,
			indexingProps.maxPerStageDescriptorUpdateAfterBindSampledImages,
			indexingProps.maxPerStageDescriptorUpdateAfterBindStorageImages,
			indexingProps.maxPerStageDescriptorUpdateAfterBindSamplers
		};
		return out;
	}

	void DescriptorHeap::WriteBufferDescriptor(Descriptor descriptor, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		if (LogErrorIfNotLoaded()) return;
		if (descriptor.IsNull())
		{
			Log(LogType::Error, "Failed to write buffer descriptor. Reason: The descriptor isn't loaded.");
			return;
		}

		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = buffer;
		bufferInfo.offset = offset;
		bufferInfo.range = range;

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = impl.descriptorSet;
		write.dstBinding = vkDescriptorBindingTable[(uint32_t)descriptor.type];
		write.dstArrayElement = descriptor.heapIndex;
		write.descriptorCount = 1;
		write.descriptorType = vkDescriptorTypeTable[(uint32_t)descriptor.type];
		write.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(context->GetVulkanDevice(), 1, &write, 0, nullptr);
	}

	void DescriptorHeap::WriteImageDescriptor(Descriptor descriptor, VkImageView imageView, VkImageLayout imageLayout)
	{
		if (LogErrorIfNotLoaded()) return;
		if (descriptor.IsNull())
		{
			Log(LogType::Error, "Failed to write image descriptor. Reason: The descriptor isn't loaded.");
			return;
		}

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageView = imageView;
		imageInfo.imageLayout = imageLayout;
		imageInfo.sampler = VK_NULL_HANDLE;

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = impl.descriptorSet;
		write.dstBinding = vkDescriptorBindingTable[(uint32_t)descriptor.type];
		write.dstArrayElement = descriptor.heapIndex;
		write.descriptorType = vkDescriptorTypeTable[(uint32_t)descriptor.type];
		write.descriptorCount = 1;
		write.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(context->GetVulkanDevice(), 1, &write, 0, nullptr);
	}

	void DescriptorHeap::WriteSamplerDescriptor(Descriptor descriptor, VkSampler sampler)
	{
		if (LogErrorIfNotLoaded()) return;
		if (descriptor.IsNull())
		{
			Log(LogType::Error, "Failed to write sampler descriptor. Reason: The descriptor isn't loaded.");
			return;
		}

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.sampler = sampler;

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = impl.descriptorSet;
		write.dstBinding = vkDescriptorBindingTable[(uint32_t)descriptor.type];
		write.dstArrayElement = descriptor.heapIndex;
		write.descriptorCount = 1;
		write.descriptorType = vkDescriptorTypeTable[(uint32_t)descriptor.type];
		write.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(context->GetVulkanDevice(), 1, &write, 0, nullptr);
	}

	VkResult DescriptorHeap::CreateTempVulkanImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkImageView* pView)
	{
		if (LogErrorIfNotLoaded()) return VkResult::VK_NOT_READY;

		VkResult result = vkCreateImageView(device, pCreateInfo, pAllocator, pView);

		if (result == VK_SUCCESS)
		{
			impl.tempImageViews[frameIndex].push_back(*pView);
		}

		return result;
	}

	void Pipeline::Impl_Unload()
	{
		if (impl.pipeline)
		{
			vkDestroyPipeline(context->GetVulkanDevice(), impl.pipeline, nullptr);
			impl.pipeline = VK_NULL_HANDLE;
		}
	}

	DetailedResult Pipeline::Impl_Load(const IGLOContext& context, const PipelineDesc& desc)
	{
		VkDevice device = context.GetVulkanDevice();

		std::array<VkPipelineShaderStageCreateInfo, 5> stages = {};
		std::array<VkShaderModuleCreateInfo, 5> shaderModules = {};
		uint32_t numShaders = 0;

		const std::pair<const Shader&, VkShaderStageFlagBits> shaderStages[] =
		{
			{ desc.VS, VK_SHADER_STAGE_VERTEX_BIT },
			{ desc.PS, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ desc.HS, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT },
			{ desc.DS, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT },
			{ desc.GS, VK_SHADER_STAGE_GEOMETRY_BIT }
		};
		for (const auto& [shader, stage] : shaderStages)
		{
			if (shader.bytecodeLength == 0) continue; // No shader provided

			VkShaderModuleCreateInfo& moduleInfo = shaderModules[numShaders];
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.codeSize = shader.bytecodeLength;
			moduleInfo.pCode = (const uint32_t*)shader.shaderBytecode;

			VkPipelineShaderStageCreateInfo& shaderStage = stages[numShaders];
			shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.pNext = &moduleInfo;
			shaderStage.stage = stage;
			shaderStage.module = VK_NULL_HANDLE; // Must be null when using pNext
			shaderStage.pName = shader.entryPointName.c_str();

			numShaders++;
		};

		// Vertex layout
		VulkanVertexLayout vertexLayout = ConvertToVulkanVertexLayout(desc.vertexLayout);

		VkPipelineVertexInputStateCreateInfo vertexInput = {};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInput.vertexBindingDescriptionCount = (uint32_t)vertexLayout.bindings.size();
		vertexInput.pVertexBindingDescriptions = vertexLayout.bindings.data();
		vertexInput.vertexAttributeDescriptionCount = (uint32_t)vertexLayout.attributes.size();
		vertexInput.pVertexAttributeDescriptions = vertexLayout.attributes.data();

		// Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = (VkPrimitiveTopology)desc.primitiveTopology;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// Viewports and scissors
		std::array<VkViewport, MAX_SIMULTANEOUS_RENDER_TARGETS> viewport = {};
		std::array<VkRect2D, MAX_SIMULTANEOUS_RENDER_TARGETS> scissor = {};
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = std::max(1U, (uint32_t)desc.renderTargetDesc.colorFormats.size());
		viewportState.pViewports = viewport.data();
		viewportState.scissorCount = std::max(1U, (uint32_t)desc.renderTargetDesc.colorFormats.size());
		viewportState.pScissors = scissor.data();

		VkCullModeFlags vkCullMode;
		switch (desc.rasterizerState.cull)
		{
		case Cull::Disabled: vkCullMode = VK_CULL_MODE_NONE; break;
		case Cull::Front: vkCullMode = VK_CULL_MODE_FRONT_BIT; break;
		case Cull::Back: vkCullMode = VK_CULL_MODE_BACK_BIT; break;
		default:
			return DetailedResult::MakeFail("Invalid cull mode.");
		}

		VkBool32 enableDepthBias = (desc.rasterizerState.depthBias != 0 ||
			desc.rasterizerState.slopeScaledDepthBias != 0.0f)
			? VK_TRUE : VK_FALSE;

		VkPipelineRasterizationLineStateCreateInfoEXT lineState = {};
		lineState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT;
		lineState.lineRasterizationMode = (desc.rasterizerState.lineRasterizationMode == LineRasterizationMode::Smooth)
			? VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT
			: VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT;

#ifdef IGLO_VULKAN_ENABLE_CONSERVATIVE_RASTERIZATION
		VkPipelineRasterizationConservativeStateCreateInfoEXT conservativeState = {};
		conservativeState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
		conservativeState.pNext = &lineState;
		conservativeState.conservativeRasterizationMode = desc.rasterizerState.enableConservativeRaster
			? VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT
			: VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT;
		conservativeState.extraPrimitiveOverestimationSize = 0.0f;
		lineState.pNext = &conservativeState;
#endif

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.pNext = &lineState;
		rasterizer.depthClampEnable = !desc.rasterizerState.enableDepthClip;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = desc.rasterizerState.enableWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = vkCullMode;
		rasterizer.frontFace = (desc.rasterizerState.frontFace == FrontFace::CW)
			? VK_FRONT_FACE_CLOCKWISE
			: VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = enableDepthBias;
		rasterizer.depthBiasConstantFactor = 0.0f; //TODO: convert 'desc.rasterizerState.depthBias' to 'depthBiasConstantFactor'.
		rasterizer.depthBiasSlopeFactor = desc.rasterizerState.slopeScaledDepthBias;
		rasterizer.depthBiasClamp = desc.rasterizerState.depthBiasClamp;


		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.rasterizationSamples = VkSampleCountFlagBits(int(desc.renderTargetDesc.msaa));
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.alphaToCoverageEnable = desc.enableAlphaToCoverage;

		// Depth/stencil
		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = desc.depthState.enableDepth ? VK_TRUE : VK_FALSE;
		depthStencil.depthWriteEnable = (desc.depthState.depthWriteMask == DepthWriteMask::All) ? VK_TRUE : VK_FALSE;
		depthStencil.depthCompareOp = (VkCompareOp)desc.depthState.depthFunc;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = desc.depthState.enableStencil ? VK_TRUE : VK_FALSE;
		depthStencil.front =
		{
			.failOp = (VkStencilOp)desc.depthState.frontFaceStencilFailOp,
			.passOp = (VkStencilOp)desc.depthState.frontFaceStencilPassOp,
			.depthFailOp = (VkStencilOp)desc.depthState.frontFaceStencilDepthFailOp,
			.compareOp = (VkCompareOp)desc.depthState.frontFaceStencilFunc,
			.compareMask = desc.depthState.stencilReadMask,
			.writeMask = desc.depthState.stencilWriteMask,
			.reference = 0
		};
		depthStencil.back =
		{
			.failOp = (VkStencilOp)desc.depthState.backFaceStencilFailOp,
			.passOp = (VkStencilOp)desc.depthState.backFaceStencilPassOp,
			.depthFailOp = (VkStencilOp)desc.depthState.backFaceStencilDepthFailOp,
			.compareOp = (VkCompareOp)desc.depthState.backFaceStencilFunc,
			.compareMask = desc.depthState.stencilReadMask,
			.writeMask = desc.depthState.stencilWriteMask,
			.reference = 0
		};
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 1.0f;

		// Color blend attachments
		std::array<VkPipelineColorBlendAttachmentState, MAX_SIMULTANEOUS_RENDER_TARGETS> blendAttachments = {};
		for (size_t i = 0; i < desc.blendStates.size(); i++)
		{
			VkPipelineColorBlendAttachmentState& vkBlendState = blendAttachments[i];
			const BlendDesc& blendDesc = desc.blendStates[i];

			vkBlendState.blendEnable = blendDesc.enabled ? VK_TRUE : VK_FALSE;
			vkBlendState.srcColorBlendFactor = (VkBlendFactor)blendDesc.srcBlend;
			vkBlendState.dstColorBlendFactor = (VkBlendFactor)blendDesc.destBlend;
			vkBlendState.colorBlendOp = (VkBlendOp)blendDesc.blendOp;
			vkBlendState.srcAlphaBlendFactor = (VkBlendFactor)blendDesc.srcBlendAlpha;
			vkBlendState.dstAlphaBlendFactor = (VkBlendFactor)blendDesc.destBlendAlpha;
			vkBlendState.alphaBlendOp = (VkBlendOp)blendDesc.blendOpAlpha;
			vkBlendState.colorWriteMask = (VkColorComponentFlags)blendDesc.colorWriteMask;
		}

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.attachmentCount = (uint32_t)desc.blendStates.size();
		colorBlending.pAttachments = blendAttachments.data();
		colorBlending.logicOpEnable = desc.blendLogicOpEnabled ? VK_TRUE : VK_FALSE;
		colorBlending.logicOp = (VkLogicOp)desc.blendLogicOp;

		// Pipeline state
		std::array<VkFormat, MAX_SIMULTANEOUS_RENDER_TARGETS> vkColorFormats = {};
		for (size_t i = 0; i < desc.renderTargetDesc.colorFormats.size(); i++)
		{
			vkColorFormats[i] = ConvertToVulkanFormat(desc.renderTargetDesc.colorFormats[i]);
		}
		VkPipelineRenderingCreateInfo renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		renderingInfo.colorAttachmentCount = (uint32_t)desc.renderTargetDesc.colorFormats.size();
		renderingInfo.pColorAttachmentFormats = vkColorFormats.data();
		renderingInfo.depthAttachmentFormat = ConvertToVulkanFormat(desc.renderTargetDesc.depthFormat);
		renderingInfo.stencilAttachmentFormat = GetFormatInfo(desc.renderTargetDesc.depthFormat).hasStencilComponent
			? ConvertToVulkanFormat(desc.renderTargetDesc.depthFormat)
			: VK_FORMAT_UNDEFINED;


		VkDynamicState dynamicStates[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = &renderingInfo;
		pipelineInfo.stageCount = numShaders;
		pipelineInfo.pStages = stages.data();
		pipelineInfo.pVertexInputState = &vertexInput;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = context.GetDescriptorHeap().GetVulkanBindlessPipelineLayout();
		pipelineInfo.renderPass = VK_NULL_HANDLE;
		pipelineInfo.subpass = 0;
		pipelineInfo.pDynamicState = &dynamicState;

		VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &impl.pipeline);
		if (result != VK_SUCCESS)
		{
			return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreateGraphicsPipelines", result));
		}

		return DetailedResult::MakeSuccess();
	}

	DetailedResult Pipeline::Impl_LoadAsCompute(const IGLOContext& context, const Shader& CS)
	{
		VkDevice device = context.GetVulkanDevice();

		VkShaderModuleCreateInfo shaderModule = {};
		shaderModule.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModule.codeSize = CS.bytecodeLength;
		shaderModule.pCode = (const uint32_t*)CS.shaderBytecode;

		VkPipelineShaderStageCreateInfo stageInfo = {};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.pNext = &shaderModule;
		stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageInfo.module = VK_NULL_HANDLE;  // Must be null when using pNext
		stageInfo.pName = CS.entryPointName.c_str();

		VkPipelineLayout pipelineLayout = context.GetDescriptorHeap().GetVulkanBindlessPipelineLayout();

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage = stageInfo;
		pipelineInfo.layout = pipelineLayout;

		VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &impl.pipeline);
		if (result != VK_SUCCESS)
		{
			return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreateComputePipelines", result));
		}
		return DetailedResult::MakeSuccess();
	}

	void CommandQueue::DestroySwapChainSemaphores()
	{
		for (auto& semaphore : impl.acquireSemaphores)
		{
			if (semaphore) vkDestroySemaphore(context->GetVulkanDevice(), semaphore, nullptr);
			semaphore = VK_NULL_HANDLE;
		}
		for (auto& semaphore : impl.presentReadySemaphores)
		{
			if (semaphore) vkDestroySemaphore(context->GetVulkanDevice(), semaphore, nullptr);
			semaphore = VK_NULL_HANDLE;
		}
		impl.acquireSemaphores.clear();
		impl.acquireSemaphores.shrink_to_fit();
		impl.presentReadySemaphores.clear();
		impl.presentReadySemaphores.shrink_to_fit();
	}

	void CommandQueue::Impl_Unload()
	{
		DestroySwapChainSemaphores();

		for (auto& semaphore : impl.timelineSemaphores)
		{
			if (semaphore) vkDestroySemaphore(context->GetVulkanDevice(), semaphore, nullptr);
			semaphore = VK_NULL_HANDLE;
		}

		impl.queues = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
		impl.queueFamIndices = {};
		impl.presentQueue = VK_NULL_HANDLE;
		impl.fenceValues = {};
		impl.frameIndex = 0;
		impl.numFrames = 0;
		impl.currentBackBufferIndex = 0;
	}

	DetailedResult CommandQueue::Impl_Load(const IGLOContext& context, uint32_t numFramesInFlight, uint32_t numBackBuffers)
	{
		VkDevice device = context.GetVulkanDevice();
		VulkanQueueFamilies fams = GetVulkanQueueFamilies(context.GetVulkanPhysicalDevice(), context.GetVulkanSurface());

		// Graphics queue
		vkGetDeviceQueue(device, fams.graphicsFamily.value(), 0, &impl.queues[0]);
		impl.queueFamIndices[0] = fams.graphicsFamily.value();

		// Present queue
		vkGetDeviceQueue(device, fams.presentFamily.value(), 0, &impl.presentQueue);

		// Compute queue
		if (fams.computeFamily)
		{
			vkGetDeviceQueue(device, fams.computeFamily.value(), 0, &impl.queues[1]);
			impl.queueFamIndices[1] = fams.computeFamily.value();
		}
		else
		{
			impl.queues[1] = impl.queues[0];
			impl.queueFamIndices[1] = impl.queueFamIndices[0];
		}

		// Transfer queue
		if (fams.transferFamily)
		{
			vkGetDeviceQueue(device, fams.transferFamily.value(), 0, &impl.queues[2]);
			impl.queueFamIndices[2] = fams.transferFamily.value();
		}
		else if (fams.computeFamily)
		{
			impl.queues[2] = impl.queues[1];
			impl.queueFamIndices[2] = impl.queueFamIndices[1];
		}
		else
		{
			impl.queues[2] = impl.queues[0];
			impl.queueFamIndices[2] = impl.queueFamIndices[0];
		}

		// Create timeline semaphores
		{
			VkSemaphoreTypeCreateInfo timelineInfo = {};
			timelineInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
			timelineInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
			timelineInfo.initialValue = 0;

			VkSemaphoreCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			createInfo.pNext = &timelineInfo;

			for (size_t i = 0; i < impl.timelineSemaphores.size(); i++)
			{
				VkResult result = vkCreateSemaphore(device, &createInfo, nullptr, &impl.timelineSemaphores[i]);
				if (result != VK_SUCCESS)
				{
					return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreateSemaphore", result));
				}
			}
		}

		RecreateSwapChainSemaphores(numFramesInFlight, numBackBuffers);

		return DetailedResult::MakeSuccess();
	}

	void CommandQueue::RecreateSwapChainSemaphores(uint32_t numFramesInFlight, uint32_t numBackBuffers)
	{
		VkDevice device = context->GetVulkanDevice();

		DestroySwapChainSemaphores();

		// Create binary semaphores
		impl.acquireSemaphores.resize(numFramesInFlight, VK_NULL_HANDLE);
		impl.presentReadySemaphores.resize(numBackBuffers, VK_NULL_HANDLE);
		VkSemaphoreCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		createInfo.flags = VK_SEMAPHORE_TYPE_BINARY;
		for (uint32_t i = 0; i < numFramesInFlight; i++)
		{
			vkCreateSemaphore(device, &createInfo, nullptr, &impl.acquireSemaphores[i]);
		}
		for (uint32_t i = 0; i < numBackBuffers; i++)
		{
			vkCreateSemaphore(device, &createInfo, nullptr, &impl.presentReadySemaphores[i]);
		}

		impl.frameIndex = 0;
		impl.numFrames = numFramesInFlight;
		impl.currentBackBufferIndex = 0;
	}

	Receipt CommandQueue::Impl_SubmitCommands(const CommandList* const* commandLists, uint32_t numCommandLists, CommandListType cmdType)
	{
		VkCommandBuffer cmdBuffers[MAX_COMMAND_LISTS_PER_SUBMIT];
		for (uint32_t i = 0; i < numCommandLists; i++)
		{
			cmdBuffers[i] = commandLists[i]->GetVulkanCommandBuffer();
		}

		{
			std::lock_guard<std::mutex> lockGuard(impl.receiptMutex);
			uint32_t t = (uint32_t)cmdType;
			impl.fenceValues[t]++;
			uint64_t signalValue = impl.fenceValues[t];

			VkTimelineSemaphoreSubmitInfo timelineSubmit = {};
			timelineSubmit.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
			timelineSubmit.signalSemaphoreValueCount = 1;
			timelineSubmit.pSignalSemaphoreValues = &signalValue;

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = &timelineSubmit;
			submitInfo.commandBufferCount = numCommandLists;
			submitInfo.pCommandBuffers = cmdBuffers;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &impl.timelineSemaphores[t];

			vkQueueSubmit(impl.queues[t], 1, &submitInfo, VK_NULL_HANDLE);

			return { signalValue, impl.timelineSemaphores[t] };
		}
	}

	Receipt CommandQueue::SubmitSignal(CommandListType type)
	{
		std::lock_guard<std::mutex> lock(impl.receiptMutex);

		uint32_t t = (uint32_t)type;
		impl.fenceValues[t]++;
		uint64_t signalValue = impl.fenceValues[t];

		VkTimelineSemaphoreSubmitInfo timelineSubmit = {};
		timelineSubmit.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		timelineSubmit.signalSemaphoreValueCount = 1;
		timelineSubmit.pSignalSemaphoreValues = &signalValue;

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = &timelineSubmit;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &impl.timelineSemaphores[t];

		vkQueueSubmit(impl.queues[t], 1, &submitInfo, VK_NULL_HANDLE);

		return { signalValue, impl.timelineSemaphores[t] };
	}

	bool CommandQueue::Impl_IsComplete(Receipt receipt) const
	{
		uint64_t currentValue = 0;
		vkGetSemaphoreCounterValue(context->GetVulkanDevice(), receipt.impl.timelineSemaphore, &currentValue);
		return currentValue >= receipt.fenceValue;
	}

	bool CommandQueue::IsIdle() const
	{
		for (size_t i = 0; i < impl.fenceValues.size(); i++)
		{
			uint64_t currentValue = 0;
			vkGetSemaphoreCounterValue(context->GetVulkanDevice(), impl.timelineSemaphores[i], &currentValue);
			if (currentValue < impl.fenceValues[i]) return false;
		}
		return true;
	}

	void CommandQueue::Impl_WaitForCompletion(Receipt receipt)
	{
		std::lock_guard<std::mutex> lock(impl.waitMutex);
		VkSemaphoreWaitInfo waitInfo = {};
		waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		waitInfo.semaphoreCount = 1;
		waitInfo.pSemaphores = &receipt.impl.timelineSemaphore;
		waitInfo.pValues = &receipt.fenceValue;

		vkWaitSemaphores(context->GetVulkanDevice(), &waitInfo, UINT64_MAX);
	}

	void CommandQueue::WaitForIdle()
	{
		assert(impl.fenceValues.size() == impl.timelineSemaphores.size());
		for (size_t i = 0; i < impl.fenceValues.size(); i++)
		{
			WaitForCompletion({ impl.fenceValues[i], impl.timelineSemaphores[i] });
		}
		if (impl.presentQueue)
		{
			vkQueueWaitIdle(impl.presentQueue);
		}
	}

	VkResult CommandQueue::Present(VkSwapchainKHR swapChain)
	{
		// Convert timeline wait to binary signal
		{
			const uint32_t cmdTypeInt = 0; // Graphics queue

			VkTimelineSemaphoreSubmitInfo timelineInfo = {};
			timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
			timelineInfo.waitSemaphoreValueCount = 1;
			timelineInfo.pWaitSemaphoreValues = &impl.fenceValues[cmdTypeInt];

			VkPipelineStageFlags flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = &timelineInfo;
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &impl.timelineSemaphores[cmdTypeInt];
			submitInfo.pWaitDstStageMask = &flags;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &impl.presentReadySemaphores[impl.currentBackBufferIndex];

			vkQueueSubmit(impl.queues[cmdTypeInt], 1, &submitInfo, VK_NULL_HANDLE);
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &impl.presentReadySemaphores[impl.currentBackBufferIndex];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = &impl.currentBackBufferIndex;

		VkResult result = vkQueuePresentKHR(impl.presentQueue, &presentInfo);

		return result;
	}

	VkResult CommandQueue::AcquireNextVulkanSwapChainImage(VkDevice device, VkSwapchainKHR swapChain, uint64_t timeout)
	{
		VkAcquireNextImageInfoKHR acquireInfo = {};
		acquireInfo.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
		acquireInfo.swapchain = swapChain;
		acquireInfo.timeout = timeout;
		acquireInfo.semaphore = impl.acquireSemaphores[impl.frameIndex];
		acquireInfo.deviceMask = 1;
		VkResult result = vkAcquireNextImage2KHR(device, &acquireInfo, &impl.currentBackBufferIndex);

		// Only submit wait if acquisition was successful
		if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
		{
			SubmitBinaryWaitSignal(impl.acquireSemaphores[impl.frameIndex]);
			impl.frameIndex = (impl.frameIndex + 1) % impl.numFrames;
		}
		return result;
	}

	Receipt CommandQueue::SubmitBinaryWaitSignal(VkSemaphore binarySemaphore)
	{
		const uint32_t cmdTypeInt = 0; // Graphics queue
		std::lock_guard<std::mutex> lock(impl.receiptMutex);

		impl.fenceValues[cmdTypeInt]++;
		uint64_t signalValue = impl.fenceValues[cmdTypeInt];

		VkTimelineSemaphoreSubmitInfo timelineSubmit = {};
		timelineSubmit.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		timelineSubmit.signalSemaphoreValueCount = 1;
		timelineSubmit.pSignalSemaphoreValues = &signalValue;

		VkPipelineStageFlags flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = &timelineSubmit;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &impl.timelineSemaphores[cmdTypeInt];
		submitInfo.waitSemaphoreCount = 1;
		// This signal will complete only after the swap chain image is ready.
		submitInfo.pWaitSemaphores = &binarySemaphore;
		submitInfo.pWaitDstStageMask = &flags;

		vkQueueSubmit(impl.queues[cmdTypeInt], 1, &submitInfo, VK_NULL_HANDLE);

		return { signalValue, impl.timelineSemaphores[cmdTypeInt] };
	}

	void CommandList::Impl_Unload()
	{
		for (size_t i = 0; i < impl.commandBuffer.size(); i++)
		{
			if (impl.commandBuffer[i])
			{
				assert(i < impl.commandPool.size());
				vkFreeCommandBuffers(context->GetVulkanDevice(), impl.commandPool[i], 1, &impl.commandBuffer[i]);
			}
		}
		for (VkCommandPool pool : impl.commandPool)
		{
			if (pool) vkDestroyCommandPool(context->GetVulkanDevice(), pool, nullptr);
		}

		impl.commandPool.clear();
		impl.commandPool.shrink_to_fit();
		impl.commandBuffer.clear();
		impl.commandBuffer.shrink_to_fit();

		impl.numGlobalBarriers = 0;
		impl.numTextureBarriers = 0;
		impl.numBufferBarriers = 0;

		impl.numCurrentRenderTextures = 0;
		impl.currentDepthTexture = nullptr;
		impl.currentCommandBuffer = VK_NULL_HANDLE;

		impl.isRendering = false;
		impl.temporaryRenderPause = false;
	}

	DetailedResult CommandList::Impl_Load(const IGLOContext& context, CommandListType commandListType)
	{
		VkDevice device = context.GetVulkanDevice();
		uint32_t queueFamilyIndex = context.GetCommandQueue().GetVulkanQueueFamilyIndex(commandListType);

		impl.commandPool.resize(numFrames, VK_NULL_HANDLE);
		impl.commandBuffer.resize(numFrames, VK_NULL_HANDLE);

		for (uint32_t i = 0; i < numFrames; i++)
		{
			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			poolInfo.queueFamilyIndex = queueFamilyIndex;

			VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &impl.commandPool[i]);
			if (result != VK_SUCCESS)
			{
				return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreateCommandPool", result));
			}

			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = impl.commandPool[i];
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;

			result = vkAllocateCommandBuffers(device, &allocInfo, &impl.commandBuffer[i]);
			if (result != VK_SUCCESS)
			{
				return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkAllocateCommandBuffers", result));
			}
		}

		impl.globalBarriers = {};
		impl.textureBarriers = {};
		impl.bufferBarriers = {};

		impl.currentRenderTextures = {};
		impl.renderInfo = {};

		return DetailedResult::MakeSuccess();
	}

	void CommandList::Impl_Begin()
	{
		DescriptorHeap& heap = context->GetDescriptorHeap();
		VkCommandBuffer cmdBuf = impl.commandBuffer[frameIndex];

		VkResult res = vkResetCommandPool(context->GetVulkanDevice(), impl.commandPool[frameIndex], 0);
		if (res != VK_SUCCESS)
		{
			Log(LogType::Error, "vkResetCommandPool failed!");
		}

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		res = vkBeginCommandBuffer(cmdBuf, &beginInfo);
		if (res != VK_SUCCESS)
		{
			Log(LogType::Error, "vkBeginCommandBuffer failed!");
		}

		impl.currentCommandBuffer = cmdBuf;

		if (commandListType == CommandListType::Graphics ||
			commandListType == CommandListType::Compute)
		{
			VkPipelineLayout bindlessLayout = heap.GetVulkanBindlessPipelineLayout();
			VkDescriptorSet bindlessSet = heap.GetVulkanDescriptorSet();

			uint32_t setIndex = 0;
			if (commandListType == CommandListType::Graphics)
			{
				vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, bindlessLayout, setIndex, 1, &bindlessSet, 0, nullptr);
			}
			if (commandListType == CommandListType::Graphics ||
				commandListType == CommandListType::Compute)
			{
				vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, bindlessLayout, setIndex, 1, &bindlessSet, 0, nullptr);
			}
		}
	}

	void CommandList::Impl_End()
	{
		SafeEndRendering();
		if (impl.currentCommandBuffer)
		{
			VkResult res = vkEndCommandBuffer(impl.currentCommandBuffer);
			if (res != VK_SUCCESS)
			{
				Log(LogType::Error, "vkEndCommandBuffer failed!");
			}
			impl.currentCommandBuffer = VK_NULL_HANDLE;
		}
		else
		{
			Log(LogType::Error, "Failed to stop recording commands."
				" Reason: The command list had not began recording commands in the first place.");
		}
	}

	void CommandList::FlushBarriers()
	{
		if (impl.numGlobalBarriers > 0 || impl.numTextureBarriers > 0 || impl.numBufferBarriers > 0)
		{
			VkDependencyInfo dependencyInfo = {};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.memoryBarrierCount = impl.numGlobalBarriers;
			dependencyInfo.pMemoryBarriers = impl.globalBarriers.data();
			dependencyInfo.imageMemoryBarrierCount = impl.numTextureBarriers;
			dependencyInfo.pImageMemoryBarriers = impl.textureBarriers.data();
			dependencyInfo.bufferMemoryBarrierCount = impl.numBufferBarriers;
			dependencyInfo.pBufferMemoryBarriers = impl.bufferBarriers.data();

			SafePauseRendering();
			vkCmdPipelineBarrier2(impl.currentCommandBuffer, &dependencyInfo);
			SafeResumeRendering();
		}

		impl.numGlobalBarriers = 0;
		impl.numTextureBarriers = 0;
		impl.numBufferBarriers = 0;
	}

	void CommandList::AddGlobalBarrier(BarrierSync syncBefore, BarrierSync syncAfter, BarrierAccess accessBefore, BarrierAccess accessAfter)
	{
		if (impl.numGlobalBarriers >= MAX_BATCHED_BARRIERS_PER_TYPE) FlushBarriers();

		VkMemoryBarrier2& barrier = impl.globalBarriers[impl.numGlobalBarriers];
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
		barrier.srcStageMask = (VkPipelineStageFlags2)syncBefore;
		barrier.srcAccessMask = (VkAccessFlags2)accessBefore;
		barrier.dstStageMask = (VkPipelineStageFlags2)syncAfter;
		barrier.dstAccessMask = (VkAccessFlags2)accessAfter;

		impl.numGlobalBarriers++;
	}

	void CommandList::AddTextureBarrier(const Texture& texture, BarrierSync syncBefore, BarrierSync syncAfter,
		BarrierAccess accessBefore, BarrierAccess accessAfter, BarrierLayout layoutBefore, BarrierLayout layoutAfter, bool discard)
	{
		if (impl.numTextureBarriers >= MAX_BATCHED_BARRIERS_PER_TYPE) FlushBarriers();

		VkImageMemoryBarrier2& barrier = impl.textureBarriers[impl.numTextureBarriers];
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.srcStageMask = (VkPipelineStageFlags2)syncBefore;
		barrier.srcAccessMask = (VkAccessFlags2)accessBefore;
		barrier.dstStageMask = (VkPipelineStageFlags2)syncAfter;
		barrier.dstAccessMask = (VkAccessFlags2)accessAfter;
		barrier.oldLayout = (VkImageLayout)layoutBefore;
		barrier.newLayout = (VkImageLayout)layoutAfter;
		barrier.image = texture.GetVulkanImage();
		barrier.subresourceRange.aspectMask = GetVulkanImageAspect(texture.GetFormat());
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = texture.GetMipLevels();
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = texture.GetNumFaces();

		impl.numTextureBarriers++;

		// Render pass ends when we detect a texture barrier involving an active renter target.
		if (impl.isRendering)
		{
			for (uint32_t a = 0; a < impl.numCurrentRenderTextures; a++)
			{
				if (barrier.image == impl.currentRenderTextures[a]->GetVulkanImage())
				{
					SafeEndRendering();
					break;
				}
			}
		}
	}

	void CommandList::AddTextureBarrierAtSubresource(const Texture& texture, BarrierSync syncBefore, BarrierSync syncAfter,
		BarrierAccess accessBefore, BarrierAccess accessAfter, BarrierLayout layoutBefore, BarrierLayout layoutAfter,
		uint32_t faceIndex, uint32_t mipIndex, bool discard)
	{
		if (impl.numTextureBarriers >= MAX_BATCHED_BARRIERS_PER_TYPE) FlushBarriers();

		VkImageMemoryBarrier2& barrier = impl.textureBarriers[impl.numTextureBarriers];
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.srcStageMask = (VkPipelineStageFlags2)syncBefore;
		barrier.srcAccessMask = (VkAccessFlags2)accessBefore;
		barrier.dstStageMask = (VkPipelineStageFlags2)syncAfter;
		barrier.dstAccessMask = (VkAccessFlags2)accessAfter;
		barrier.oldLayout = (VkImageLayout)layoutBefore;
		barrier.newLayout = (VkImageLayout)layoutAfter;
		barrier.image = texture.GetVulkanImage();
		barrier.subresourceRange.aspectMask = GetVulkanImageAspect(texture.GetFormat());
		barrier.subresourceRange.baseMipLevel = mipIndex;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = faceIndex;
		barrier.subresourceRange.layerCount = 1;

		impl.numTextureBarriers++;
	}

	void CommandList::AddBufferBarrier(const Buffer& buffer, BarrierSync syncBefore, BarrierSync syncAfter,
		BarrierAccess accessBefore, BarrierAccess accessAfter)
	{
		if (impl.numBufferBarriers >= MAX_BATCHED_BARRIERS_PER_TYPE) FlushBarriers();

		VkBufferMemoryBarrier2& barrier = impl.bufferBarriers[impl.numBufferBarriers];
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
		barrier.srcStageMask = (VkPipelineStageFlags2)syncBefore;
		barrier.srcAccessMask = (VkAccessFlags2)accessBefore;
		barrier.dstStageMask = (VkPipelineStageFlags2)syncAfter;
		barrier.dstAccessMask = (VkAccessFlags2)accessAfter;
		barrier.buffer = buffer.GetVulkanBuffer();
		barrier.offset = 0;
		barrier.size = buffer.GetSize();

		impl.numBufferBarriers++;
	}

	void CommandList::SetPipeline(const Pipeline& pipeline)
	{
		VkPipelineBindPoint bindPoint = pipeline.IsComputePipeline()
			? VK_PIPELINE_BIND_POINT_COMPUTE
			: VK_PIPELINE_BIND_POINT_GRAPHICS;
		vkCmdBindPipeline(impl.currentCommandBuffer, bindPoint, pipeline.GetVulkanPipeline());
	}

	void CommandList::Impl_SetRenderTargets(const Texture* const* renderTextures, uint32_t numRenderTextures,
		const Texture* depthBuffer, bool optimizedClear)
	{
		SafeEndRendering();

		if (numRenderTextures > 0 || depthBuffer)
		{
			impl.renderInfo.colorAttachments = {};
			for (uint32_t i = 0; i < numRenderTextures; i++)
			{
				VkRenderingAttachmentInfo& attachment = impl.renderInfo.colorAttachments[i];
				attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				attachment.imageView = renderTextures[i]->GetVulkanImageView_RTV_DSV();
				attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachment.loadOp = optimizedClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
				attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

				if (optimizedClear)
				{
					Color color = renderTextures[i]->GetOptimiziedClearValue().color;
					attachment.clearValue.color = { color.red, color.green, color.blue, color.alpha };
				}
			}

			bool hasStencil = false;
			impl.renderInfo.depthAttachment = {};
			impl.renderInfo.stencilAttachment = {};
			if (depthBuffer)
			{
				hasStencil = GetFormatInfo(depthBuffer->GetFormat()).hasStencilComponent;

				impl.renderInfo.depthAttachment =
				{
					.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
					.imageView = depthBuffer->GetVulkanImageView_RTV_DSV(),
					.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					.loadOp = optimizedClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				};
				if (hasStencil)
				{
					impl.renderInfo.stencilAttachment =
					{
						 .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
						 .imageView = depthBuffer->GetVulkanImageView_RTV_DSV(),
						 .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						 .loadOp = optimizedClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
						 .storeOp = VK_ATTACHMENT_STORE_OP_STORE
					};
				}

				if (optimizedClear)
				{
					ClearValue clearValue = depthBuffer->GetOptimiziedClearValue();
					impl.renderInfo.depthAttachment.clearValue.depthStencil.depth = clearValue.depth;
					if (hasStencil) impl.renderInfo.stencilAttachment.clearValue.depthStencil.stencil = clearValue.stencil;
				}

			}

			impl.renderInfo.renderingInfo = {};
			impl.renderInfo.renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
			impl.renderInfo.renderingInfo.renderArea.offset = { 0, 0 };
			impl.renderInfo.renderingInfo.renderArea.extent =
			{
				(numRenderTextures > 0) ? renderTextures[0]->GetWidth() : depthBuffer->GetWidth(),
				(numRenderTextures > 0) ? renderTextures[0]->GetHeight() : depthBuffer->GetHeight()
			};
			impl.renderInfo.renderingInfo.layerCount = 1;
			impl.renderInfo.renderingInfo.colorAttachmentCount = numRenderTextures;
			impl.renderInfo.renderingInfo.pColorAttachments = impl.renderInfo.colorAttachments.data();
			impl.renderInfo.renderingInfo.pDepthAttachment = depthBuffer ? &impl.renderInfo.depthAttachment : nullptr;
			impl.renderInfo.renderingInfo.pStencilAttachment = (hasStencil) ? &impl.renderInfo.stencilAttachment : nullptr;

			vkCmdBeginRendering(impl.currentCommandBuffer, &impl.renderInfo.renderingInfo);

			// It's important we don't clear these render targets again when pausing and resuming render passes later.
			for (uint32_t i = 0; i < numRenderTextures; i++)
			{
				impl.renderInfo.colorAttachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			}
			if (depthBuffer) impl.renderInfo.depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			if (hasStencil) impl.renderInfo.stencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

			// Remember which render targets are active
			impl.numCurrentRenderTextures = numRenderTextures;
			for (uint32_t i = 0; i < numRenderTextures; i++)
			{
				impl.currentRenderTextures[i] = renderTextures[i];
			}
			impl.currentDepthTexture = depthBuffer;

			// Render pass has begun
			impl.isRendering = true;
		}
	}

	void CommandList::Impl_ClearColor(const Texture& renderTexture, Color color, uint32_t numRects, const IntRect* rects)
	{
		// Check if the texture is one of the current render targets
		int32_t attachmentIndex = -1;
		if (impl.isRendering)
		{
			for (uint32_t i = 0; i < impl.numCurrentRenderTextures; i++)
			{
				if (impl.currentRenderTextures[i] == &renderTexture)
				{
					attachmentIndex = i;
					break;
				}
			}
		}

		if (attachmentIndex >= 0)
		{
			VkClearAttachment clearAttachment = {};
			clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			clearAttachment.colorAttachment = (uint32_t)attachmentIndex;
			clearAttachment.clearValue.color = { color.red, color.green, color.blue, color.alpha };

			if (numRects == 0)
			{
				VkClearRect clearRect = {};
				clearRect.rect = { {0, 0}, {renderTexture.GetWidth(), renderTexture.GetHeight()} };
				clearRect.baseArrayLayer = 0;
				clearRect.layerCount = renderTexture.GetNumFaces();
				vkCmdClearAttachments(impl.currentCommandBuffer, 1, &clearAttachment, 1, &clearRect);
			}
			else
			{
				std::vector<VkClearRect> clearRects(numRects);
				for (uint32_t i = 0; i < numRects; i++)
				{
					clearRects[i].rect.offset = { rects[i].left, rects[i].top };
					clearRects[i].rect.extent = { (uint32_t)rects[i].GetWidth(), (uint32_t)rects[i].GetHeight() };
					clearRects[i].baseArrayLayer = 0;
					clearRects[i].layerCount = renderTexture.GetNumFaces();
				}
				vkCmdClearAttachments(impl.currentCommandBuffer, 1, &clearAttachment, numRects, clearRects.data());
			}
		}
		else
		{
			VkClearColorValue clearValue = { color.red, color.green, color.blue, color.alpha };
			VkImageSubresourceRange range = {};
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = renderTexture.GetMipLevels();
			range.baseArrayLayer = 0;
			range.layerCount = renderTexture.GetNumFaces();

			SafePauseRendering();
			vkCmdClearColorImage(impl.currentCommandBuffer, renderTexture.GetVulkanImage(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &range);
			SafeResumeRendering();

			if (numRects > 0)
			{
				Log(LogType::Warning, "Partial rect clears not supported outside render pass. Entire render texture cleared.");
			}
		}
	}

	void CommandList::Impl_ClearDepth(const Texture& depthBuffer, float depth, byte stencil, bool clearDepth, bool clearStencil,
		uint32_t numRects, const IntRect* rects)
	{
		if (impl.isRendering && (impl.currentDepthTexture == &depthBuffer))
		{
			VkClearAttachment clearAttachment = {};
			clearAttachment.aspectMask = (clearDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) | (clearStencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
			clearAttachment.clearValue.depthStencil = { depth, stencil };

			if (numRects == 0)
			{
				VkClearRect clearRect = {};
				clearRect.rect = { {0, 0}, {depthBuffer.GetWidth(), depthBuffer.GetHeight()} };
				clearRect.baseArrayLayer = 0;
				clearRect.layerCount = depthBuffer.GetNumFaces();
				vkCmdClearAttachments(impl.currentCommandBuffer, 1, &clearAttachment, 1, &clearRect);
			}
			else
			{
				std::vector<VkClearRect> clearRects(numRects);
				for (uint32_t i = 0; i < numRects; i++)
				{
					clearRects[i].rect.offset = { rects[i].left, rects[i].top };
					clearRects[i].rect.extent = { (uint32_t)rects[i].GetWidth(), (uint32_t)rects[i].GetHeight() };
					clearRects[i].baseArrayLayer = 0;
					clearRects[i].layerCount = depthBuffer.GetNumFaces();
				}
				vkCmdClearAttachments(impl.currentCommandBuffer, 1, &clearAttachment, numRects, clearRects.data());
			}
		}
		else
		{
			VkClearDepthStencilValue clearValue = { depth, stencil };
			VkImageSubresourceRange range = {};
			range.aspectMask = (clearDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) | (clearStencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
			range.baseMipLevel = 0;
			range.levelCount = depthBuffer.GetMipLevels();
			range.baseArrayLayer = 0;
			range.layerCount = depthBuffer.GetNumFaces();

			SafePauseRendering();
			vkCmdClearDepthStencilImage(impl.currentCommandBuffer, depthBuffer.GetVulkanImage(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &range);
			SafeResumeRendering();

			if (numRects > 0)
			{
				Log(LogType::Warning, "Partial rect clears not supported outside render pass. Entire depth buffer cleared.");
			}
		}
	}

	void CommandList::Impl_ClearUnorderedAccessBufferUInt32(const Buffer& buffer, const uint32_t value)
	{
		vkCmdFillBuffer(impl.currentCommandBuffer, buffer.GetVulkanBuffer(), 0, VK_WHOLE_SIZE, value);
	}

	void CommandList::Impl_ClearUnorderedAccessTextureFloat(const Texture& texture, const float values[4])
	{
		VkClearColorValue clearColor = {};
		for (uint32_t i = 0; i < 4; i++)
		{
			clearColor.float32[i] = values[i];
		}

		VkImageSubresourceRange range = {};
		range.aspectMask = GetVulkanImageAspect(texture.GetFormat());
		range.baseMipLevel = 0;
		range.levelCount = texture.GetMipLevels();
		range.baseArrayLayer = 0;
		range.layerCount = texture.GetNumFaces();

		vkCmdClearColorImage(impl.currentCommandBuffer, texture.GetVulkanImage(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);
	}

	void CommandList::Impl_ClearUnorderedAccessTextureUInt32(const Texture& texture, const uint32_t values[4])
	{
		VkClearColorValue clearColor = {};
		for (uint32_t i = 0; i < 4; i++)
		{
			clearColor.uint32[i] = values[i];
		}

		VkImageSubresourceRange range = {};
		range.aspectMask = GetVulkanImageAspect(texture.GetFormat());
		range.baseMipLevel = 0;
		range.levelCount = texture.GetMipLevels();
		range.baseArrayLayer = 0;
		range.layerCount = texture.GetNumFaces();

		vkCmdClearColorImage(impl.currentCommandBuffer, texture.GetVulkanImage(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);
	}

	void CommandList::Impl_SetPushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes)
	{
		VkPipelineLayout layout = context->GetDescriptorHeap().GetVulkanBindlessPipelineLayout();
		vkCmdPushConstants(impl.currentCommandBuffer, layout, VK_SHADER_STAGE_ALL, destOffsetInBytes, sizeInBytes, data);
	}

	void CommandList::Impl_SetComputePushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes)
	{
		VkPipelineLayout layout = context->GetDescriptorHeap().GetVulkanBindlessPipelineLayout();
		vkCmdPushConstants(impl.currentCommandBuffer, layout, VK_SHADER_STAGE_ALL, destOffsetInBytes, sizeInBytes, data);
	}

	void CommandList::Impl_SetVertexBuffers(const Buffer* const* vertexBuffers, uint32_t count, uint32_t startSlot)
	{
		VkBuffer buffers[MAX_VERTEX_BUFFER_BIND_SLOTS];
		VkDeviceSize offsets[MAX_VERTEX_BUFFER_BIND_SLOTS];
		for (uint32_t i = 0; i < count; i++)
		{
			buffers[i] = vertexBuffers[i]->GetVulkanBuffer();
			offsets[i] = 0;
		}
		vkCmdBindVertexBuffers(impl.currentCommandBuffer, startSlot, count, buffers, offsets);
	}

	void CommandList::SetIndexBuffer(const Buffer& indexBuffer)
	{
		VkBuffer vkBuffer = indexBuffer.GetVulkanBuffer();
		VkDeviceSize offset = 0;
		VkIndexType indexType = (indexBuffer.GetStride() == 4) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
		vkCmdBindIndexBuffer(impl.currentCommandBuffer, vkBuffer, offset, indexType);
	}

	void CommandList::Draw(uint32_t vertexCount, uint32_t startVertexLocation)
	{
		vkCmdDraw(impl.currentCommandBuffer, vertexCount, 1, startVertexLocation, 0);
	}
	void CommandList::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation,
		uint32_t startInstanceLocation)
	{
		vkCmdDraw(impl.currentCommandBuffer, vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
	}
	void CommandList::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation)
	{
		vkCmdDrawIndexed(impl.currentCommandBuffer, indexCount, 1, startIndexLocation, baseVertexLocation, 0);
	}
	void CommandList::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
		int32_t baseVertexLocation, uint32_t startInstanceLocation)
	{
		vkCmdDrawIndexed(impl.currentCommandBuffer, indexCountPerInstance, instanceCount, startIndexLocation,
			baseVertexLocation, startInstanceLocation);
	}

	void CommandList::SetViewports(const Viewport* viewPorts, uint32_t count)
	{
		VkViewport vkViewports[MAX_SIMULTANEOUS_RENDER_TARGETS];
		for (uint32_t i = 0; i < count; i++)
		{
			// Vulkan and D3D12 use different coordinate systems for the Y-axis.
			// We flip the viewport's Y-axis here for compatibility with D3D12.
			vkViewports[i].x = viewPorts[i].x;
			vkViewports[i].y = viewPorts[i].y + viewPorts[i].height;
			vkViewports[i].width = viewPorts[i].width;
			vkViewports[i].height = -viewPorts[i].height;
			vkViewports[i].minDepth = viewPorts[i].minDepth;
			vkViewports[i].maxDepth = viewPorts[i].maxDepth;
		}
		vkCmdSetViewport(impl.currentCommandBuffer, 0, count, vkViewports);
	}

	void CommandList::SetScissorRectangles(const IntRect* scissorRects, uint32_t count)
	{
		VkRect2D vkScissors[MAX_SIMULTANEOUS_RENDER_TARGETS];
		for (uint32_t i = 0; i < count; i++)
		{
			vkScissors[i].offset = { scissorRects[i].left,scissorRects[i].top };
			vkScissors[i].extent = { (uint32_t)scissorRects[i].GetWidth(), (uint32_t)scissorRects[i].GetHeight() };
		}
		vkCmdSetScissor(impl.currentCommandBuffer, 0, count, vkScissors);
	}

	void CommandList::DispatchCompute(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
	{
		vkCmdDispatch(impl.currentCommandBuffer, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}

	void CommandList::Impl_CopyTexture(const Texture& source, const Texture& destination)
	{
		// Copy all mip levels and faces
		std::vector<VkImageCopy> regionList;
		regionList.reserve(source.GetMipLevels());
		for (uint32_t mip = 0; mip < source.GetMipLevels(); mip++)
		{
			VkImageCopy region = {};

			region.srcSubresource.aspectMask = GetVulkanImageAspect(source.GetFormat());
			region.srcSubresource.mipLevel = mip;
			region.srcSubresource.baseArrayLayer = 0;
			region.srcSubresource.layerCount = source.GetNumFaces();

			region.dstSubresource.aspectMask = GetVulkanImageAspect(destination.GetFormat());
			region.dstSubresource.mipLevel = mip;
			region.dstSubresource.baseArrayLayer = 0;
			region.dstSubresource.layerCount = destination.GetNumFaces();

			region.extent =
			{
				std::max(source.GetWidth() >> mip, 1u),
				std::max(source.GetHeight() >> mip, 1u),
				1u // Depth is always 1 for 2D textures
			};

			regionList.push_back(region);
		}

		SafePauseRendering();
		vkCmdCopyImage(impl.currentCommandBuffer,
			source.GetVulkanImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			destination.GetVulkanImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			(uint32_t)regionList.size(), regionList.data());
		SafeResumeRendering();
	}

	void CommandList::Impl_CopyTextureSubresource(const Texture& source, uint32_t sourceFaceIndex, uint32_t sourceMipIndex,
		const Texture& destination, uint32_t destFaceIndex, uint32_t destMipIndex)
	{
		VkImageCopy region = {};
		region.srcSubresource.aspectMask = GetVulkanImageAspect(source.GetFormat());
		region.srcSubresource.mipLevel = sourceMipIndex;
		region.srcSubresource.baseArrayLayer = sourceFaceIndex;
		region.srcSubresource.layerCount = 1;

		region.dstSubresource.aspectMask = GetVulkanImageAspect(destination.GetFormat());
		region.dstSubresource.mipLevel = destMipIndex;
		region.dstSubresource.baseArrayLayer = destFaceIndex;
		region.dstSubresource.layerCount = 1;

		region.extent =
		{
			std::max(source.GetWidth() >> sourceMipIndex, 1u),
			std::max(source.GetHeight() >> sourceMipIndex, 1u),
			1u  // Depth is always 1 for 2D textures
		};

		SafePauseRendering();
		vkCmdCopyImage(impl.currentCommandBuffer,
			source.GetVulkanImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			destination.GetVulkanImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &region);
		SafeResumeRendering();
	}

	void CommandList::Impl_CopyTextureToReadableTexture(const Texture& source, const Texture& destination)
	{
		const uint32_t numFaces = source.GetNumFaces();
		const uint32_t numMips = source.GetMipLevels();
		std::vector<VkImageCopy> regionList;
		regionList.reserve(numFaces * numMips);

		for (uint32_t face = 0; face < numFaces; face++)
		{
			for (uint32_t mip = 0; mip < numMips; mip++)
			{
				VkImageCopy region = {};
				region.srcSubresource.aspectMask = GetVulkanImageAspect(source.GetFormat());
				region.srcSubresource.mipLevel = mip;
				region.srcSubresource.baseArrayLayer = face;
				region.srcSubresource.layerCount = 1;

				region.dstSubresource.aspectMask = GetVulkanImageAspect(destination.GetFormat());
				region.dstSubresource.mipLevel = mip;
				region.dstSubresource.baseArrayLayer = face;
				region.dstSubresource.layerCount = 1;

				region.extent =
				{
					std::max(source.GetWidth() >> mip, 1u),
					std::max(source.GetHeight() >> mip, 1u),
					1u  // Depth is always 1 for 2D textures
				};

				regionList.push_back(region);
			}
		}

		SafePauseRendering();
		vkCmdCopyImage(impl.currentCommandBuffer,
			source.GetVulkanImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			destination.GetVulkanImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			(uint32_t)regionList.size(), regionList.data());
		SafeResumeRendering();
	}

	void CommandList::CopyBuffer(const Buffer& source, const Buffer& destination)
	{
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = source.GetSize();

		SafePauseRendering();
		vkCmdCopyBuffer(impl.currentCommandBuffer, source.GetVulkanBuffer(), destination.GetVulkanBuffer(), 1, &copyRegion);
		SafeResumeRendering();
	}

	void CommandList::CopyTempBufferToTexture(const TempBuffer& source, const Texture& destination)
	{
		std::vector<VkBufferImageCopy> regionList;
		regionList.reserve(destination.GetNumFaces() * destination.GetMipLevels());

		FormatInfo formatInfo = GetFormatInfo(destination.GetFormat());
		bool blockCompressed = (formatInfo.blockSize > 0);
		uint32_t texelBlockSize = blockCompressed ? 4 : 1;

		uint64_t vkBufferOffset = source.offset;
		for (uint32_t face = 0; face < destination.GetNumFaces(); face++)
		{
			for (uint32_t mip = 0; mip < destination.GetMipLevels(); mip++)
			{
				Extent2D mipExtent = Image::CalculateMipExtent(destination.GetExtent(), mip);

				VkBufferImageCopy region = {};
				region.bufferOffset = vkBufferOffset;
				region.bufferRowLength = (uint32_t)AlignUp(mipExtent.width, texelBlockSize);
				region.bufferImageHeight = (uint32_t)AlignUp(mipExtent.height, texelBlockSize);
				region.imageSubresource.aspectMask = GetVulkanImageAspect(destination.GetFormat());
				region.imageSubresource.mipLevel = mip;
				region.imageSubresource.baseArrayLayer = face;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = { mipExtent.width, mipExtent.height, 1 };

				regionList.push_back(region);

				uint64_t basicRowPitch = Image::CalculateMipRowPitch(destination.GetExtent(), destination.GetFormat(), mip);
				uint64_t basicMipSize = Image::CalculateMipSize(destination.GetExtent(), destination.GetFormat(), mip);
				uint64_t alignedRowPitch = AlignUp(basicRowPitch, context->GetGraphicsSpecs().bufferPlacementAlignments.textureRowPitch);
				uint64_t numScanLines = basicMipSize / basicRowPitch;
				vkBufferOffset += (alignedRowPitch * numScanLines);
			}
		}


		SafePauseRendering();
		vkCmdCopyBufferToImage(impl.currentCommandBuffer,
			source.impl.buffer, destination.GetVulkanImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			(uint32_t)regionList.size(), regionList.data());
		SafeResumeRendering();
	}

	void CommandList::CopyTempBufferToTextureSubresource(const TempBuffer& source, const Texture& destination, uint32_t destFaceIndex, uint32_t destMipIndex)
	{
		Extent2D mipExtent = Image::CalculateMipExtent(destination.GetExtent(), destMipIndex);
		FormatInfo formatInfo = GetFormatInfo(destination.GetFormat());
		bool blockCompressed = (formatInfo.blockSize > 0);
		uint32_t texelBlockSize = blockCompressed ? 4 : 1;

		VkBufferImageCopy region = {};
		region.bufferOffset = source.offset;
		region.bufferRowLength = (uint32_t)AlignUp(mipExtent.width, texelBlockSize);
		region.bufferImageHeight = (uint32_t)AlignUp(mipExtent.height, texelBlockSize);
		region.imageSubresource.aspectMask = GetVulkanImageAspect(destination.GetFormat());
		region.imageSubresource.mipLevel = destMipIndex;
		region.imageSubresource.baseArrayLayer = destFaceIndex;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { mipExtent.width, mipExtent.height, 1 };

		SafePauseRendering();
		vkCmdCopyBufferToImage(impl.currentCommandBuffer, source.impl.buffer, destination.GetVulkanImage(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		SafeResumeRendering();
	}

	void CommandList::CopyTempBufferToBuffer(const TempBuffer& source, const Buffer& destination)
	{
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = source.offset;
		copyRegion.dstOffset = 0;
		copyRegion.size = destination.GetSize();

		SafePauseRendering();
		vkCmdCopyBuffer(impl.currentCommandBuffer, source.impl.buffer, destination.GetVulkanBuffer(), 1, &copyRegion);
		SafeResumeRendering();
	}

	void CommandList::SafeEndRendering()
	{
		if (impl.isRendering)
		{
			if (impl.temporaryRenderPause) throw std::runtime_error("This should be impossible.");

			vkCmdEndRendering(impl.currentCommandBuffer);

			impl.isRendering = false;
			impl.numCurrentRenderTextures = 0;
			impl.currentDepthTexture = nullptr;
		}
	}

	void CommandList::SafePauseRendering()
	{
		if (impl.isRendering)
		{
			if (impl.temporaryRenderPause) throw std::runtime_error("This should be impossible.");

			vkCmdEndRendering(impl.currentCommandBuffer);

			impl.isRendering = false;
			impl.temporaryRenderPause = true;
		}
	}

	void CommandList::SafeResumeRendering()
	{
		if (impl.temporaryRenderPause)
		{
			if (impl.isRendering) throw std::runtime_error("This should be impossible.");

			vkCmdBeginRendering(impl.currentCommandBuffer, &impl.renderInfo.renderingInfo);

			impl.isRendering = true;
			impl.temporaryRenderPause = false;
		}
	}

	void CommandList::ResolveTexture(const Texture& source, const Texture& destination)
	{
		VkImageResolve resolveRegion = {};
		resolveRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		resolveRegion.srcSubresource.mipLevel = 0;
		resolveRegion.srcSubresource.baseArrayLayer = 0;
		resolveRegion.srcSubresource.layerCount = 1;
		resolveRegion.srcOffset = { 0, 0, 0 };

		resolveRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		resolveRegion.dstSubresource.mipLevel = 0;
		resolveRegion.dstSubresource.baseArrayLayer = 0;
		resolveRegion.dstSubresource.layerCount = 1;
		resolveRegion.dstOffset = { 0, 0, 0 };

		resolveRegion.extent =
		{
			source.GetWidth(),
			source.GetHeight(),
			1 // Depth
		};

		SafePauseRendering();
		vkCmdResolveImage(impl.currentCommandBuffer,
			source.GetVulkanImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			destination.GetVulkanImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &resolveRegion);
		SafeResumeRendering();
	}

	void TempBufferAllocator::Page::Impl_Free(const IGLOContext& context)
	{
		if (impl.memory) vkUnmapMemory(context.GetVulkanDevice(), impl.memory);
		if (impl.buffer) vkDestroyBuffer(context.GetVulkanDevice(), impl.buffer, nullptr);
		if (impl.memory) vkFreeMemory(context.GetVulkanDevice(), impl.memory, nullptr);
		impl.buffer = VK_NULL_HANDLE;
		impl.memory = VK_NULL_HANDLE;
	}

	TempBufferAllocator::Page TempBufferAllocator::Page::Create(const IGLOContext& context, uint64_t sizeInBytes)
	{
		Page out;
		VkDevice device = context.GetVulkanDevice();
		VkPhysicalDevice physicalDevice = context.GetVulkanPhysicalDevice();

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeInBytes;
		bufferInfo.usage =
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			//VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
			//VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &out.impl.buffer);
		if (result != VK_SUCCESS) return Page();

		VkMemoryRequirements memReq = {};
		vkGetBufferMemoryRequirements(device, out.impl.buffer, &memReq);

		VkMemoryPropertyFlags memPropFlags =
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		std::optional<uint32_t> memTypeIndex = FindVulkanMemoryType(physicalDevice, memReq.memoryTypeBits, memPropFlags);

		if (!memTypeIndex.has_value())
		{
			vkDestroyBuffer(device, out.impl.buffer, nullptr);
			return Page();
		}

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReq.size;
		allocInfo.memoryTypeIndex = memTypeIndex.value();

		result = vkAllocateMemory(device, &allocInfo, nullptr, &out.impl.memory);
		if (result != VK_SUCCESS)
		{
			vkDestroyBuffer(device, out.impl.buffer, nullptr);
			return Page();
		}

		vkBindBufferMemory(device, out.impl.buffer, out.impl.memory, 0);

		result = vkMapMemory(device, out.impl.memory, 0, sizeInBytes, 0, &out.mapped);
		if (result != VK_SUCCESS)
		{
			vkDestroyBuffer(device, out.impl.buffer, nullptr);
			vkFreeMemory(device, out.impl.memory, nullptr);
			return Page();
		}

		return out;
	}

	void Texture::Impl_Unload()
	{
		if (!isWrapped)
		{
			if (impl.imageView_SRV) vkDestroyImageView(context->GetVulkanDevice(), impl.imageView_SRV, nullptr);
			if (impl.imageView_UAV) vkDestroyImageView(context->GetVulkanDevice(), impl.imageView_UAV, nullptr);
			if (impl.imageView_RTV_DSV) vkDestroyImageView(context->GetVulkanDevice(), impl.imageView_RTV_DSV, nullptr);

			for (size_t i = 0; i < readMapped.size(); i++)
			{
				vkUnmapMemory(context->GetVulkanDevice(), impl.memory[i]);
			}
			for (size_t i = 0; i < impl.image.size(); i++)
			{
				if (impl.image[i]) vkDestroyImage(context->GetVulkanDevice(), impl.image[i], nullptr);
			}
			for (size_t i = 0; i < impl.memory.size(); i++)
			{
				if (impl.memory[i]) vkFreeMemory(context->GetVulkanDevice(), impl.memory[i], nullptr);
			}
		}

		impl.image.clear();
		impl.memory.clear();

		impl.imageView_SRV = VK_NULL_HANDLE;
		impl.imageView_UAV = VK_NULL_HANDLE;
		impl.imageView_RTV_DSV = VK_NULL_HANDLE;

	}

	DetailedResult Texture::Impl_Load(const IGLOContext& context, const TextureDesc& desc)
	{
		FormatInfo formatInfo = GetFormatInfo(desc.format);
		bool isMultisampledCubemap = (desc.isCubemap && desc.msaa != MSAA::Disabled);

		VkDevice device = context.GetVulkanDevice();
		VkPhysicalDevice physicalDevice = context.GetVulkanPhysicalDevice();
		DescriptorHeap& heap = context.GetDescriptorHeap();

		VkFormat vkFormat = ConvertToVulkanFormat(desc.format);
		if (vkFormat == VK_FORMAT_UNDEFINED)
		{
			return DetailedResult::MakeFail(ToString("This iglo format is not supported in Vulkan: ", (uint32_t)desc.format, "."));
		}

		uint32_t numImages = (desc.usage == TextureUsage::Readable) ? context.GetMaxFramesInFlight() : 1;

		impl.image.resize(numImages, VK_NULL_HANDLE);
		impl.memory.resize(numImages, VK_NULL_HANDLE);

		VkImageUsageFlags imageUsage = 0;
		switch (desc.usage)
		{
		case TextureUsage::Default:
			imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
			imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			break;

		case TextureUsage::Readable:
			imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			break;

		case TextureUsage::UnorderedAccess:
			imageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
			imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
			imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			break;

		case TextureUsage::RenderTexture:
			imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
			imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			break;

		case TextureUsage::DepthBuffer:
			imageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
			imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			break;

		default:
			throw std::invalid_argument("Invalid texture usage.");
		}

		VkMemoryPropertyFlags memProperties = 0;
		if (desc.usage == TextureUsage::Readable)
		{
			memProperties |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			memProperties |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}
		else
		{
			memProperties |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		}


		for (uint32_t i = 0; i < numImages; i++)
		{
			VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = desc.extent.width;
			imageInfo.extent.height = desc.extent.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = desc.mipLevels;
			imageInfo.arrayLayers = desc.numFaces;
			imageInfo.format = vkFormat;
			imageInfo.tiling = (desc.usage == TextureUsage::Readable) ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = imageUsage;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.samples = (VkSampleCountFlagBits)desc.msaa;

			imageInfo.flags = 0;
			if (desc.isCubemap) imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			if (desc.overrideSRVFormat != Format::None) imageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

			VkResult result = vkCreateImage(device, &imageInfo, nullptr, &impl.image[i]);
			if (result != VK_SUCCESS)
			{
				return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreateImage", result));
			}

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(device, impl.image[i], &memRequirements);

			VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
			allocInfo.allocationSize = memRequirements.size;
			std::optional<uint32_t> memoryType = FindVulkanMemoryType(physicalDevice, memRequirements.memoryTypeBits, memProperties);
			if (!memoryType)
			{
				return DetailedResult::MakeFail("Failed to find suitable memory type.");
			}
			allocInfo.memoryTypeIndex = memoryType.value();

			result = vkAllocateMemory(device, &allocInfo, nullptr, &impl.memory[i]);
			if (result != VK_SUCCESS)
			{
				return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkAllocateMemory", result));
			}

			vkBindImageMemory(device, impl.image[i], impl.memory[i], 0);

			// Map memory for readable textures
			if (desc.usage == TextureUsage::Readable)
			{
				void* mappedPtr = nullptr;

				result = vkMapMemory(device, impl.memory[i], 0, memRequirements.size, 0, &mappedPtr);
				if (result != VK_SUCCESS)
				{
					return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkMapMemory", result));
				}

				readMapped.push_back(mappedPtr);
			}
		}

		bool createSRV = (!isMultisampledCubemap && desc.usage != TextureUsage::Readable);
		bool createUAV = (desc.usage == TextureUsage::UnorderedAccess);

		if (!desc.createDescriptors)
		{
			createSRV = false;
			createUAV = false;
		}

		auto CreateImageView = [&](VkImage image, VkImageViewType viewType, VkFormat format,
			VkImageAspectFlags aspect, uint32_t baseMip = 0, uint32_t mipCount = VK_REMAINING_MIP_LEVELS) -> VkImageView
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = viewType;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = aspect;
			viewInfo.subresourceRange.baseMipLevel = baseMip;
			viewInfo.subresourceRange.levelCount = mipCount;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = desc.numFaces;
			viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			VkImageView imageView = VK_NULL_HANDLE;
			if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
			{
				return VK_NULL_HANDLE;
			}
			return imageView;
		};

		if (createSRV)
		{
			// Allocate descriptor
			srvDescriptor = heap.AllocatePersistent(DescriptorType::Texture_SRV);
			if (srvDescriptor.IsNull()) return DetailedResult::MakeFail("Failed to allocate descriptor.");

			VkImageViewType srvViewType = VK_IMAGE_VIEW_TYPE_2D;
			if (desc.isCubemap)
			{
				srvViewType = (desc.numFaces == 6) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			}
			else if (desc.numFaces > 1)
			{
				srvViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			}

			VkFormat srvFormat = vkFormat;
			VkImageAspectFlagBits srvAspect = formatInfo.isDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

			if (desc.overrideSRVFormat != Format::None)
			{
				srvFormat = ConvertToVulkanFormat(desc.overrideSRVFormat);
				srvAspect = GetFormatInfo(desc.overrideSRVFormat).isDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			}

			// SRV
			impl.imageView_SRV = CreateImageView(impl.image[0], srvViewType, srvFormat, srvAspect);
			if (!impl.imageView_SRV) return DetailedResult::MakeFail("Failed to create SRV image view.");
			heap.WriteImageDescriptor(srvDescriptor, impl.imageView_SRV, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		if (createUAV)
		{
			// Allocate descriptor
			uavDescriptor = heap.AllocatePersistent(DescriptorType::Texture_UAV);
			if (uavDescriptor.IsNull()) return DetailedResult::MakeFail("Failed to allocate descriptor.");

			VkImageViewType viewType = (desc.numFaces == 1) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;

			// UAV
			impl.imageView_UAV = CreateImageView(impl.image[0], viewType, vkFormat, VK_IMAGE_ASPECT_COLOR_BIT);
			if (!impl.imageView_UAV) return DetailedResult::MakeFail("Failed to create UAV image view.");
			heap.WriteImageDescriptor(uavDescriptor, impl.imageView_UAV, VK_IMAGE_LAYOUT_GENERAL);
		}

		if (desc.usage == TextureUsage::RenderTexture ||
			desc.usage == TextureUsage::DepthBuffer)
		{
			VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
			if (desc.isCubemap)
			{
				viewType = (desc.numFaces == 6) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			}
			else if (desc.numFaces > 1)
			{
				viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			}

			VkImageAspectFlags aspectFlags = 0;
			if (desc.usage == TextureUsage::RenderTexture)
			{
				aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
				impl.imageView_RTV_DSV = CreateImageView(impl.image[0], viewType, vkFormat, aspectFlags);
				if (!impl.imageView_RTV_DSV) return DetailedResult::MakeFail("Failed to create RTV image view.");
			}
			else if (desc.usage == TextureUsage::DepthBuffer)
			{
				aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
				if (formatInfo.hasStencilComponent)
				{
					aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
				}
				impl.imageView_RTV_DSV = CreateImageView(impl.image[0], viewType, vkFormat, aspectFlags);
				if (!impl.imageView_RTV_DSV) return DetailedResult::MakeFail("Failed to create DSV image view.");
			}
		}

		return DetailedResult::MakeSuccess();
	}

	VkImage Texture::GetVulkanImage() const
	{
		if (!isLoaded) return VK_NULL_HANDLE;
		if (impl.image.size() == 0) throw std::runtime_error("This should be impossible.");
		switch (desc.usage)
		{
		case TextureUsage::Default:
		case TextureUsage::UnorderedAccess:
		case TextureUsage::RenderTexture:
		case TextureUsage::DepthBuffer:
			return impl.image[0];
		case TextureUsage::Readable:
			return impl.image[context->GetFrameIndex()];
		default:
			throw std::runtime_error("Invalid texture usage.");
		}
	}

	VkDeviceMemory Texture::GetVulkanMemory() const
	{
		if (!isLoaded) return VK_NULL_HANDLE;
		if (impl.memory.size() == 0) throw std::runtime_error("This should be impossible.");
		switch (desc.usage)
		{
		case TextureUsage::Default:
		case TextureUsage::UnorderedAccess:
		case TextureUsage::RenderTexture:
		case TextureUsage::DepthBuffer:
			return impl.memory[0];
		case TextureUsage::Readable:
			return impl.memory[context->GetFrameIndex()];
		default:
			throw std::runtime_error("Invalid texture usage.");
		}
	}

	void Buffer::Impl_Unload()
	{
		for (size_t i = 0; i < mapped.size(); i++)
		{
			assert(i < impl.memory.size());
			vkUnmapMemory(context->GetVulkanDevice(), impl.memory[i]);
		}
		for (size_t i = 0; i < impl.buffer.size(); i++)
		{
			if (impl.buffer[i]) vkDestroyBuffer(context->GetVulkanDevice(), impl.buffer[i], nullptr);
		}
		for (size_t i = 0; i < impl.memory.size(); i++)
		{
			if (impl.memory[i]) vkFreeMemory(context->GetVulkanDevice(), impl.memory[i], nullptr);
		}

		impl.buffer.clear();
		impl.memory.clear();
	}

	DetailedResult Buffer::Impl_InternalLoad(const IGLOContext& context, uint64_t size, uint32_t stride,
		uint32_t numElements, BufferUsage usage, BufferType type)
	{
		VkDevice device = context.GetVulkanDevice();
		VkPhysicalDevice physicalDevice = context.GetVulkanPhysicalDevice();
		DescriptorHeap& heap = context.GetDescriptorHeap();

		uint32_t numBuffers = 1;
		if (usage == BufferUsage::Readable ||
			usage == BufferUsage::Dynamic)
		{
			numBuffers = context.GetMaxFramesInFlight();
		}

		impl.buffer.resize(numBuffers, VK_NULL_HANDLE);
		impl.memory.resize(numBuffers, VK_NULL_HANDLE);

		uint64_t bufferSize = size;
		if (type == BufferType::ShaderConstant)
		{
			bufferSize = AlignUp(size, context.GetGraphicsSpecs().bufferPlacementAlignments.constant);
		}

		VkBufferUsageFlags usageFlags = 0;
		switch (type)
		{
		case BufferType::VertexBuffer: usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; break;
		case BufferType::IndexBuffer: usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT; break;
		case BufferType::StructuredBuffer: usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; break;
		case BufferType::RawBuffer: usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; break;
		case BufferType::ShaderConstant: usageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; break;
		default:
			throw std::invalid_argument("Invalid buffer type.");
		}

		switch (usage)
		{
		case BufferUsage::Default:
			usageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			break;

		case BufferUsage::Dynamic:
			break;

		case BufferUsage::Readable:
			usageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			break;

		case BufferUsage::UnorderedAccess:
			usageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			usageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			break;

		default:
			throw std::invalid_argument("Invalid buffer usage.");
		}

		VkMemoryPropertyFlags memoryProperties = 0;
		if (usage == BufferUsage::Dynamic ||
			usage == BufferUsage::Readable)
		{
			memoryProperties |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			memoryProperties |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}
		else
		{
			memoryProperties |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		}

		for (uint32_t i = 0; i < numBuffers; i++)
		{
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = bufferSize;
			bufferInfo.usage = usageFlags;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &impl.buffer[i]);
			if (result != VK_SUCCESS)
			{
				return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreateBuffer", result));
			}

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(device, impl.buffer[i], &memRequirements);

			std::optional<uint32_t> memType = FindVulkanMemoryType(physicalDevice, memRequirements.memoryTypeBits, memoryProperties);
			if (!memType)
			{
				return DetailedResult::MakeFail("FindVulkanMemoryType failed.");
			}

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = memType.value();

			result = vkAllocateMemory(device, &allocInfo, nullptr, &impl.memory[i]);
			if (result != VK_SUCCESS)
			{
				return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkAllocateMemory", result));
			}

			vkBindBufferMemory(device, impl.buffer[i], impl.memory[i], 0);

			if (usage == BufferUsage::Dynamic ||
				usage == BufferUsage::Readable)
			{
				void* mappedPtr = nullptr;

				result = vkMapMemory(device, impl.memory[i], 0, bufferSize, 0, &mappedPtr);
				if (result != VK_SUCCESS)
				{
					return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkMapMemory", result));
				}

				mapped.push_back(mappedPtr);
			}
		}

		uint32_t numDescriptors = 0;
		if (usage == BufferUsage::Default) numDescriptors = 1;
		else if (usage == BufferUsage::UnorderedAccess) numDescriptors = 1;
		else if (usage == BufferUsage::Dynamic) numDescriptors = context.GetMaxFramesInFlight();

		// SRV or CBV
		for (uint32_t i = 0; i < numDescriptors; i++)
		{
			if (type == BufferType::StructuredBuffer ||
				type == BufferType::RawBuffer ||
				type == BufferType::ShaderConstant)
			{
				DescriptorType descriptorType = (type == BufferType::ShaderConstant)
					? DescriptorType::ConstantBuffer_CBV
					: DescriptorType::RawOrStructuredBuffer_SRV_UAV;

				Descriptor allocatedDescriptor = heap.AllocatePersistent(descriptorType);
				if (allocatedDescriptor.IsNull()) return DetailedResult::MakeFail("Failed to allocate descriptor.");

				heap.WriteBufferDescriptor(allocatedDescriptor, impl.buffer[i], 0, size);

				descriptor_SRV_or_CBV.push_back(allocatedDescriptor);
			}
		}

		// UAV
		if (usage == BufferUsage::UnorderedAccess)
		{
			descriptor_UAV = heap.AllocatePersistent(DescriptorType::RawOrStructuredBuffer_SRV_UAV);
			if (descriptor_UAV.IsNull()) return DetailedResult::MakeFail("Failed to allocate UAV descriptor.");

			heap.WriteBufferDescriptor(descriptor_UAV, impl.buffer[0], 0, size);
		}

		return DetailedResult::MakeSuccess();
	}

	VkBuffer Buffer::GetVulkanBuffer() const
	{
		if (!isLoaded) return nullptr;
		switch (usage)
		{
		case BufferUsage::Default:
		case BufferUsage::UnorderedAccess:
			return impl.buffer[0];
		case BufferUsage::Readable:
			return impl.buffer[context->GetFrameIndex()];
		case BufferUsage::Dynamic:
			return impl.buffer[dynamicSetCounter];
		default:
			throw std::runtime_error("This should be impossible.");
		}
	}

	VkDeviceMemory Buffer::GetVulkanMemory() const
	{
		if (!isLoaded) return nullptr;
		switch (usage)
		{
		case BufferUsage::Default:
		case BufferUsage::UnorderedAccess:
			return impl.memory[0];
		case BufferUsage::Readable:
			return impl.memory[context->GetFrameIndex()];
		case BufferUsage::Dynamic:
			return impl.memory[dynamicSetCounter];
		default:
			throw std::runtime_error("This should be impossible.");
		}
	}

	DetailedResult Sampler::Impl_Load(const IGLOContext& context, const SamplerDesc& desc)
	{
		VkDevice device = context.GetVulkanDevice();
		DescriptorHeap& heap = context.GetDescriptorHeap();

		descriptor = heap.AllocatePersistent(DescriptorType::Sampler);
		if (descriptor.IsNull())
		{
			return DetailedResult::MakeFail("Failed to allocate sampler descriptor.");
		}

		VkClearColorValue borderColor = {};
		borderColor.float32[0] = desc.borderColor.red;
		borderColor.float32[1] = desc.borderColor.green;
		borderColor.float32[2] = desc.borderColor.blue;
		borderColor.float32[3] = desc.borderColor.alpha;

		VkSamplerCustomBorderColorCreateInfoEXT customBorderColorInfo = {};
		customBorderColorInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT;
		customBorderColorInfo.customBorderColor = borderColor;
		customBorderColorInfo.format = VK_FORMAT_UNDEFINED;

		VulkanTextureFilter convertedFilter = ConvertToVulkanTextureFilter(desc.filter);

		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.pNext = &customBorderColorInfo;
		samplerInfo.magFilter = convertedFilter.magFilter;
		samplerInfo.minFilter = convertedFilter.minFilter;
		samplerInfo.mipmapMode = convertedFilter.mipmapMode;
		samplerInfo.anisotropyEnable = convertedFilter.anisotropyEnable;
		samplerInfo.maxAnisotropy = convertedFilter.maxAnisotropy;
		samplerInfo.compareEnable = convertedFilter.compareEnable;
		samplerInfo.compareOp = (VkCompareOp)desc.comparisonFunc;
		samplerInfo.addressModeU = (VkSamplerAddressMode)desc.wrapU;
		samplerInfo.addressModeV = (VkSamplerAddressMode)desc.wrapV;
		samplerInfo.addressModeW = (VkSamplerAddressMode)desc.wrapW;
		samplerInfo.minLod = std::max(0.0f, desc.minLOD);
		samplerInfo.maxLod = std::min(VK_LOD_CLAMP_NONE, desc.maxLOD);
		samplerInfo.mipLodBias = desc.mipMapLODBias;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_CUSTOM_EXT;

		VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &vkSampler);
		if (result != VK_SUCCESS)
		{
			return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreateSampler", result));
		}

		heap.WriteSamplerDescriptor(descriptor, vkSampler);

		return DetailedResult::MakeSuccess();
	}

	uint32_t IGLOContext::Impl_GetMaxMultiSampleCount(Format textureFormat) const
	{
		VkPhysicalDevice physicalDevice = GetVulkanPhysicalDevice();
		VkFormat vkFormat = ConvertToVulkanFormat(textureFormat);
		FormatInfo formatInfo = GetFormatInfo(textureFormat);

		VkImageUsageFlags usage = formatInfo.isDepthFormat
			? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
			: VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageFormatProperties formatProps = {};
		VkResult result = vkGetPhysicalDeviceImageFormatProperties(physicalDevice, vkFormat,
			VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &formatProps);

		uint32_t maxMSAA = 1;
		if (result == VK_SUCCESS)
		{
			VkSampleCountFlags counts = formatProps.sampleCounts;

			if (counts & VK_SAMPLE_COUNT_2_BIT) maxMSAA = 2;
			if (counts & VK_SAMPLE_COUNT_4_BIT) maxMSAA = 4;
			if (counts & VK_SAMPLE_COUNT_8_BIT) maxMSAA = 8;
			if (counts & VK_SAMPLE_COUNT_16_BIT) maxMSAA = 16;
			if (counts & VK_SAMPLE_COUNT_32_BIT) maxMSAA = 16; // Capped to x16
			if (counts & VK_SAMPLE_COUNT_64_BIT) maxMSAA = 16; // Capped to x16
		}

		return maxMSAA;
	}

	uint32_t IGLOContext::GetCurrentBackBufferIndex() const
	{
		return commandQueue.GetCurrentBackBufferIndex();
	}

	void IGLOContext::SetPresentMode(PresentMode presentMode)
	{
		if (swapChain.presentMode == presentMode) return;

		commandQueue.WaitForIdle();
		DetailedResult result = CreateSwapChain(swapChain.extent, swapChain.format,
			swapChain.numBackBuffers, numFramesInFlight, presentMode);
		if (!result)
		{
			Log(LogType::Error, "Failed to change present mode. Reason: " + result.errorMessage);
		}
	}

	void IGLOContext::DestroySwapChainResources()
	{
		// Wrapped textures don't free any graphical resources upon unloading, so we must do it manually.
		// Don't free any VkImage from the wrapped textures, as they belong to the swap chain.
		for (size_t i = 0; i < swapChain.wrapped.size(); i++)
		{
			VkImageView imageView_RTV = swapChain.wrapped[i]->GetVulkanImageView_RTV_DSV();
			if (imageView_RTV) vkDestroyImageView(graphics.device, imageView_RTV, nullptr);
		}
		for (size_t i = 0; i < swapChain.wrapped_sRGB_opposite.size(); i++)
		{
			VkImageView imageView_RTV = swapChain.wrapped_sRGB_opposite[i]->GetVulkanImageView_RTV_DSV();
			if (imageView_RTV) vkDestroyImageView(graphics.device, imageView_RTV, nullptr);
		}
		swapChain = SwapChainInfo();
	}

	DetailedResult IGLOContext::CreateSwapChain(Extent2D extent, Format format, uint32_t numBackBuffers,
		uint32_t numFramesInFlight, PresentMode presentMode)
	{
		vkDeviceWaitIdle(graphics.device);

		graphics.validSwapChain = false;

		// Handle resize
		Extent2D cappedExtent = extent;
		VkSurfaceCapabilitiesKHR caps;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(graphics.physicalDevice, graphics.surface, &caps);
		if (extent.width > caps.minImageExtent.width ||
			extent.height > caps.minImageExtent.height ||
			extent.width < caps.minImageExtent.width ||
			extent.height < caps.minImageExtent.height)
		{
			cappedExtent = Extent2D(caps.minImageExtent.width, caps.minImageExtent.height);

			// If we can't create a valid swapchain yet
			if (cappedExtent.width == 0 || cappedExtent.height == 0)
			{
				return DetailedResult::MakeFail("Min/max image caps are invalid at the moment.");
			}

			Event resizeEvent;
			resizeEvent.type = EventType::Resize;
			resizeEvent.resize.width = cappedExtent.width;
			resizeEvent.resize.height = cappedExtent.height;
			eventQueue.push(resizeEvent);
		}

		FormatInfo formatInfo = GetFormatInfo(format);

		VkImageFormatListCreateInfo formatListInfo = {};
		formatListInfo.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = graphics.surface;
		createInfo.minImageCount = numBackBuffers;
		createInfo.imageFormat = ConvertToVulkanFormat(format);
		createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		createInfo.imageExtent = { cappedExtent.width, cappedExtent.height };
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
		createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = graphics.swapChain; // Important
		createInfo.presentMode = (VkPresentModeKHR)presentMode;

		if (formatInfo.sRGB_opposite != Format::None)
		{
			VkFormat formats[2] = {createInfo.imageFormat, ConvertToVulkanFormat(formatInfo.sRGB_opposite)};
			formatListInfo.viewFormatCount = 2;
			formatListInfo.pViewFormats = formats;

			createInfo.pNext = &formatListInfo;
			createInfo.flags |= VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;
		}

		VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
		VkResult result = vkCreateSwapchainKHR(graphics.device, &createInfo, nullptr, &newSwapchain);
		if (result != VK_SUCCESS)
		{
			return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreateSwapchainKHR", result));
		}

		DestroySwapChainResources();
		commandQueue.DestroySwapChainSemaphores();

		// Destroy old swapchain after creating new swapchain
		if (graphics.swapChain) vkDestroySwapchainKHR(graphics.device, graphics.swapChain, nullptr);
		graphics.swapChain = newSwapchain;

		// Retrieve swapchain images
		uint32_t imageCount = 0;
		vkGetSwapchainImagesKHR(graphics.device, graphics.swapChain, &imageCount, nullptr);
		if (imageCount < numBackBuffers) throw std::runtime_error("This should be impossible.");
		if (imageCount != numBackBuffers)
		{
			Log(LogType::Warning, ToString("Using ", imageCount, " backbuffers instead of the requested ", numBackBuffers, "."));
		}

		graphics.swapChainUsesMinImageCount = (imageCount == caps.minImageCount);

		commandQueue.RecreateSwapChainSemaphores(numFramesInFlight, imageCount);

		swapChain.extent = cappedExtent;
		swapChain.format = format;
		swapChain.presentMode = presentMode;
		swapChain.numBackBuffers = imageCount;
		swapChain.renderTargetDesc = RenderTargetDesc({ format });
		swapChain.renderTargetDesc_sRGB_opposite = (formatInfo.sRGB_opposite != Format::None)
			? RenderTargetDesc({ formatInfo.sRGB_opposite })
			: RenderTargetDesc({});

		std::vector<VkImage> swapchainImages(imageCount);
		result = vkGetSwapchainImagesKHR(graphics.device, graphics.swapChain, &imageCount, swapchainImages.data());
		if (result != VK_SUCCESS)
		{
			return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkGetSwapchainImagesKHR", result));
		}

		// Wrap backbuffers
		for (uint32_t i = 0; i < imageCount; i++)
		{
			VkImageView imageView = VK_NULL_HANDLE;
			CreateVulkanImageViewForSwapChain(graphics.device, swapchainImages[i], createInfo.imageFormat, &imageView);

			WrappedTextureDesc desc;
			desc.textureDesc.extent = cappedExtent;
			desc.textureDesc.format = format;
			desc.textureDesc.usage = TextureUsage::RenderTexture;
			desc.impl.image = { swapchainImages[i] };
			desc.impl.memory = { VK_NULL_HANDLE };
			desc.impl.imageView_RTV_DSV = imageView;

			std::unique_ptr<Texture> currentBackBuffer = std::make_unique<Texture>();
			currentBackBuffer->LoadAsWrapped(*this, desc);
			swapChain.wrapped.push_back(std::move(currentBackBuffer));

			// sRGB opposite views
			if (formatInfo.sRGB_opposite != Format::None)
			{
				VkImageView imageView_sRGB_opposite = VK_NULL_HANDLE;
				CreateVulkanImageViewForSwapChain(graphics.device, swapchainImages[i],
					ConvertToVulkanFormat(formatInfo.sRGB_opposite), &imageView_sRGB_opposite);
				desc.impl.imageView_RTV_DSV = imageView_sRGB_opposite;

				std::unique_ptr<Texture> backBuffer_sRGB_opposite = std::make_unique<Texture>();
				backBuffer_sRGB_opposite->LoadAsWrapped(*this, desc);
				swapChain.wrapped_sRGB_opposite.push_back(std::move(backBuffer_sRGB_opposite));
			}
		}

		// Acquire next image
		result = commandQueue.AcquireNextVulkanSwapChainImage(graphics.device, graphics.swapChain,
			graphics.swapChainUsesMinImageCount ? 0 : UINT64_MAX);

		graphics.validSwapChain = false;
		if (result == VK_SUCCESS)
		{
			graphics.validSwapChain = true;
		}
		else if (result == VK_SUBOPTIMAL_KHR)
		{
			graphics.validSwapChain = true;
			Log(LogType::Info, "Acquired suboptimal image at swapchain creation.");
		}
		else if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			Log(LogType::Info, "Acquired out of date image at swapchain creation.");
		}
		else
		{
			Log(LogType::Error, "Failed to acquire swapchain image at swapchain creation (" + vk::to_string((vk::Result)result) + ").");
		}

		return DetailedResult::MakeSuccess();
	}

	void IGLOContext::HandleVulkanSwapChainResult(VkResult result, const std::string& scenario)
	{
		switch (result)
		{
		case VK_SUCCESS:
			graphics.validSwapChain = true;
			break;

		case VK_SUBOPTIMAL_KHR:
		case VK_ERROR_OUT_OF_DATE_KHR:
		case VK_NOT_READY:
			graphics.validSwapChain = false;
			break;

		case VK_ERROR_DEVICE_LOST:
			graphics.validSwapChain = false;
			Log(LogType::Error, "Device removal detected at " + scenario + "!");
			if (callbackOnDeviceRemoved) callbackOnDeviceRemoved("The device was lost.");
			break;

		default:
			graphics.validSwapChain = false;
			Log(LogType::Error, scenario + " failed (" + vk::to_string((vk::Result)result) + ").");
			break;
		}
	}

	DetailedResult IGLOContext::Impl_LoadGraphicsDevice()
	{
		const char* strEnsureLatestDrivers = "Ensure that you have the latest graphics drivers installed.";

		// App
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = NULL;
		appInfo.pApplicationName = "iglo";
		appInfo.applicationVersion = VK_MAKE_VERSION(IGLO_VERSION_MAJOR, IGLO_VERSION_MINOR, IGLO_VERSION_PATCH);
		appInfo.pEngineName = "iglo";
		appInfo.engineVersion = VK_MAKE_VERSION(IGLO_VERSION_MAJOR, IGLO_VERSION_MINOR, IGLO_VERSION_PATCH);
		appInfo.apiVersion = vulkanVersion;

		const std::vector<const char*> instanceExtensions =
		{
			VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
			VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifdef __linux__
			VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
#ifdef _DEBUG
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
		};

		const std::vector<const char*> deviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME,
			VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
			VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
			VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
			VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME,
			VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME,
			VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME,
			VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME,
#ifdef IGLO_VULKAN_ENABLE_CONSERVATIVE_RASTERIZATION
			VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME,
#endif
		};

		// Check available instance extensions
		{
			uint32_t extCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
			std::vector<VkExtensionProperties> extensions(extCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extensions.data());

			std::vector<const char*> missingExtensions;
			for (const auto& reqExt : instanceExtensions)
			{
				bool found = false;
				for (const auto& availExt : extensions)
				{
					if (strcmp(reqExt, availExt.extensionName) == 0)
					{
						found = true;
						break;
					}
				}

				if (!found)
				{
					missingExtensions.push_back(reqExt);
				}
			}

			if (!missingExtensions.empty())
			{
				std::string errStr = "Missing required Vulkan instance extensions:\n\n";
				for (const auto& missing : missingExtensions)
				{
					errStr.append(missing);
					errStr.append("\n");
				}
				errStr.append("\n");
				errStr.append(strEnsureLatestDrivers);
				return DetailedResult::MakeFail(errStr);
			}
		}

		// Instance
		VkInstanceCreateInfo instanceInfo = {};
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pApplicationInfo = &appInfo;
		if (instanceExtensions.size() > 0)
		{
			instanceInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
			instanceInfo.ppEnabledExtensionNames = instanceExtensions.data();
		}

#ifdef _DEBUG
		// Debug messenger
		VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugInfo.pfnUserCallback = VulkanDebugCallback;
		debugInfo.pUserData = nullptr;

		// Validation layer
		{
			const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
			bool present = false;
			uint32_t layerCount = 0;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
			for (size_t i = 0; i < availableLayers.size(); i++)
			{
				if (strcmp(availableLayers[i].layerName, validationLayerName) == 0)
				{
					present = true;
					break;
				}
			}
			if (present)
			{
				instanceInfo.enabledLayerCount = 1;
				instanceInfo.ppEnabledLayerNames = &validationLayerName;
				instanceInfo.pNext = &debugInfo; // Link debug messenger to instance creation
			}
			else
			{
				return DetailedResult::MakeFail(ToString("The requested validation layer ", validationLayerName, " is not present."));
			}
		}
#endif

		VkResult result = vkCreateInstance(&instanceInfo, nullptr, &graphics.instance);
		if (result != VK_SUCCESS)
		{
			return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreateInstance", result));
		}

#ifdef _DEBUG
		// Create debug messenger
		result = CreateDebugUtilsMessengerEXT(graphics.instance, &debugInfo, nullptr, &graphics.debugMessenger);
		if (result != VK_SUCCESS)
		{
			return DetailedResult::MakeFail(CreateVulkanErrorMsg("CreateDebugUtilsMessengerEXT", result));
		}
#endif

#ifdef _WIN32
		// Create surface
		{
			VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = {};
			win32SurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			win32SurfaceInfo.hwnd = window.hwnd;
			win32SurfaceInfo.hinstance = GetModuleHandle(nullptr);
			result = vkCreateWin32SurfaceKHR(graphics.instance, &win32SurfaceInfo, nullptr, &graphics.surface);
			if (result != VK_SUCCESS)
			{
				return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreateWin32SurfaceKHR", result));
			}
		}
#endif
#ifdef __linux__
		// Create surface
		{
			VkXlibSurfaceCreateInfoKHR xlibSurfaceInfo = {};
			xlibSurfaceInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
			xlibSurfaceInfo.dpy = window.display;
			xlibSurfaceInfo.window = window.handle;
			VkResult result = vkCreateXlibSurfaceKHR(graphics.instance, &xlibSurfaceInfo, nullptr, &graphics.surface);
			if (result != VK_SUCCESS)
			{
				return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreateXlibSurfaceKHR", result));
			}
		}
#endif

		// Find most suitable physical device
		{
			uint32_t physicalDeviceCount = 0;
			vkEnumeratePhysicalDevices(graphics.instance, &physicalDeviceCount, nullptr);
			if (physicalDeviceCount == 0)
			{
				return DetailedResult::MakeFail("No Vulkan compatible device found.");
			}
			std::vector<VkPhysicalDevice> physicalDeviceList(physicalDeviceCount);
			vkEnumeratePhysicalDevices(graphics.instance, &physicalDeviceCount, physicalDeviceList.data());

			std::vector<DetailedResult> suitable;
			for (size_t i = 0; i < physicalDeviceList.size(); i++)
			{
				suitable.push_back(IsVulkanPhysicalDeviceSuitable(physicalDeviceList[i], graphics.surface, deviceExtensions));
			}

			uint32_t numSuitableDevices = 0;
			uint64_t bestTier = 0; // Has higher priority than score.
			uint64_t bestScore = 0;
			size_t bestIndex = 0;
			for (size_t i = 0; i < physicalDeviceList.size(); i++)
			{
				assert(physicalDeviceList.size() == suitable.size());

				// Device must be suitable
				if (!suitable[i]) continue;

				numSuitableDevices++;

				const VkPhysicalDevice& physicalDevice = physicalDeviceList[i];

				VkPhysicalDeviceProperties props;
				VkPhysicalDeviceDescriptorIndexingFeatures indexingFeats = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, nullptr };
				VkPhysicalDeviceFeatures2 feats = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &indexingFeats };
				VulkanQueueFamilies fams = GetVulkanQueueFamilies(physicalDevice, graphics.surface);

				vkGetPhysicalDeviceProperties(physicalDevice, &props);
				vkGetPhysicalDeviceFeatures2(physicalDevice, &feats);

				uint64_t tier = 0;
				uint64_t score = 0;

				// If dedicated compute/transfer queue, increase score
				if (fams.computeFamily) score += 10000;
				if (fams.transferFamily) score += 5000;

				score += props.limits.maxImageDimension2D;

				if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					tier = 10;
				}
				else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
				{
					tier = 5;
				}
				else
				{
					tier = 1;
				}

				// If a better tier has been found, ignore this one
				if (tier < bestTier) continue;

				// If this is a better tier OR an equal tier but with a better score...
				if (tier > bestTier || (score > bestScore))
				{
					bestTier = tier;
					bestScore = score;
					bestIndex = i;
				}
			}
			if (numSuitableDevices == 0)
			{
				std::string errStr = "Unable to find a device that supports the required features.\n";
				for (size_t i = 0; i < suitable.size(); i++)
				{
					VkPhysicalDeviceProperties props;
					vkGetPhysicalDeviceProperties(physicalDeviceList[i], &props);

					errStr.append(ToString("\n", props.deviceName, " (deviceID ", props.deviceID, "): "));
					errStr.append(suitable[i].errorMessage);
					errStr.append("\n");
				}
				errStr.append("\n");
				errStr.append(strEnsureLatestDrivers);
				return DetailedResult::MakeFail(errStr);
			}

			graphics.physicalDevice = physicalDeviceList[bestIndex];

			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(graphics.physicalDevice, &props);

			std::string vulkanVersionString = ToString(VK_API_VERSION_MAJOR(props.apiVersion), ".", VK_API_VERSION_MINOR(props.apiVersion));
			graphicsSpecs.apiName = "Vulkan " + vulkanVersionString;
			graphicsSpecs.rendererName = props.deviceName;
			graphicsSpecs.vendorName = GetGpuVendorNameFromID(props.vendorID);
			graphicsSpecs.maxTextureDimension = props.limits.maxImageDimension2D;

			graphicsSpecs.bufferPlacementAlignments.vertexOrIndexBuffer = 4;
			graphicsSpecs.bufferPlacementAlignments.rawOrStructuredBuffer = (uint32_t)props.limits.minStorageBufferOffsetAlignment;
			graphicsSpecs.bufferPlacementAlignments.constant = (uint32_t)props.limits.minUniformBufferOffsetAlignment;
			graphicsSpecs.bufferPlacementAlignments.texture = 16;
			graphicsSpecs.bufferPlacementAlignments.textureRowPitch = (uint32_t)props.limits.optimalBufferCopyRowPitchAlignment;

			// Check supported present modes
			uint32_t presentModeCount = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(graphics.physicalDevice, graphics.surface, &presentModeCount, nullptr);
			if (presentModeCount == 0)
			{
				return DetailedResult::MakeFail("No present modes are supported. This should be impossible.");
			}
			std::vector<VkPresentModeKHR> presentModes(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(graphics.physicalDevice, graphics.surface, &presentModeCount, presentModes.data());

			graphicsSpecs.supportedPresentModes = SupportedPresentModes();
			for (const VkPresentModeKHR& mode : presentModes)
			{
				switch (mode)
				{
				case (VkPresentModeKHR)PresentMode::ImmediateWithTearing:
					graphicsSpecs.supportedPresentModes.immediateWithTearing = true;
					break;

				case (VkPresentModeKHR)PresentMode::Immediate:
					graphicsSpecs.supportedPresentModes.immediate = true;
					break;

				case (VkPresentModeKHR)PresentMode::Vsync:
					graphicsSpecs.supportedPresentModes.vsync = true;
					break;

				case (VkPresentModeKHR)PresentMode::VsyncRelaxed:
					graphicsSpecs.supportedPresentModes.vsyncRelaxed = true;
					break;

				default:
					break;
				}
			}
		}

		VulkanQueueFamilies fams = GetVulkanQueueFamilies(graphics.physicalDevice, graphics.surface);
		std::set<uint32_t> uniqueFams;
		if (fams.graphicsFamily) uniqueFams.insert(fams.graphicsFamily.value());
		if (fams.computeFamily) uniqueFams.insert(fams.computeFamily.value());
		if (fams.transferFamily) uniqueFams.insert(fams.transferFamily.value());
		if (fams.presentFamily) uniqueFams.insert(fams.presentFamily.value());

		std::vector<VkDeviceQueueCreateInfo> queueInfo;
		const float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueFams)
		{
			VkDeviceQueueCreateInfo tempInfo = {};
			tempInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			tempInfo.pQueuePriorities = &queuePriority;
			tempInfo.queueCount = 1; // Only 1 of each type of queue is ever needed.
			tempInfo.queueFamilyIndex = queueFamily;
			queueInfo.push_back(tempInfo);
		}

		//TODO: test code paths for when no dedicated compute and transfer queues exist.

		VkPhysicalDeviceLineRasterizationFeaturesKHR lineFeats = {};
		lineFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_KHR;
		lineFeats.rectangularLines = VK_TRUE;
		lineFeats.bresenhamLines = VK_TRUE;
		lineFeats.smoothLines = VK_TRUE;
		lineFeats.stippledRectangularLines = VK_TRUE;
		lineFeats.stippledBresenhamLines = VK_TRUE;
		lineFeats.stippledSmoothLines = VK_TRUE;

		VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR dynamicLocalReadFeats = {};
		dynamicLocalReadFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_LOCAL_READ_FEATURES_KHR;
		dynamicLocalReadFeats.pNext = &lineFeats;
		dynamicLocalReadFeats.dynamicRenderingLocalRead = VK_TRUE;

		VkPhysicalDeviceMaintenance5FeaturesKHR maintenance5Feats = {};
		maintenance5Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR;
		maintenance5Feats.pNext = &dynamicLocalReadFeats;
		maintenance5Feats.maintenance5 = VK_TRUE;

		VkPhysicalDeviceCustomBorderColorFeaturesEXT borderColorFeats = {};
		borderColorFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT;
		borderColorFeats.pNext = &maintenance5Feats;
		borderColorFeats.customBorderColors = VK_TRUE;
		borderColorFeats.customBorderColorWithoutFormat = VK_TRUE;

		VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableFeats = {};
		mutableFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
		mutableFeats.pNext = &borderColorFeats;
		mutableFeats.mutableDescriptorType = VK_TRUE;

		VkPhysicalDeviceVulkan13Features feats13 = {};
		feats13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		feats13.pNext = &mutableFeats;
		feats13.synchronization2 = VK_TRUE;
		feats13.dynamicRendering = VK_TRUE;

		VkPhysicalDeviceVulkan12Features feats12 = {};
		feats12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		feats12.pNext = &feats13;
		feats12.runtimeDescriptorArray = VK_TRUE;
		feats12.descriptorBindingPartiallyBound = VK_TRUE;
		feats12.descriptorBindingVariableDescriptorCount = VK_TRUE;
		feats12.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
		feats12.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
		feats12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		feats12.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
		feats12.timelineSemaphore = VK_TRUE;

		VkPhysicalDeviceFeatures2 feats2 = {};
		feats2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		feats2.pNext = &feats12;
		feats2.features.independentBlend = VK_TRUE;
		feats2.features.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo deviceInfo = {};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.queueCreateInfoCount = (uint32_t)queueInfo.size();
		deviceInfo.pQueueCreateInfos = queueInfo.data();
		deviceInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
		deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
		deviceInfo.pNext = &feats2;

		result = vkCreateDevice(graphics.physicalDevice, &deviceInfo, nullptr, &graphics.device);
		if (result != VK_SUCCESS)
		{
			return DetailedResult::MakeFail(CreateVulkanErrorMsg("vkCreateDevice", result));
		}

		return DetailedResult::MakeSuccess();
	}

	void IGLOContext::Impl_UnloadGraphicsDevice()
	{
		if (graphics.device)
		{
			if (graphics.swapChain) vkDestroySwapchainKHR(graphics.device, graphics.swapChain, nullptr);
			vkDestroyDevice(graphics.device, nullptr);
		}
		if (graphics.instance)
		{
			if (graphics.surface) vkDestroySurfaceKHR(graphics.instance, graphics.surface, nullptr);

#ifdef _DEBUG
			if (graphics.debugMessenger) DestroyDebugUtilsMessengerEXT(graphics.instance, graphics.debugMessenger, nullptr);
#endif

			vkDestroyInstance(graphics.instance, nullptr);
		}

		graphics = Impl_GraphicsContext();
	}

}

#endif