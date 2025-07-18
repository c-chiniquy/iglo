
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
		bool swapChainUsesMinImageCount = false;
		bool validSwapChain = false;
#ifdef _DEBUG
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

		std::array<VkMemoryBarrier2, MAX_BATCHED_BARRIERS_PER_TYPE> globalBarriers = {};
		std::array<VkImageMemoryBarrier2, MAX_BATCHED_BARRIERS_PER_TYPE> textureBarriers = {};
		std::array<VkBufferMemoryBarrier2, MAX_BATCHED_BARRIERS_PER_TYPE> bufferBarriers = {};
		uint32_t numGlobalBarriers = 0;
		uint32_t numTextureBarriers = 0;
		uint32_t numBufferBarriers = 0;

		std::array<const Texture*, MAX_SIMULTANEOUS_RENDER_TARGETS> currentRenderTextures = {};
		uint32_t numCurrentRenderTextures = 0;
		const Texture* currentDepthTexture = nullptr;
		VkCommandBuffer currentCommandBuffer = VK_NULL_HANDLE;

		bool isRendering = false;
		bool temporaryRenderPause = false;
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

	struct Impl_DescriptorHeap
	{
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		VkPipelineLayout bindlessPipelineLayout = VK_NULL_HANDLE;

		std::vector<std::vector<VkImageView>> tempImageViews; // Per-frame.
	};

	struct Impl_Texture
	{
		std::vector<VkImage> image;  // Per-frame if Readable
		std::vector<VkDeviceMemory> memory;  // Per-frame if Readable
		VkImageView imageView_SRV = VK_NULL_HANDLE;
		VkImageView imageView_UAV = VK_NULL_HANDLE;
		VkImageView imageView_RTV_DSV = VK_NULL_HANDLE;
	};

	struct Impl_Buffer
	{
		std::vector<VkBuffer> buffer;
		std::vector<VkDeviceMemory> memory;
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

	VkFormat ConvertToVulkanFormat(Format);

	struct VulkanVertexLayout
	{
		std::vector<VkVertexInputBindingDescription> bindings;
		std::vector<VkVertexInputAttributeDescription> attributes;
	};
	VulkanVertexLayout ConvertToVulkanVertexLayout(const std::vector<VertexElement>& elems);

	struct VulkanTextureFilter
	{
		VkFilter magFilter = VkFilter::VK_FILTER_NEAREST;
		VkFilter minFilter = VkFilter::VK_FILTER_NEAREST;
		VkSamplerMipmapMode mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
		VkBool32 anisotropyEnable = VK_FALSE;
		float maxAnisotropy = 1.0f;
		VkBool32 compareEnable = VK_FALSE;
	};
	VulkanTextureFilter ConvertToVulkanTextureFilter(TextureFilter);


	std::optional<uint32_t> FindVulkanMemoryType(VkPhysicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	DetailedResult IsVulkanPhysicalDeviceSuitable(VkPhysicalDevice, VkSurfaceKHR, const std::vector<const char*>& requiredDeviceExtensions);
	uint64_t GetRequiredUploadBufferSize(const Image&, const BufferPlacementAlignments&);
	VkResult CreateVulkanImageViewForSwapChain(VkDevice device, VkImage swapChainImage, VkFormat swapChainFormat, VkImageView* out_pView);
	VkImageAspectFlags GetVulkanImageAspect(Format format);

}
