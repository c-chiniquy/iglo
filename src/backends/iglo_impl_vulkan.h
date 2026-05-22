
#pragma once

#include <vulkan/vulkan.h>

#ifdef _WIN32
#pragma comment(lib, "vulkan-1.lib")
#include <vulkan/vulkan_win32.h>
#endif
#ifdef __linux__
#include <vulkan/vulkan_xlib.h>
#endif

namespace ig
{
	struct Impl_GraphicsContext
	{
		VkInstance instance = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkSwapchainKHR swapChain = VK_NULL_HANDLE;
		bool validSwapChain = false;
		bool usesMemoryBudgetExt = false;
#ifndef NDEBUG
		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
#endif
	};

	struct Impl_CommandQueue
	{
		// One of each: Graphics, Compute, Copy
		std::array<VkQueue, 3> queues = { VK_NULL_HANDLE , VK_NULL_HANDLE , VK_NULL_HANDLE };
		std::array<uint32_t, 3> queueFamIndices = {};
		VkQueue presentQueue = VK_NULL_HANDLE;

		std::array<VkSemaphore, 3> timelineSemaphores = { VK_NULL_HANDLE , VK_NULL_HANDLE , VK_NULL_HANDLE };
		std::array<uint64_t, 3> fenceValues = {};

		std::vector<VkSemaphore> acquireSemaphores; // Per frame in flight
		std::vector<VkSemaphore> presentReadySemaphores; // Per swapchain image
		uint32_t frameIndex = 0;
		uint32_t numFrames = 0;
		uint32_t currentBackBufferIndex = 0;

		std::mutex receiptMutex;
		std::mutex waitMutex;
	};

	struct VulkanRenderInfo
	{
		VkRenderingInfo renderingInfo = {};
		std::array<VkRenderingAttachmentInfo, MAX_SIMULTANEOUS_RENDER_TARGETS> colorAttachments = {};
		VkRenderingAttachmentInfo depthAttachment = {};
		VkRenderingAttachmentInfo stencilAttachment = {};
	};

	struct Impl_CommandList
	{
		VulkanRenderInfo renderInfo;

		std::vector<VkCommandPool> commandPool; // One per frame
		std::vector<VkCommandBuffer> commandBuffer; // One per frame

		std::array<VkMemoryBarrier2, MAX_QUEUED_BARRIERS_PER_TYPE> globalBarriers = {};
		std::array<VkImageMemoryBarrier2, MAX_QUEUED_BARRIERS_PER_TYPE> textureBarriers = {};
		std::array<VkBufferMemoryBarrier2, MAX_QUEUED_BARRIERS_PER_TYPE> bufferBarriers = {};
		uint32_t numGlobalBarriers = 0;
		uint32_t numTextureBarriers = 0;
		uint32_t numBufferBarriers = 0;

		std::array<const Texture*, MAX_SIMULTANEOUS_RENDER_TARGETS> currentRenderTextures = {};
		uint32_t numCurrentRenderTextures = 0;
		const Texture* currentDepthTexture = nullptr;
		VkCommandBuffer currentCommandBuffer = VK_NULL_HANDLE;

		bool activeRenderPass = false;
		uint32_t nestedPauseCounter = false;
	};

	struct Impl_Pipeline
	{
		VkPipeline pipeline = VK_NULL_HANDLE;
	};

	struct Impl_BufferAllocatorPage
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	struct Impl_TempBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;

		void Set(const Impl_BufferAllocatorPage& page)
		{
			buffer = page.buffer;
			memory = page.memory;
		}
	};

	struct Impl_Receipt
	{
		VkSemaphore timelineSemaphore = VK_NULL_HANDLE;
	};

	//-------------------- Descriptor Heap --------------------//
	static constexpr uint32_t NUM_VK_DESCRIPTOR_TYPES = 5;
	static constexpr uint32_t VK_BINDING_NON_SAMPLER = 0;
	static constexpr uint32_t VK_BINDING_SAMPLER = 1;
	enum class VulkanDescriptorType
	{
		ConstantBuffer_CBV = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // CBV
		RawOrStructuredBuffer_SRV_UAV = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // SRV or UAV
		Texture_SRV = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, // SRV
		Texture_UAV = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, // UAV
		Sampler = VK_DESCRIPTOR_TYPE_SAMPLER,
	};
	static constexpr std::array<VkDescriptorType, NUM_VK_DESCRIPTOR_TYPES> vkDescriptorTypeTable =
	{
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		VK_DESCRIPTOR_TYPE_SAMPLER
	};
	std::array<uint32_t, NUM_VK_DESCRIPTOR_TYPES> GetVulkanPropsMaxDescriptors(VkPhysicalDevice);

	struct Impl_DescriptorHeap
	{
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		VkPipelineLayout bindlessPipelineLayout = VK_NULL_HANDLE;
	};
	//----------------------------------------//

	struct Impl_PerFrameTexture
	{
		void* mapped = nullptr;
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	struct Impl_Texture
	{
		Descriptor srv;
		Descriptor uav;
		VkImageView view_srv = VK_NULL_HANDLE;
		VkImageView view_uav_rtv_dsv = VK_NULL_HANDLE;
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	struct Impl_PerFrameBuffer
	{
		void* mapped = nullptr;
		Descriptor cbv_srv_uav;
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	struct Impl_Buffer
	{
		Descriptor cbv_srv_uav;
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	struct VulkanQueueFamilies
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> computeFamily;
		std::optional<uint32_t> transferFamily;
		std::optional<uint32_t> presentFamily;
	};
	VulkanQueueFamilies GetVulkanQueueFamilies(VkPhysicalDevice, VkSurfaceKHR);

	struct VulkanSwapChainInfo
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
	VulkanSwapChainInfo GetVulkanSwapChainInfo(VkPhysicalDevice, VkSurfaceKHR);

	VkFormat ToVulkanFormat(Format);

	struct VulkanVertexLayout
	{
		std::vector<VkVertexInputBindingDescription> bindings;
		std::vector<VkVertexInputAttributeDescription> attributes;
	};
	VulkanVertexLayout ToVulkanVertexLayout(const std::vector<VertexElement>& elems);

	struct VulkanTextureFilter
	{
		VkFilter magFilter = VkFilter::VK_FILTER_NEAREST;
		VkFilter minFilter = VkFilter::VK_FILTER_NEAREST;
		VkSamplerMipmapMode mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
		VkBool32 anisotropyEnable = VK_FALSE;
		float maxAnisotropy = 1.0f;
		VkBool32 compareEnable = VK_FALSE;
	};
	VulkanTextureFilter ToVulkanTextureFilter(TextureFilter);

	VkPresentModeKHR ToVulkanPresentMode(PresentMode);
	std::optional<uint32_t> FindVulkanMemoryType(VkPhysicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	DetailedResult IsVulkanPhysicalDeviceSuitable(VkPhysicalDevice, VkSurfaceKHR, const std::vector<const char*>& requiredDeviceExtensions);
	uint64_t GetRequiredUploadBufferSize(const Image&, const BufferPlacementAlignments&);
	VkImageAspectFlags GetFullImageAspect(Format format);

}
