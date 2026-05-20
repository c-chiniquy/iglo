
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
		ComPtr<IDXGIFactory6> factory;
		ComPtr<IDXGIAdapter1> adapter;
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

		std::array<D3D12_GLOBAL_BARRIER, MAX_QUEUED_BARRIERS_PER_TYPE> globalBarriers = {};
		std::array<D3D12_TEXTURE_BARRIER, MAX_QUEUED_BARRIERS_PER_TYPE> textureBarriers = {};
		std::array<D3D12_BUFFER_BARRIER, MAX_QUEUED_BARRIERS_PER_TYPE> bufferBarriers = {};
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
		ComPtr<ID3D12RootSignature> bindlessRootSignature;

		// Shader-visible descriptor heaps
		ComPtr<ID3D12DescriptorHeap> heap_CBV_SRV_UAV;
		ComPtr<ID3D12DescriptorHeap> heap_Samplers;

		// Non-shader-visible descriptor heaps
		ComPtr<ID3D12DescriptorHeap> heap_RTV;
		ComPtr<ID3D12DescriptorHeap> heap_DSV;

		// Small reusable non-shader-visible descriptor heaps
		ComPtr<ID3D12DescriptorHeap> heap_Reusable_UAV; // For clearing unordered access textures/buffers

		uint32_t descriptorSize_CBV_SRV_UAV = 0;
		uint32_t descriptorSize_Sampler = 0;
		uint32_t descriptorSize_RTV = 0;
		uint32_t descriptorSize_DSV = 0;
	};

	struct Impl_PerFrameTexture
	{
		void* mapped = nullptr;
		ComPtr<ID3D12Resource> resource;
	};

	struct Impl_Texture
	{
		Descriptor srv;
		Descriptor uav;
		Descriptor rtv_dsv;
		ComPtr<ID3D12Resource> resource;
	};

	struct Impl_PerFrameBuffer
	{
		void* mapped = nullptr;
		Descriptor cbv_srv;
		Descriptor uav;
		ComPtr<ID3D12Resource> resource;
	};

	struct Impl_Buffer
	{
		Descriptor cbv_srv;
		Descriptor uav;
		ComPtr<ID3D12Resource> resource;
	};

	struct D3D12TextureFilter
	{
		D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		unsigned int maxAnisotropy = 0;
	};
	D3D12TextureFilter ToD3D12TextureFilter(TextureFilter filter);

	struct FormatInfoDXGI
	{
		DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
		DXGI_FORMAT typeless = DXGI_FORMAT_UNKNOWN;
	};
	// Returns DXGI_FORMAT_UNKNOWN if format is not supported in D3D12 or if format is unknown.
	FormatInfoDXGI GetFormatInfoDXGI(Format format);

	std::vector<D3D12_INPUT_ELEMENT_DESC> ToD3D12InputElements(const std::vector<VertexElement>& elems);
	DetailedResult IsAdapterCompatible(ID3D12Device* device);

}
