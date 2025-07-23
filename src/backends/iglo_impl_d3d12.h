
#pragma once

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#include <d3d12.h>
#include <dxgi1_6.h> 
#include <wrl.h> // For ComPtr

using Microsoft::WRL::ComPtr;

namespace ig
{
	struct Impl_GraphicsContext
	{
		ComPtr<ID3D12Device10> device;
		ComPtr<IDXGISwapChain3> swapChain;
		ComPtr<IDXGIFactory4> factory;
	};

	struct Impl_CommandQueue
	{
		// One of each: Graphics, Compute, Copy
		std::array<ComPtr<ID3D12CommandQueue>, 3> commandQueues;
		std::array<ComPtr<ID3D12Fence>, 3> fences;
		std::array<uint64_t, 3> fenceValues = {};
		HANDLE fenceEvent = nullptr;
		std::mutex receiptMutex;
		std::mutex waitMutex;
	};

	struct Impl_CommandList
	{
		std::vector<ComPtr<ID3D12CommandAllocator>> commandAllocator; // One per frame
		ComPtr<ID3D12GraphicsCommandList7> graphicsCommandList;

		std::array<D3D12_GLOBAL_BARRIER, MAX_BATCHED_BARRIERS_PER_TYPE> globalBarriers = {};
		std::array<D3D12_TEXTURE_BARRIER, MAX_BATCHED_BARRIERS_PER_TYPE> textureBarriers = {};
		std::array<D3D12_BUFFER_BARRIER, MAX_BATCHED_BARRIERS_PER_TYPE> bufferBarriers = {};
		uint32_t numGlobalBarriers = 0;
		uint32_t numTextureBarriers = 0;
		uint32_t numBufferBarriers = 0;
	};

	struct Impl_Pipeline
	{
		ComPtr<ID3D12PipelineState> pipeline;
	};

	struct Impl_BufferAllocatorPage
	{
		ComPtr<ID3D12Resource> resource;
	};

	struct Impl_TempBuffer
	{
		ID3D12Resource* resource = nullptr;

		void Set(const Impl_BufferAllocatorPage& page)
		{
			resource = page.resource.Get();
		}
	};

	struct Impl_Receipt
	{
		ID3D12Fence* fence = nullptr;
	};

	struct Impl_DescriptorHeap
	{
		// Small CPU-only descriptor heaps.
		ComPtr<ID3D12DescriptorHeap> descriptorHeap_NonShaderVisible_RTV; // For clearing and setting render textures
		ComPtr<ID3D12DescriptorHeap> descriptorHeap_NonShaderVisible_DSV; // For clearing and setting depth buffers
		ComPtr<ID3D12DescriptorHeap> descriptorHeap_NonShaderVisible_UAV; // For clearing unordered access textures/buffers

		// Shader visible descriptor heaps
		ComPtr<ID3D12DescriptorHeap> descriptorHeap_Resources; // SRV CBV UAV
		ComPtr<ID3D12DescriptorHeap> descriptorHeap_Samplers;

		ComPtr<ID3D12RootSignature> bindlessRootSignature;

		uint32_t descriptorSize_RTV = 0;
		uint32_t descriptorSize_DSV = 0;
		uint32_t descriptorSize_Sampler = 0;
		uint32_t descriptorSize_CBV_SRV_UAV = 0;
	};

	struct D3D12ViewDescUnion_RTV_DSV_UAV
	{
		D3D12ViewDescUnion_RTV_DSV_UAV()
		{
			std::memset(this, 0, sizeof(*this));
		}

		union
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtv;
			D3D12_DEPTH_STENCIL_VIEW_DESC dsv;
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav; // For clearing uav textures
		};
	};

	struct Impl_Texture
	{
		std::vector<ComPtr<ID3D12Resource>> resource; // Per-frame only if Readable; otherwise just one.
		
		// Info used to create RTVs, DSVs and UAVs when setting/clearing render targets and clearing unordered access textures.
		// These CPU-only descriptors are created only when needed.
		D3D12ViewDescUnion_RTV_DSV_UAV desc_cpu; 
	};

	struct Impl_Buffer
	{
		std::vector<ComPtr<ID3D12Resource>> resource;
		D3D12_UNORDERED_ACCESS_VIEW_DESC desc_cpu_uav = {}; // For clearing uav buffers
	};

	struct D3D12TextureFilter
	{
		D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		unsigned int maxAnisotropy = 0;
	};
	D3D12TextureFilter ConvertToD3D12TextureFilter(TextureFilter filter);

	struct FormatInfoDXGI
	{
		DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
		DXGI_FORMAT typeless = DXGI_FORMAT_UNKNOWN;
	};
	// Returns DXGI_FORMAT_UNKNOWN if format is not supported in D3D12 or if format is unknown.
	FormatInfoDXGI GetFormatInfoDXGI(Format format);

	std::vector<D3D12_INPUT_ELEMENT_DESC> ConvertToD3D12InputElements(const std::vector<VertexElement>& elems);

}
