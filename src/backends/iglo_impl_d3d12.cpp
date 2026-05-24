
#include "iglo.h"

#ifdef IGLO_D3D12

namespace ig
{
	constexpr D3D_FEATURE_LEVEL d3d12FeatureLevel = D3D_FEATURE_LEVEL_12_1;
	constexpr DXGI_SWAP_EFFECT d3d12SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	constexpr UINT d3d12SwapChainFlags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	std::string FeatureLevelToString(D3D_FEATURE_LEVEL featureLevel)
	{
		uint32_t value = (uint32_t)featureLevel;
		uint32_t major = (value >> 12) & 0xF;
		uint32_t minor = (value >> 8) & 0xF;
		return ToString(major, "_", minor);
	}

	std::string D3D12ErrorMsg(const char* functionName, HRESULT hr)
	{
		return ToString(functionName, " returned error code: ", (uint32_t)hr, ".");
	}

	D3D12TextureFilter ToD3D12TextureFilter(TextureFilter filter)
	{
		D3D12TextureFilter out;
		out.filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		out.maxAnisotropy = 0;

		switch (filter)
		{
		case TextureFilter::Point:			out.filter = D3D12_FILTER_MIN_MAG_MIP_POINT;		out.maxAnisotropy = 1;	break;
		case TextureFilter::Bilinear:		out.filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT; out.maxAnisotropy = 1;	break;
		case TextureFilter::Trilinear:		out.filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;		out.maxAnisotropy = 1;	break;
		case TextureFilter::AnisotropicX2:	out.filter = D3D12_FILTER_ANISOTROPIC;				out.maxAnisotropy = 2; 	break;
		case TextureFilter::AnisotropicX4:	out.filter = D3D12_FILTER_ANISOTROPIC;				out.maxAnisotropy = 4; 	break;
		case TextureFilter::AnisotropicX8:	out.filter = D3D12_FILTER_ANISOTROPIC;				out.maxAnisotropy = 8; 	break;
		case TextureFilter::AnisotropicX16:	out.filter = D3D12_FILTER_ANISOTROPIC;				out.maxAnisotropy = 16; break;

		case TextureFilter::_Comparison_Point:			out.filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;		   out.maxAnisotropy = 1;  break;
		case TextureFilter::_Comparison_Bilinear:		out.filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; out.maxAnisotropy = 1;  break;
		case TextureFilter::_Comparison_Trilinear:		out.filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;	   out.maxAnisotropy = 1;  break;
		case TextureFilter::_Comparison_AnisotropicX2:	out.filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;			   out.maxAnisotropy = 2;  break;
		case TextureFilter::_Comparison_AnisotropicX4:	out.filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;			   out.maxAnisotropy = 4;  break;
		case TextureFilter::_Comparison_AnisotropicX8:	out.filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;			   out.maxAnisotropy = 8;  break;
		case TextureFilter::_Comparison_AnisotropicX16:	out.filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;			   out.maxAnisotropy = 16; break;

		case TextureFilter::_Minimum_Point:			 out.filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;		 out.maxAnisotropy = 1;  break;
		case TextureFilter::_Minimum_Bilinear:		 out.filter = D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT; out.maxAnisotropy = 1;  break;
		case TextureFilter::_Minimum_Trilinear:		 out.filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;		 out.maxAnisotropy = 1;  break;
		case TextureFilter::_Minimum_AnisotropicX2:	 out.filter = D3D12_FILTER_MINIMUM_ANISOTROPIC;			     out.maxAnisotropy = 2;  break;
		case TextureFilter::_Minimum_AnisotropicX4:	 out.filter = D3D12_FILTER_MINIMUM_ANISOTROPIC;			     out.maxAnisotropy = 4;  break;
		case TextureFilter::_Minimum_AnisotropicX8:	 out.filter = D3D12_FILTER_MINIMUM_ANISOTROPIC;			     out.maxAnisotropy = 8;  break;
		case TextureFilter::_Minimum_AnisotropicX16: out.filter = D3D12_FILTER_MINIMUM_ANISOTROPIC;			     out.maxAnisotropy = 16; break;

		case TextureFilter::_Maximum_Point:			 out.filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;		 out.maxAnisotropy = 1;  break;
		case TextureFilter::_Maximum_Bilinear:		 out.filter = D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT; out.maxAnisotropy = 1;  break;
		case TextureFilter::_Maximum_Trilinear:		 out.filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;		 out.maxAnisotropy = 1;  break;
		case TextureFilter::_Maximum_AnisotropicX2:	 out.filter = D3D12_FILTER_MAXIMUM_ANISOTROPIC;			     out.maxAnisotropy = 2;  break;
		case TextureFilter::_Maximum_AnisotropicX4:	 out.filter = D3D12_FILTER_MAXIMUM_ANISOTROPIC;			     out.maxAnisotropy = 4;  break;
		case TextureFilter::_Maximum_AnisotropicX8:	 out.filter = D3D12_FILTER_MAXIMUM_ANISOTROPIC;			     out.maxAnisotropy = 8;  break;
		case TextureFilter::_Maximum_AnisotropicX16: out.filter = D3D12_FILTER_MAXIMUM_ANISOTROPIC;			     out.maxAnisotropy = 16; break;

		default:
			Fatal("Invalid texture filter.");
		}

		return out;
	}

	FormatInfoDXGI GetFormatInfoDXGI(Format format)
	{
		DXGI_FORMAT dxgi = DXGI_FORMAT_UNKNOWN;
		DXGI_FORMAT typeless = DXGI_FORMAT_UNKNOWN;

		switch (format)
		{
		case Format::FLOAT_FLOAT_FLOAT_FLOAT:			dxgi = DXGI_FORMAT_R32G32B32A32_FLOAT;	typeless = DXGI_FORMAT_R32G32B32A32_TYPELESS; break;
		case Format::FLOAT_FLOAT_FLOAT:					dxgi = DXGI_FORMAT_R32G32B32_FLOAT;		typeless = DXGI_FORMAT_R32G32B32_TYPELESS; break;
		case Format::FLOAT_FLOAT:						dxgi = DXGI_FORMAT_R32G32_FLOAT;		typeless = DXGI_FORMAT_R32G32_TYPELESS; break;
		case Format::FLOAT:								dxgi = DXGI_FORMAT_R32_FLOAT;			typeless = DXGI_FORMAT_R32_TYPELESS; break;
		case Format::FLOAT16_FLOAT16_FLOAT16_FLOAT16:	dxgi = DXGI_FORMAT_R16G16B16A16_FLOAT;	typeless = DXGI_FORMAT_R16G16B16A16_TYPELESS; break;
		case Format::FLOAT16_FLOAT16:					dxgi = DXGI_FORMAT_R16G16_FLOAT;		typeless = DXGI_FORMAT_R16G16_TYPELESS; break;
		case Format::FLOAT16:							dxgi = DXGI_FORMAT_R16_FLOAT;			typeless = DXGI_FORMAT_R16_TYPELESS; break;
		case Format::BYTE_BYTE_BYTE_BYTE:				dxgi = DXGI_FORMAT_R8G8B8A8_UNORM;		typeless = DXGI_FORMAT_R8G8B8A8_TYPELESS; break;
		case Format::BYTE_BYTE:							dxgi = DXGI_FORMAT_R8G8_UNORM;			typeless = DXGI_FORMAT_R8G8_TYPELESS; break;
		case Format::BYTE:								dxgi = DXGI_FORMAT_R8_UNORM;			typeless = DXGI_FORMAT_R8_TYPELESS; break;
		case Format::INT8_INT8_INT8_INT8:				dxgi = DXGI_FORMAT_R8G8B8A8_SNORM;		typeless = DXGI_FORMAT_R8G8B8A8_TYPELESS; break;
		case Format::INT8_INT8:							dxgi = DXGI_FORMAT_R8G8_SNORM;			typeless = DXGI_FORMAT_R8G8_TYPELESS; break;
		case Format::INT8:								dxgi = DXGI_FORMAT_R8_SNORM;			typeless = DXGI_FORMAT_R8_TYPELESS; break;
		case Format::UINT16_UINT16_UINT16_UINT16:		dxgi = DXGI_FORMAT_R16G16B16A16_UNORM;	typeless = DXGI_FORMAT_R16G16B16A16_TYPELESS; break;
		case Format::UINT16_UINT16:						dxgi = DXGI_FORMAT_R16G16_UNORM;		typeless = DXGI_FORMAT_R16G16_TYPELESS; break;
		case Format::UINT16:							dxgi = DXGI_FORMAT_R16_UNORM;			typeless = DXGI_FORMAT_R16_TYPELESS; break;
		case Format::INT16_INT16_INT16_INT16:			dxgi = DXGI_FORMAT_R16G16B16A16_SNORM;	typeless = DXGI_FORMAT_R16G16B16A16_TYPELESS; break;
		case Format::INT16_INT16:						dxgi = DXGI_FORMAT_R16G16_SNORM;		typeless = DXGI_FORMAT_R16G16_TYPELESS; break;
		case Format::INT16:								dxgi = DXGI_FORMAT_R16_SNORM;			typeless = DXGI_FORMAT_R16_TYPELESS; break;
		case Format::UINT10_UINT10_UINT10_UINT2:		dxgi = DXGI_FORMAT_R10G10B10A2_UNORM;	typeless = DXGI_FORMAT_R10G10B10A2_TYPELESS; break;
		case Format::UINT10_UINT10_UINT10_UINT2_NotNormalized: dxgi = DXGI_FORMAT_R10G10B10A2_UINT; typeless = DXGI_FORMAT_R10G10B10A2_TYPELESS; break;
		case Format::UFLOAT11_UFLOAT11_UFLOAT10:		dxgi = DXGI_FORMAT_R11G11B10_FLOAT;		typeless = DXGI_FORMAT_UNKNOWN; break;
		case Format::UFLOAT9_UFLOAT9_UFLOAT9_UFLOAT5:	dxgi = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;	typeless = DXGI_FORMAT_UNKNOWN; break;
		case Format::BYTE_BYTE_BYTE_BYTE_sRGB:			dxgi = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	typeless = DXGI_FORMAT_R8G8B8A8_TYPELESS; break;
		case Format::BYTE_BYTE_BYTE_BYTE_BGRA_sRGB:		dxgi = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;	typeless = DXGI_FORMAT_B8G8R8A8_TYPELESS; break;
		case Format::BYTE_BYTE_BYTE_BYTE_BGRA:			dxgi = DXGI_FORMAT_B8G8R8A8_UNORM;		typeless = DXGI_FORMAT_B8G8R8A8_TYPELESS; break;
		case Format::BYTE_BYTE_BYTE_BYTE_NotNormalized: dxgi = DXGI_FORMAT_R8G8B8A8_UINT;		typeless = DXGI_FORMAT_R8G8B8A8_TYPELESS; break;
		case Format::BYTE_BYTE_NotNormalized:			dxgi = DXGI_FORMAT_R8G8_UINT;			typeless = DXGI_FORMAT_R8G8_TYPELESS; break;
		case Format::BYTE_NotNormalized:				dxgi = DXGI_FORMAT_R8_UINT;				typeless = DXGI_FORMAT_R8_TYPELESS; break;
		case Format::BC1:			dxgi = DXGI_FORMAT_BC1_UNORM;		typeless = DXGI_FORMAT_BC1_TYPELESS; break;
		case Format::BC1_sRGB:		dxgi = DXGI_FORMAT_BC1_UNORM_SRGB;	typeless = DXGI_FORMAT_BC1_TYPELESS; break;
		case Format::BC2:			dxgi = DXGI_FORMAT_BC2_UNORM;		typeless = DXGI_FORMAT_BC2_TYPELESS; break;
		case Format::BC2_sRGB:		dxgi = DXGI_FORMAT_BC2_UNORM_SRGB;	typeless = DXGI_FORMAT_BC2_TYPELESS; break;
		case Format::BC3:			dxgi = DXGI_FORMAT_BC3_UNORM;		typeless = DXGI_FORMAT_BC3_TYPELESS; break;
		case Format::BC3_sRGB:		dxgi = DXGI_FORMAT_BC3_UNORM_SRGB;	typeless = DXGI_FORMAT_BC3_TYPELESS; break;
		case Format::BC4_UNSIGNED:	dxgi = DXGI_FORMAT_BC4_UNORM;		typeless = DXGI_FORMAT_BC4_TYPELESS; break;
		case Format::BC4_SIGNED:	dxgi = DXGI_FORMAT_BC4_SNORM;		typeless = DXGI_FORMAT_BC4_TYPELESS; break;
		case Format::BC5_UNSIGNED:	dxgi = DXGI_FORMAT_BC5_UNORM;		typeless = DXGI_FORMAT_BC5_TYPELESS; break;
		case Format::BC5_SIGNED:	dxgi = DXGI_FORMAT_BC5_SNORM;		typeless = DXGI_FORMAT_BC5_TYPELESS; break;
		case Format::BC6H_UFLOAT16: dxgi = DXGI_FORMAT_BC6H_UF16;		typeless = DXGI_FORMAT_BC6H_TYPELESS; break;
		case Format::BC6H_SFLOAT16: dxgi = DXGI_FORMAT_BC6H_SF16;		typeless = DXGI_FORMAT_BC6H_TYPELESS; break;
		case Format::BC7:			dxgi = DXGI_FORMAT_BC7_UNORM;		typeless = DXGI_FORMAT_BC7_TYPELESS; break;
		case Format::BC7_sRGB:		dxgi = DXGI_FORMAT_BC7_UNORM_SRGB;	typeless = DXGI_FORMAT_BC7_TYPELESS; break;
		case Format::INT8_INT8_INT8_INT8_NotNormalized:			dxgi = DXGI_FORMAT_R8G8B8A8_SINT;			typeless = DXGI_FORMAT_R8G8B8A8_TYPELESS; break;
		case Format::INT8_INT8_NotNormalized:					dxgi = DXGI_FORMAT_R8G8_SINT;				typeless = DXGI_FORMAT_R8G8_TYPELESS; break;
		case Format::INT8_NotNormalized:						dxgi = DXGI_FORMAT_R8_SINT;					typeless = DXGI_FORMAT_R8_TYPELESS; break;
		case Format::UINT16_UINT16_UINT16_UINT16_NotNormalized:	dxgi = DXGI_FORMAT_R16G16B16A16_UINT;		typeless = DXGI_FORMAT_R16G16B16A16_TYPELESS; break;
		case Format::UINT16_UINT16_NotNormalized:				dxgi = DXGI_FORMAT_R16G16_UINT;				typeless = DXGI_FORMAT_R16G16_TYPELESS; break;
		case Format::UINT16_NotNormalized:						dxgi = DXGI_FORMAT_R16_UINT;				typeless = DXGI_FORMAT_R16_TYPELESS; break;
		case Format::INT16_INT16_INT16_INT16_NotNormalized:		dxgi = DXGI_FORMAT_R16G16B16A16_SINT;		typeless = DXGI_FORMAT_R16G16B16A16_TYPELESS; break;
		case Format::INT16_INT16_NotNormalized:					dxgi = DXGI_FORMAT_R16G16_SINT;				typeless = DXGI_FORMAT_R16G16_TYPELESS; break;
		case Format::INT16_NotNormalized:						dxgi = DXGI_FORMAT_R16_SINT;				typeless = DXGI_FORMAT_R16_TYPELESS; break;
		case Format::INT32_INT32_INT32_INT32_NotNormalized:		dxgi = DXGI_FORMAT_R32G32B32A32_SINT;		typeless = DXGI_FORMAT_R32G32B32A32_TYPELESS; break;
		case Format::INT32_INT32_INT32_NotNormalized:			dxgi = DXGI_FORMAT_R32G32B32_SINT;			typeless = DXGI_FORMAT_R32G32B32_TYPELESS; break;
		case Format::INT32_INT32_NotNormalized:					dxgi = DXGI_FORMAT_R32G32_SINT;				typeless = DXGI_FORMAT_R32G32_TYPELESS; break;
		case Format::INT32_NotNormalized:						dxgi = DXGI_FORMAT_R32_SINT;				typeless = DXGI_FORMAT_R32_TYPELESS; break;
		case Format::UINT32_UINT32_UINT32_UINT32_NotNormalized:	dxgi = DXGI_FORMAT_R32G32B32A32_UINT;		typeless = DXGI_FORMAT_R32G32B32A32_TYPELESS; break;
		case Format::UINT32_UINT32_UINT32_NotNormalized:		dxgi = DXGI_FORMAT_R32G32B32_UINT;			typeless = DXGI_FORMAT_R32G32B32_TYPELESS; break;
		case Format::UINT32_UINT32_NotNormalized:				dxgi = DXGI_FORMAT_R32G32_UINT;				typeless = DXGI_FORMAT_R32G32_TYPELESS; break;
		case Format::UINT32_NotNormalized:						dxgi = DXGI_FORMAT_R32_UINT;				typeless = DXGI_FORMAT_R32_TYPELESS; break;
		case Format::DEPTHFORMAT_FLOAT:							dxgi = DXGI_FORMAT_D32_FLOAT;				typeless = DXGI_FORMAT_R32_TYPELESS; break;
		case Format::DEPTHFORMAT_UINT16:						dxgi = DXGI_FORMAT_D16_UNORM;				typeless = DXGI_FORMAT_R16_TYPELESS;  break;
		case Format::DEPTHFORMAT_UINT24_BYTE:					dxgi = DXGI_FORMAT_D24_UNORM_S8_UINT;		typeless = DXGI_FORMAT_R24G8_TYPELESS; break;
		case Format::DEPTHFORMAT_FLOAT_BYTE:					dxgi = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;	typeless = DXGI_FORMAT_R32G8X24_TYPELESS; break;

		default:
			break;
		}

		FormatInfoDXGI out = FormatInfoDXGI();
		out.dxgiFormat = dxgi;
		out.typeless = typeless;
		return out;
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> ToD3D12InputElements(const std::vector<VertexElement>& elems)
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> out(elems.size());
		for (size_t i = 0; i < elems.size(); i++)
		{
			const VertexElement& element = elems[i];
			FormatInfoDXGI formatInfoDXGI = GetFormatInfoDXGI(element.format);
			DXGI_FORMAT format = formatInfoDXGI.dxgiFormat;
			D3D12_INPUT_CLASSIFICATION inputClass = (element.inputSlotClass == InputClass::PerInstance)
				? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA
				: D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			UINT align = (i == 0) ? 0 : D3D12_APPEND_ALIGNED_ELEMENT;
			out[i] = { element.semanticName, element.semanticIndex, format, element.inputSlot, align, inputClass, element.instanceDataStepRate };
		}
		return out;
	}

	void Pipeline::Impl_Destroy()
	{
		impl.pipeline = nullptr;
	}

	DetailedResult Pipeline::Impl_CreateGraphics(const PipelineDesc& desc)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipe = {};

		// Root signature
		pipe.pRootSignature = context.GetDescriptorHeap().GetD3D12BindlessRootSignature();

		// Shaders
		pipe.VS.BytecodeLength = desc.VS.bytecodeLength;
		pipe.VS.pShaderBytecode = (const void*)desc.VS.shaderBytecode;

		pipe.PS.BytecodeLength = desc.PS.bytecodeLength;
		pipe.PS.pShaderBytecode = (const void*)desc.PS.shaderBytecode;

		pipe.DS.BytecodeLength = desc.DS.bytecodeLength;
		pipe.DS.pShaderBytecode = (const void*)desc.DS.shaderBytecode;

		pipe.HS.BytecodeLength = desc.HS.bytecodeLength;
		pipe.HS.pShaderBytecode = (const void*)desc.HS.shaderBytecode;

		pipe.GS.BytecodeLength = desc.GS.bytecodeLength;
		pipe.GS.pShaderBytecode = (const void*)desc.GS.shaderBytecode;

		// Blend
		pipe.BlendState = {};
		pipe.BlendState.IndependentBlendEnable = true;
		pipe.BlendState.AlphaToCoverageEnable = desc.enableAlphaToCoverage;
		for (size_t i = 0; i < desc.blendStates.size(); i++)
		{
			D3D12_RENDER_TARGET_BLEND_DESC& d3d12BlendDesc = pipe.BlendState.RenderTarget[i];
			const BlendDesc& blendDesc = desc.blendStates[i];

			d3d12BlendDesc.BlendEnable = blendDesc.enabled;
			d3d12BlendDesc.SrcBlend = (D3D12_BLEND)blendDesc.srcBlend;
			d3d12BlendDesc.DestBlend = (D3D12_BLEND)blendDesc.destBlend;
			d3d12BlendDesc.BlendOp = (D3D12_BLEND_OP)blendDesc.blendOp;
			d3d12BlendDesc.SrcBlendAlpha = (D3D12_BLEND)blendDesc.srcBlendAlpha;
			d3d12BlendDesc.DestBlendAlpha = (D3D12_BLEND)blendDesc.destBlendAlpha;
			d3d12BlendDesc.BlendOpAlpha = (D3D12_BLEND_OP)blendDesc.blendOpAlpha;
			d3d12BlendDesc.RenderTargetWriteMask = (UINT8)blendDesc.colorWriteMask;
			d3d12BlendDesc.LogicOpEnable = desc.blendLogicOpEnabled;
			d3d12BlendDesc.LogicOp = (D3D12_LOGIC_OP)desc.blendLogicOp;
		}

		// Rasterizer
		pipe.RasterizerState = {};
		pipe.RasterizerState.FrontCounterClockwise = (desc.rasterizerState.frontFace == FrontFace::CCW) ? true : false;
		if (desc.rasterizerState.cull == Cull::Disabled) pipe.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		else if (desc.rasterizerState.cull == Cull::Back) pipe.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		else if (desc.rasterizerState.cull == Cull::Front) pipe.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
		pipe.RasterizerState.DepthBias = desc.rasterizerState.depthBias;
		pipe.RasterizerState.DepthBiasClamp = desc.rasterizerState.depthBiasClamp;
		pipe.RasterizerState.DepthClipEnable = desc.rasterizerState.enableDepthClip;
		pipe.RasterizerState.SlopeScaledDepthBias = desc.rasterizerState.slopeScaledDepthBias;
		pipe.RasterizerState.ForcedSampleCount = desc.rasterizerState.forcedSampleCount;
		if (desc.rasterizerState.enableConservativeRaster)
		{
			pipe.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;
		}
		else
		{
			pipe.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		}
		// For feature levels 10_1 and higher, AntialiasedLineEnable and MultisampleEnable only affect how lines are rasterized.
		if (desc.rasterizerState.lineRasterizationMode == LineRasterizationMode::Pixelated)
		{
			pipe.RasterizerState.AntialiasedLineEnable = false;
			pipe.RasterizerState.MultisampleEnable = false;
		}
		else
		{
			pipe.RasterizerState.AntialiasedLineEnable = true;
			pipe.RasterizerState.MultisampleEnable = false; // This property makes the line look thicker only on MSAA render targets.
		}
		pipe.RasterizerState.FillMode = desc.rasterizerState.enableWireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;

		// Depth
		pipe.DepthStencilState = {};
		pipe.DepthStencilState.DepthEnable = desc.depthState.enableDepth;
		pipe.DepthStencilState.DepthWriteMask = (D3D12_DEPTH_WRITE_MASK)desc.depthState.depthWriteMask;
		pipe.DepthStencilState.DepthFunc = (D3D12_COMPARISON_FUNC)desc.depthState.depthFunc;
		pipe.DepthStencilState.StencilEnable = desc.depthState.enableStencil;
		pipe.DepthStencilState.StencilReadMask = desc.depthState.stencilReadMask;
		pipe.DepthStencilState.StencilWriteMask = desc.depthState.stencilWriteMask;
		pipe.DepthStencilState.FrontFace.StencilFailOp = (D3D12_STENCIL_OP)desc.depthState.frontFaceStencilFailOp;
		pipe.DepthStencilState.FrontFace.StencilDepthFailOp = (D3D12_STENCIL_OP)desc.depthState.frontFaceStencilDepthFailOp;
		pipe.DepthStencilState.FrontFace.StencilPassOp = (D3D12_STENCIL_OP)desc.depthState.frontFaceStencilPassOp;
		pipe.DepthStencilState.FrontFace.StencilFunc = (D3D12_COMPARISON_FUNC)desc.depthState.frontFaceStencilFunc;
		pipe.DepthStencilState.BackFace.StencilFailOp = (D3D12_STENCIL_OP)desc.depthState.backFaceStencilFailOp;
		pipe.DepthStencilState.BackFace.StencilDepthFailOp = (D3D12_STENCIL_OP)desc.depthState.backFaceStencilDepthFailOp;
		pipe.DepthStencilState.BackFace.StencilPassOp = (D3D12_STENCIL_OP)desc.depthState.backFaceStencilPassOp;
		pipe.DepthStencilState.BackFace.StencilFunc = (D3D12_COMPARISON_FUNC)desc.depthState.backFaceStencilFunc;

		// Vertex layout
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = ToD3D12InputElements(desc.vertexLayout);
		pipe.InputLayout.NumElements = (UINT)inputLayout.size();
		pipe.InputLayout.pInputElementDescs = inputLayout.data();

		// Sample mask
		pipe.SampleMask = desc.sampleMask;

		// Primitive topology type
		switch (desc.primitiveTopology)
		{
		case PrimitiveTopology::PointList:
			pipe.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			break;

		case PrimitiveTopology::LineList:
		case PrimitiveTopology::LineStrip:
		case PrimitiveTopology::_LineList_Adj:
		case PrimitiveTopology::_LineStrip_Adj:
			pipe.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			break;

		case PrimitiveTopology::TriangleList:
		case PrimitiveTopology::TriangleStrip:
		case PrimitiveTopology::_TriangleList_Adj:
		case PrimitiveTopology::_TriangleStrip_Adj:
			pipe.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			break;

		case PrimitiveTopology::__1_ControlPointPatchList:
		case PrimitiveTopology::__2_ControlPointPatchList:
		case PrimitiveTopology::__3_ControlPointPatchList:
		case PrimitiveTopology::__4_ControlPointPatchList:
		case PrimitiveTopology::__5_ControlPointPatchList:
		case PrimitiveTopology::__6_ControlPointPatchList:
		case PrimitiveTopology::__7_ControlPointPatchList:
		case PrimitiveTopology::__8_ControlPointPatchList:
		case PrimitiveTopology::__9_ControlPointPatchList:
		case PrimitiveTopology::__10_ControlPointPatchList:
		case PrimitiveTopology::__11_ControlPointPatchList:
		case PrimitiveTopology::__12_ControlPointPatchList:
		case PrimitiveTopology::__13_ControlPointPatchList:
		case PrimitiveTopology::__14_ControlPointPatchList:
		case PrimitiveTopology::__15_ControlPointPatchList:
		case PrimitiveTopology::__16_ControlPointPatchList:
		case PrimitiveTopology::__17_ControlPointPatchList:
		case PrimitiveTopology::__18_ControlPointPatchList:
		case PrimitiveTopology::__19_ControlPointPatchList:
		case PrimitiveTopology::__20_ControlPointPatchList:
		case PrimitiveTopology::__21_ControlPointPatchList:
		case PrimitiveTopology::__22_ControlPointPatchList:
		case PrimitiveTopology::__23_ControlPointPatchList:
		case PrimitiveTopology::__24_ControlPointPatchList:
		case PrimitiveTopology::__25_ControlPointPatchList:
		case PrimitiveTopology::__26_ControlPointPatchList:
		case PrimitiveTopology::__27_ControlPointPatchList:
		case PrimitiveTopology::__28_ControlPointPatchList:
		case PrimitiveTopology::__29_ControlPointPatchList:
		case PrimitiveTopology::__30_ControlPointPatchList:
		case PrimitiveTopology::__31_ControlPointPatchList:
		case PrimitiveTopology::__32_ControlPointPatchList:
			pipe.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
			break;

		default:
			pipe.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
			break;
		}

		// Render target desc
		pipe.SampleDesc.Count = (UINT)desc.renderTargetDesc.msaa;
		pipe.SampleDesc.Quality = 0;
		pipe.NumRenderTargets = (UINT)desc.renderTargetDesc.colorFormats.size();
		for (size_t i = 0; i < desc.renderTargetDesc.colorFormats.size(); i++)
		{
			pipe.RTVFormats[i] = GetFormatInfoDXGI(desc.renderTargetDesc.colorFormats[i]).dxgiFormat;
		}
		pipe.DSVFormat = GetFormatInfoDXGI(desc.renderTargetDesc.depthFormat).dxgiFormat;

		HRESULT hr = context.GetD3D12Device()->CreateGraphicsPipelineState(&pipe, IID_PPV_ARGS(&impl.pipeline));
		if (FAILED(hr))
		{
			return DetailedResult::Fail(D3D12ErrorMsg("ID3D12Device::CreateGraphicsPipelineState", hr));
		}
		return DetailedResult::Success();
	}

	DetailedResult Pipeline::Impl_CreateCompute(const Shader& CS)
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc = {};
		computeDesc.pRootSignature = context.GetDescriptorHeap().GetD3D12BindlessRootSignature();
		computeDesc.CS.BytecodeLength = CS.bytecodeLength;
		computeDesc.CS.pShaderBytecode = (const void*)CS.shaderBytecode;
		HRESULT hr = context.GetD3D12Device()->CreateComputePipelineState(&computeDesc, IID_PPV_ARGS(&impl.pipeline));
		if (FAILED(hr))
		{
			return DetailedResult::Fail(D3D12ErrorMsg("ID3D12Device::CreateComputePipelineState", hr));
		}

		return DetailedResult::Success();
	}

	void CommandQueue::Impl_Destroy()
	{
		for (size_t i = 0; i < impl.commandQueues.size(); i++)
		{
			impl.commandQueues[i] = nullptr;
			impl.fences[i] = nullptr;
			impl.fenceValues[i] = 0;
		}
		if (impl.fenceEvent)
		{
			CloseHandle(impl.fenceEvent);
			impl.fenceEvent = nullptr;
		}
	}

	DetailedResult CommandQueue::Impl_Create()
	{
		std::array<D3D12_COMMAND_QUEUE_DESC, 3> queueDesc = {};

		// Graphics queue
		queueDesc[0].Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc[0].Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		// Compute queue
		queueDesc[1].Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		queueDesc[1].Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		// Copy queue
		queueDesc[2].Type = D3D12_COMMAND_LIST_TYPE_COPY;
		queueDesc[2].Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		for (size_t i = 0; i < queueDesc.size(); i++)
		{
			HRESULT hr = context.GetD3D12Device()->CreateCommandQueue(&queueDesc[i], IID_PPV_ARGS(&impl.commandQueues[i]));
			if (FAILED(hr))
			{
				return DetailedResult::Fail(D3D12ErrorMsg("ID3D12Device10::CreateCommandQueue", hr));
			}

			hr = context.GetD3D12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&impl.fences[i]));
			if (FAILED(hr))
			{
				return DetailedResult::Fail(D3D12ErrorMsg("ID3D12Device10::CreateCommandQueue", hr));
			}

			impl.fenceValues[i] = 0;
		}

		impl.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!impl.fenceEvent)
		{
			return DetailedResult::Fail("Failed to create fence event.");
		}

		return DetailedResult::Success();
	}

	Receipt CommandQueue::Impl_SubmitCommands(const CommandList* const* commandLists, uint32_t numCommandLists, CommandListType cmdType)
	{
		ID3D12CommandList* d3d12cmdLists[MAX_COMMAND_LISTS_PER_SUBMIT];

		for (uint32_t i = 0; i < numCommandLists; i++)
		{
			d3d12cmdLists[i] = commandLists[i]->GetD3D12GraphicsCommandList();
		}

		impl.commandQueues[(uint32_t)cmdType]->ExecuteCommandLists(numCommandLists, d3d12cmdLists);
		return SubmitSignal(cmdType);
	}

	Receipt CommandQueue::SubmitSignal(CommandListType type)
	{
		std::lock_guard<std::mutex> lockGuard(impl.receiptMutex);
		uint32_t t = (uint32_t)type;
		impl.fenceValues[t]++;
		impl.commandQueues[t]->Signal(impl.fences[t].Get(), impl.fenceValues[t]);
		return { impl.fenceValues[t], impl.fences[t].Get() };
	}

	bool CommandQueue::Impl_IsComplete(Receipt receipt) const
	{
		return receipt.impl.fence->GetCompletedValue() >= receipt.fenceValue;
	}

	bool CommandQueue::IsIdle() const
	{
		for (size_t i = 0; i < impl.commandQueues.size(); i++)
		{
			if (!IsComplete({ impl.fenceValues[i], impl.fences[i].Get() })) return false;
		}
		return true;
	}

	void CommandQueue::Impl_WaitForCompletion(Receipt receipt)
	{
		std::lock_guard<std::mutex> lockGuard(impl.waitMutex);
		receipt.impl.fence->SetEventOnCompletion(receipt.fenceValue, impl.fenceEvent);
		WaitForSingleObjectEx(impl.fenceEvent, INFINITE, FALSE);
	}

	void CommandQueue::WaitForIdle()
	{
		for (size_t i = 0; i < impl.commandQueues.size(); i++)
		{
			WaitForCompletion({ impl.fenceValues[i], impl.fences[i].Get() });
		}
	}

	void CommandList::Impl_Destroy()
	{
		impl.commandAllocator.clear();
		impl.commandAllocator.shrink_to_fit();

		impl.graphicsCommandList = nullptr;
	}

	DetailedResult CommandList::Impl_Create()
	{
		auto device = context.GetD3D12Device();

		D3D12_COMMAND_LIST_TYPE d3d12CmdType;
		switch (commandListType)
		{
		case CommandListType::Graphics: d3d12CmdType = D3D12_COMMAND_LIST_TYPE_DIRECT; break;
		case CommandListType::Compute: d3d12CmdType = D3D12_COMMAND_LIST_TYPE_COMPUTE; break;
		case CommandListType::Copy: d3d12CmdType = D3D12_COMMAND_LIST_TYPE_COPY; break;

		default:
			Fatal("Invalid command list type.");
		}

		// Create one commmand allocator for each frame
		impl.commandAllocator.resize((size_t)maxFrames, nullptr);
		for (uint32_t i = 0; i < maxFrames; i++)
		{
			HRESULT hr = device->CreateCommandAllocator(d3d12CmdType, IID_PPV_ARGS(&impl.commandAllocator[i]));
			if (FAILED(hr))
			{
				return DetailedResult::Fail(D3D12ErrorMsg("ID3D12Device::CreateCommandAllocator", hr));
			}
		}

		// Create command list
		{
			ComPtr<ID3D12GraphicsCommandList> cmdList0;
			HRESULT hr = device->CreateCommandList(0, d3d12CmdType, impl.commandAllocator[frameIndex].Get(),
				nullptr, IID_PPV_ARGS(&cmdList0));
			if (FAILED(hr))
			{
				return DetailedResult::Fail(D3D12ErrorMsg("ID3D12Device::CreateCommandList", hr));
			}
			hr = cmdList0.As(&impl.graphicsCommandList);
			if (FAILED(hr))
			{
				return DetailedResult::Fail(D3D12ErrorMsg("ComPtr::As", hr));
			}
		}
		impl.graphicsCommandList->Close();

		impl.globalBarriers = {};
		impl.textureBarriers = {};
		impl.bufferBarriers = {};

		return DetailedResult::Success();
	}

	void CommandList::Impl_Begin()
	{
		DescriptorHeap& heap = context.GetDescriptorHeap();
		if (FAILED(impl.commandAllocator[frameIndex]->Reset()))
		{
			Log(LogType::Error, "Failed to reset D3D12 command allocator.");
		}
		if (FAILED(impl.graphicsCommandList->Reset(impl.commandAllocator[frameIndex].Get(), nullptr)))
		{
			Log(LogType::Error, "Failed to reset D3D12 command list.");
		}
		if (commandListType == CommandListType::Graphics ||
			commandListType == CommandListType::Compute)
		{
			ID3D12DescriptorHeap* heaps[] =
			{
				heap.GetD3D12DescriptorHeap(DescriptorType::Resource),
				heap.GetD3D12DescriptorHeap(DescriptorType::Sampler)
			};
			impl.graphicsCommandList->SetDescriptorHeaps(2, heaps);
			if (commandListType == CommandListType::Graphics)
			{
				impl.graphicsCommandList->SetGraphicsRootSignature(heap.GetD3D12BindlessRootSignature());
			}
			if (commandListType == CommandListType::Graphics ||
				commandListType == CommandListType::Compute)
			{
				impl.graphicsCommandList->SetComputeRootSignature(heap.GetD3D12BindlessRootSignature());
			}
		}
	}

	void CommandList::Impl_End()
	{
		if (impl.numGlobalBarriers > 0 || impl.numTextureBarriers > 0 || impl.numBufferBarriers > 0)
		{
			impl.numGlobalBarriers = 0;
			impl.numTextureBarriers = 0;
			impl.numBufferBarriers = 0;
			Log(LogType::Warning, "You forgot to flush some barriers!");
		}
		if (FAILED(impl.graphicsCommandList->Close()))
		{
			Log(LogType::Error, "Failed to close d3d12 graphics command list.");
		}
	}

	void CommandList::FlushBarriers()
	{
		D3D12_BARRIER_GROUP groups[3] = {};
		uint32_t index = 0;
		if (impl.numGlobalBarriers > 0)
		{
			groups[index].pGlobalBarriers = impl.globalBarriers.data();
			groups[index].NumBarriers = impl.numGlobalBarriers;
			groups[index].Type = D3D12_BARRIER_TYPE_GLOBAL;
			index++;
		}
		if (impl.numTextureBarriers > 0)
		{
			groups[index].pTextureBarriers = impl.textureBarriers.data();
			groups[index].NumBarriers = impl.numTextureBarriers;
			groups[index].Type = D3D12_BARRIER_TYPE_TEXTURE;
			index++;
		}
		if (impl.numBufferBarriers > 0)
		{
			groups[index].pBufferBarriers = impl.bufferBarriers.data();
			groups[index].NumBarriers = impl.numBufferBarriers;
			groups[index].Type = D3D12_BARRIER_TYPE_BUFFER;
			index++;
		}
		if (index > 0)
		{
			impl.graphicsCommandList->Barrier(index, groups);
		}

		impl.numGlobalBarriers = 0;
		impl.numTextureBarriers = 0;
		impl.numBufferBarriers = 0;
	}

	void CommandList::AddGlobalBarrier(BarrierSync syncBefore, BarrierAccess accessBefore, BarrierSync syncAfter, BarrierAccess accessAfter)
	{
		if (impl.numGlobalBarriers >= MAX_QUEUED_BARRIERS_PER_TYPE) FlushBarriers();

		D3D12_GLOBAL_BARRIER& barrier = impl.globalBarriers[impl.numGlobalBarriers];
		barrier.SyncBefore = (D3D12_BARRIER_SYNC)syncBefore;
		barrier.SyncAfter = (D3D12_BARRIER_SYNC)syncAfter;
		barrier.AccessBefore = (D3D12_BARRIER_ACCESS)accessBefore;
		barrier.AccessAfter = (D3D12_BARRIER_ACCESS)accessAfter;

		impl.numGlobalBarriers++;
	}

	void CommandList::AddTextureBarrier(const Texture& texture,
		BarrierSync syncBefore, BarrierAccess accessBefore, BarrierLayout layoutBefore,
		BarrierSync syncAfter, BarrierAccess accessAfter, BarrierLayout layoutAfter, bool discard)
	{
		if (impl.numTextureBarriers >= MAX_QUEUED_BARRIERS_PER_TYPE) FlushBarriers();

		D3D12_TEXTURE_BARRIER& barrier = impl.textureBarriers[impl.numTextureBarriers];
		barrier.pResource = texture.GetD3D12Resource();
		barrier.SyncBefore = (D3D12_BARRIER_SYNC)syncBefore;
		barrier.SyncAfter = (D3D12_BARRIER_SYNC)syncAfter;
		barrier.AccessBefore = (D3D12_BARRIER_ACCESS)accessBefore;
		barrier.AccessAfter = (D3D12_BARRIER_ACCESS)accessAfter;
		barrier.LayoutBefore = (D3D12_BARRIER_LAYOUT)layoutBefore;
		barrier.LayoutAfter = (D3D12_BARRIER_LAYOUT)layoutAfter;
		barrier.Flags = discard ? D3D12_TEXTURE_BARRIER_FLAG_DISCARD : D3D12_TEXTURE_BARRIER_FLAG_NONE;
		barrier.Subresources.IndexOrFirstMipLevel = 0xffffffff;

		impl.numTextureBarriers++;
	}

	void CommandList::AddTextureBarrierAtSubresource(const Texture& texture,
		BarrierSync syncBefore, BarrierAccess accessBefore, BarrierLayout layoutBefore,
		BarrierSync syncAfter, BarrierAccess accessAfter, BarrierLayout layoutAfter,
		uint32_t faceIndex, uint32_t mipIndex, bool discard)
	{
		if (impl.numTextureBarriers >= MAX_QUEUED_BARRIERS_PER_TYPE) FlushBarriers();

		D3D12_TEXTURE_BARRIER& barrier = impl.textureBarriers[impl.numTextureBarriers];
		barrier.pResource = texture.GetD3D12Resource();
		barrier.SyncBefore = (D3D12_BARRIER_SYNC)syncBefore;
		barrier.SyncAfter = (D3D12_BARRIER_SYNC)syncAfter;
		barrier.AccessBefore = (D3D12_BARRIER_ACCESS)accessBefore;
		barrier.AccessAfter = (D3D12_BARRIER_ACCESS)accessAfter;
		barrier.LayoutBefore = (D3D12_BARRIER_LAYOUT)layoutBefore;
		barrier.LayoutAfter = (D3D12_BARRIER_LAYOUT)layoutAfter;
		barrier.Flags = discard ? D3D12_TEXTURE_BARRIER_FLAG_DISCARD : D3D12_TEXTURE_BARRIER_FLAG_NONE;
		barrier.Subresources.IndexOrFirstMipLevel = (faceIndex * texture.GetMipLevels()) + mipIndex;

		impl.numTextureBarriers++;
	}

	void CommandList::AddBufferBarrier(const Buffer& buffer,
		BarrierSync syncBefore, BarrierAccess accessBefore,
		BarrierSync syncAfter, BarrierAccess accessAfter)
	{
		if (impl.numBufferBarriers >= MAX_QUEUED_BARRIERS_PER_TYPE) FlushBarriers();

		D3D12_BUFFER_BARRIER& barrier = impl.bufferBarriers[impl.numBufferBarriers];
		barrier.pResource = buffer.GetD3D12Resource();
		barrier.SyncBefore = (D3D12_BARRIER_SYNC)syncBefore;
		barrier.SyncAfter = (D3D12_BARRIER_SYNC)syncAfter;
		barrier.AccessBefore = (D3D12_BARRIER_ACCESS)accessBefore;
		barrier.AccessAfter = (D3D12_BARRIER_ACCESS)accessAfter;
		barrier.Size = UINT64_MAX;
		barrier.Offset = 0;

		impl.numBufferBarriers++;
	}

	void CommandList::SetPipeline(const Pipeline& pipeline)
	{
		impl.graphicsCommandList->SetPipelineState(pipeline.GetD3D12PipelineState());
		if (!pipeline.IsComputePipeline())
		{
			impl.graphicsCommandList->IASetPrimitiveTopology((D3D12_PRIMITIVE_TOPOLOGY)pipeline.GetPrimitiveTopology());
		}
	}

	void CommandList::SetRenderTarget(const Texture* renderTexture, const Texture* depthBuffer, bool optimizedClear)
	{
		if (renderTexture)
		{
			const Texture* renderTextures[] = { renderTexture };
			SetRenderTargets(renderTextures, 1, depthBuffer, optimizedClear);
		}
		else
		{
			SetRenderTargets(nullptr, 0, depthBuffer, optimizedClear);
		}
	}

	void CommandList::SetRenderTargets(const Texture* const* renderTextures, uint32_t numRenderTextures,
		const Texture* depthBuffer, bool optimizedClear)
	{
		if (numRenderTextures > 0) assert(renderTextures);
		assert(numRenderTextures <= MAX_SIMULTANEOUS_RENDER_TARGETS && "too many render textures provided");

		DescriptorHeap& heap = context.GetDescriptorHeap();

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[MAX_SIMULTANEOUS_RENDER_TARGETS] = {};
		for (uint32_t i = 0; i < numRenderTextures; i++)
		{
			rtvHandles[i] = heap.GetD3D12CPUHandle(renderTextures[i]->GetRenderDescriptor());
		}

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
		if (depthBuffer) dsvHandle = heap.GetD3D12CPUHandle(depthBuffer->GetRenderDescriptor());

		impl.graphicsCommandList->OMSetRenderTargets(numRenderTextures, rtvHandles, FALSE, depthBuffer ? &dsvHandle : nullptr);

		if (optimizedClear)
		{
			for (uint32_t i = 0; i < numRenderTextures; i++)
			{
				ClearColor(*renderTextures[i], renderTextures[i]->GetOptimizedClearValue().color);
			}
			if (depthBuffer)
			{
				ClearValue clearValue = depthBuffer->GetOptimizedClearValue();
				ClearDepth(*depthBuffer, clearValue.depth, clearValue.stencil, true, true);
			}
		}
	}

	void CommandList::BeginRenderPass(const Texture* renderTexture, const Texture* depthBuffer, bool optimizedClear)
	{
		SetRenderTarget(renderTexture, depthBuffer, optimizedClear);
	}
	void CommandList::BeginRenderPassMultiTarget(const Texture* const* renderTextures, uint32_t numRenderTextures,
		const Texture* depthBuffer, bool optimizedClear)
	{
		SetRenderTargets(renderTextures, numRenderTextures, depthBuffer, optimizedClear);
	}

	void CommandList::EndRenderPass()
	{
		SetRenderTarget(nullptr);
	}

	void CommandList::SafePauseRenderPass()
	{
	}
	void CommandList::SafeResumeRenderPass()
	{
	}

	void CommandList::Impl_ClearColor(const Texture& renderTexture, Color color, uint32_t numRects, const IntRect* rects)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = context.GetDescriptorHeap().GetD3D12CPUHandle(renderTexture.GetRenderDescriptor());
		impl.graphicsCommandList->ClearRenderTargetView(rtvHandle, (FLOAT*)&color, numRects, (D3D12_RECT*)rects);
	}

	void CommandList::Impl_ClearDepth(const Texture& depthBuffer, float depth, byte stencil, bool clearDepth, bool clearStencil,
		uint32_t numRects, const IntRect* rects)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = context.GetDescriptorHeap().GetD3D12CPUHandle(depthBuffer.GetRenderDescriptor());
		D3D12_CLEAR_FLAGS flags = (D3D12_CLEAR_FLAGS)0;
		if (clearDepth) flags |= D3D12_CLEAR_FLAG_DEPTH;
		if (clearStencil) flags |= D3D12_CLEAR_FLAG_STENCIL;
		impl.graphicsCommandList->ClearDepthStencilView(dsvHandle, flags, depth, stencil, numRects, (D3D12_RECT*)rects);
	}

	void CommandList::Impl_ClearUnorderedAccessBufferUInt32(const Buffer& buffer, const uint32_t value)
	{
		DescriptorHeap& heap = context.GetDescriptorHeap();
		auto device = context.GetD3D12Device();

		// This non-shader-visible uav needs to be identical the buffer's shader-visible uav
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = (buffer.GetType() == BufferType::StructuredBuffer)
			? Buffer::GenerateD3D12Desc_UAV_Structured(buffer.GetNumElements(), buffer.GetStride())
			: Buffer::GenerateD3D12Desc_UAV_Raw(buffer.GetSize());
		D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = heap.GetD3D12CPUHandle_Reusable_UAV();
		device->CreateUnorderedAccessView(buffer.GetD3D12Resource(), nullptr, &uavDesc, uavHandle);

		const UINT clearValues[4] = { value, value, value, value };

		impl.graphicsCommandList->ClearUnorderedAccessViewUint(heap.GetD3D12GPUHandle(buffer.GetUnorderedAccessDescriptor()),
			uavHandle, buffer.GetD3D12Resource(), clearValues, 0, nullptr);
	}

	void CommandList::Impl_ClearUnorderedAccessTextureFloat(const Texture& texture, const float values[4])
	{
		DescriptorHeap& heap = context.GetDescriptorHeap();
		auto device = context.GetD3D12Device();

		// This non-shader-visible uav needs to be identical to the texture's shader-visible uav
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = Texture::GenerateD3D12Desc_UAV(texture.GetFormat(), texture.GetMSAA(), texture.GetNumFaces());
		D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = heap.GetD3D12CPUHandle_Reusable_UAV();
		device->CreateUnorderedAccessView(texture.GetD3D12Resource(), nullptr, &uavDesc, uavHandle);

		impl.graphicsCommandList->ClearUnorderedAccessViewFloat(heap.GetD3D12GPUHandle(texture.GetUnorderedAccessDescriptor()),
			uavHandle, texture.GetD3D12Resource(), values, 0, nullptr);
	}

	void CommandList::Impl_ClearUnorderedAccessTextureUInt32(const Texture& texture, const uint32_t values[4])
	{
		DescriptorHeap& heap = context.GetDescriptorHeap();
		auto device = context.GetD3D12Device();

		// This non-shader-visible uav needs to be identical to the texture's shader-visible uav
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = Texture::GenerateD3D12Desc_UAV(texture.GetFormat(), texture.GetMSAA(), texture.GetNumFaces());
		D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = heap.GetD3D12CPUHandle_Reusable_UAV();
		device->CreateUnorderedAccessView(texture.GetD3D12Resource(), nullptr, &uavDesc, uavHandle);

		impl.graphicsCommandList->ClearUnorderedAccessViewUint(heap.GetD3D12GPUHandle(texture.GetUnorderedAccessDescriptor()),
			uavHandle, texture.GetD3D12Resource(), values, 0, nullptr);
	}

	void CommandList::Impl_SetPushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes)
	{
		impl.graphicsCommandList->SetGraphicsRoot32BitConstants(0, sizeInBytes / 4, data, destOffsetInBytes / 4);
	}

	void CommandList::Impl_SetComputePushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes)
	{
		impl.graphicsCommandList->SetComputeRoot32BitConstants(0, sizeInBytes / 4, data, destOffsetInBytes / 4);
	}

	void CommandList::Impl_SetVertexBuffers(const Buffer* const* vertexBuffers, uint32_t count, uint32_t startSlot)
	{
		D3D12_VERTEX_BUFFER_VIEW views[MAX_VERTEX_BUFFER_BIND_SLOTS];
		for (uint32_t i = 0; i < count; i++)
		{
			views[i].BufferLocation = vertexBuffers[i]->GetD3D12Resource()->GetGPUVirtualAddress();
			views[i].SizeInBytes = (UINT)vertexBuffers[i]->GetSize();
			views[i].StrideInBytes = vertexBuffers[i]->GetStride();
		}
		impl.graphicsCommandList->IASetVertexBuffers(startSlot, count, views);
	}

	void CommandList::SetIndexBuffer(const Buffer& indexBuffer)
	{
		D3D12_INDEX_BUFFER_VIEW view = {};
		view.BufferLocation = indexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
		view.SizeInBytes = (UINT)indexBuffer.GetSize();
		view.Format = (indexBuffer.GetStride() == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
		impl.graphicsCommandList->IASetIndexBuffer(&view);
	}

	void CommandList::Draw(uint32_t vertexCount, uint32_t startVertexLocation)
	{
		impl.graphicsCommandList->DrawInstanced(vertexCount, 1, startVertexLocation, 0);
	}
	void CommandList::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation,
		uint32_t startInstanceLocation)
	{
		impl.graphicsCommandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
	}
	void CommandList::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation)
	{
		impl.graphicsCommandList->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
	}
	void CommandList::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
		int32_t baseVertexLocation, uint32_t startInstanceLocation)
	{
		impl.graphicsCommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation,
			baseVertexLocation, startInstanceLocation);
	}

	void CommandList::SetViewports(const Viewport* viewPorts, uint32_t count)
	{
		impl.graphicsCommandList->RSSetViewports(count, (D3D12_VIEWPORT*)viewPorts);
	}

	void CommandList::SetScissorRectangles(const IntRect* scissorRects, uint32_t count)
	{
		impl.graphicsCommandList->RSSetScissorRects(count, (D3D12_RECT*)scissorRects);
	}

	void CommandList::DispatchCompute(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
	{
		impl.graphicsCommandList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}

	void CommandList::Impl_CopyTexture(const Texture& source, const Texture& destination)
	{
		impl.graphicsCommandList->CopyResource(destination.GetD3D12Resource(), source.GetD3D12Resource());
	}

	void CommandList::Impl_CopyTextureSubresource(const Texture& source, uint32_t sourceFaceIndex, uint32_t sourceMipIndex,
		const Texture& destination, uint32_t destFaceIndex, uint32_t destMipIndex)
	{
		D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
		srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		srcLoc.pResource = source.GetD3D12Resource();
		srcLoc.SubresourceIndex = (sourceFaceIndex * source.GetMipLevels()) + sourceMipIndex;

		D3D12_TEXTURE_COPY_LOCATION destLoc = {};
		destLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destLoc.pResource = destination.GetD3D12Resource();
		destLoc.SubresourceIndex = (destFaceIndex * destination.GetMipLevels()) + destMipIndex;

		impl.graphicsCommandList->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);
	}

	void CommandList::Impl_CopyTextureToReadableTexture(const Texture& source, const Texture& destination)
	{
		D3D12_RESOURCE_DESC desc = source.GetD3D12Resource()->GetDesc();
		uint32_t numSubResources = source.GetNumFaces() * source.GetMipLevels();
		uint64_t currentOffset = 0;
		for (uint32_t i = 0; i < numSubResources; i++)
		{
			D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
			srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			srcLoc.pResource = source.GetD3D12Resource();
			srcLoc.SubresourceIndex = i;

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
			uint64_t totalBytes = 0;
			context.GetD3D12Device()->GetCopyableFootprints(&desc, i, 1, 0, &footprint, nullptr, nullptr, &totalBytes);

			D3D12_TEXTURE_COPY_LOCATION destLoc = {};
			destLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			destLoc.pResource = destination.GetD3D12Resource();
			destLoc.PlacedFootprint = footprint;
			destLoc.PlacedFootprint.Offset = currentOffset;
			currentOffset += AlignUp(totalBytes, context.GetGraphicsSpecs().bufferPlacementAlignments.textureRowPitch);

			impl.graphicsCommandList->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);
		}
	}

	void CommandList::CopyBuffer(const Buffer& source, const Buffer& destination)
	{
		impl.graphicsCommandList->CopyResource(destination.GetD3D12Resource(), source.GetD3D12Resource());
	}

	void CommandList::CopyTempBufferToTexture(const TempBuffer& source, const Texture& destination)
	{
		D3D12_RESOURCE_DESC desc = destination.GetD3D12Resource()->GetDesc();
		uint32_t numSubResources = desc.MipLevels * desc.DepthOrArraySize;

		uint64_t currentOffset = source.offset;
		for (uint32_t i = 0; i < numSubResources; i++)
		{
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
			uint64_t totalBytes = 0;
			context.GetD3D12Device()->GetCopyableFootprints(&desc, i, 1, 0, &footprint, nullptr, nullptr, &totalBytes);

			D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
			srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			srcLoc.pResource = source.impl.resource;
			srcLoc.PlacedFootprint = footprint;
			srcLoc.PlacedFootprint.Offset = currentOffset;
			currentOffset += AlignUp(totalBytes, context.GetGraphicsSpecs().bufferPlacementAlignments.textureRowPitch);

			D3D12_TEXTURE_COPY_LOCATION destLoc = {};
			destLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			destLoc.pResource = destination.GetD3D12Resource();
			destLoc.SubresourceIndex = i;

			impl.graphicsCommandList->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);
		}
	}

	void CommandList::CopyTempBufferToTextureSubresource(const TempBuffer& source, const Texture& destination, uint32_t destFaceIndex, uint32_t destMipIndex)
	{
		D3D12_RESOURCE_DESC desc = destination.GetD3D12Resource()->GetDesc();
		uint32_t subResourceIndex = (destFaceIndex * destination.GetMipLevels()) + destMipIndex;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		uint64_t totalBytes = 0;
		context.GetD3D12Device()->GetCopyableFootprints(&desc, subResourceIndex, 1, 0, &footprint, nullptr, nullptr, &totalBytes);

		D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
		srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLoc.pResource = source.impl.resource;
		srcLoc.PlacedFootprint = footprint;
		srcLoc.PlacedFootprint.Offset = source.offset;

		D3D12_TEXTURE_COPY_LOCATION destLoc = {};
		destLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destLoc.pResource = destination.GetD3D12Resource();
		destLoc.SubresourceIndex = subResourceIndex;

		impl.graphicsCommandList->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);
	}

	void CommandList::CopyTempBufferToBuffer(const TempBuffer& source, const Buffer& destination)
	{
		impl.graphicsCommandList->CopyBufferRegion(destination.GetD3D12Resource(), 0, source.impl.resource,
			source.offset, destination.GetSize());
	}

	void CommandList::ResolveTexture(const Texture& source, const Texture& destination)
	{
		//TODO: Add support for resolving cubemaps
		impl.graphicsCommandList->ResolveSubresource(destination.GetD3D12Resource(), 0, source.GetD3D12Resource(), 0,
			GetFormatInfoDXGI(source.GetFormat()).dxgiFormat);
	}

	void Texture::Impl_Destroy()
	{
		DescriptorHeap& heap = context.GetDescriptorHeap();
		if (impl.srv)
		{
			heap.FreePersistent(impl.srv);
			impl.srv.SetToNull();
		}
		if (impl.uav)
		{
			heap.FreePersistent(impl.uav);
			impl.uav.SetToNull();
		}
		if (impl.rtv_dsv)
		{
			heap.FreePersistent(impl.rtv_dsv);
			impl.rtv_dsv.SetToNull();
		}

		if (implPerFrame)
		{
			const uint32_t arrayLength = GetPerFrameArrayLength();
			for (uint32_t i = 0; i < arrayLength; i++)
			{
				auto& frame = implPerFrame[i];
				if (frame.mapped)
				{
					D3D12_RANGE noWrite = { 0, 0 };
					frame.resource->Unmap(0, &noWrite);
					frame.mapped = nullptr;
				}
				frame.resource = nullptr;
			}
			implPerFrame = nullptr;
		}

		impl.resource = nullptr;
	}

	DetailedResult Texture::Impl_Create()
	{
		auto device = context.GetD3D12Device();
		DescriptorHeap& heap = context.GetDescriptorHeap();

		const uint32_t perFrameArrayLength = GetPerFrameArrayLength();
		const bool isMultisampledCubemap = (desc.isCubemap && desc.msaa != MSAA::Disabled);
		const FormatInfoDXGI formatInfoD3D = GetFormatInfoDXGI(desc.format);
		const bool useTypeless =
			HasFlag(desc.flags, TextureFlags::AllowMultiFormat) ||
			(!HasFlag(desc.flags, TextureFlags::DepthBuffer_DenyShaderResource) && desc.usage == TextureUsage::DepthBuffer);

		if (formatInfoD3D.dxgiFormat == DXGI_FORMAT_UNKNOWN)
		{
			return DetailedResult::Fail(ToString("This iglo format is not supported in D3D12: ", GetFormatName(desc.format)));
		}
		if (desc.usage == TextureUsage::Readable)
		{
			implPerFrame = std::make_unique<Impl_PerFrameTexture[]>(perFrameArrayLength);
		}

		const uint32_t numResources = implPerFrame ? perFrameArrayLength : 1;

		for (uint32_t i = 0; i < numResources; i++)
		{
			auto& currentResource = implPerFrame ? implPerFrame[i].resource : impl.resource;

			assert(desc.numFaces <= IGLO_UINT16_MAX);
			assert(desc.mipLevels <= IGLO_UINT16_MAX);
			D3D12_RESOURCE_DESC1 resDesc =
			{
				.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				.Alignment = 0,
				.Width = desc.extent.width,
				.Height = desc.extent.height,
				.DepthOrArraySize = (UINT16)desc.numFaces,
				.MipLevels = (UINT16)desc.mipLevels,
				.Format = useTypeless ? formatInfoD3D.typeless : formatInfoD3D.dxgiFormat,
				.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			};

			resDesc.SampleDesc =
			{
				.Count = (UINT)desc.msaa,
				.Quality = 0, // Quality = 0 uses standard MSAA. Higher values are vendor-specific.
			};


			resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			switch (desc.usage)
			{
			case TextureUsage::UnorderedAccess:
				resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				break;

			case TextureUsage::RenderTexture:
				resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
				break;

			case TextureUsage::DepthBuffer:
				resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
				if (HasFlag(desc.flags, TextureFlags::DepthBuffer_DenyShaderResource))
				{
					resDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
				}
				break;

			case TextureUsage::UnorderedAccessRenderTexture:
				resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
				break;

			default:
				break;
			}

			resDesc.SamplerFeedbackMipRegion = { .Width = 0, .Height = 0, .Depth = 0 };

			if (desc.usage == TextureUsage::Readable)
			{
				UINT64 requiredBufferSize = 0;
				device->GetCopyableFootprints1(&resDesc, 0, desc.numFaces * desc.mipLevels, 0, nullptr, nullptr, nullptr, &requiredBufferSize);
				resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				resDesc.Alignment = 0;
				resDesc.Width = requiredBufferSize;
				resDesc.Height = 1;
				resDesc.DepthOrArraySize = 1;
				resDesc.MipLevels = 1;
				resDesc.Format = DXGI_FORMAT_UNKNOWN;
				resDesc.SampleDesc.Count = 1;
				resDesc.SampleDesc.Quality = 0;
				resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			}

			D3D12_HEAP_PROPERTIES heapProp =
			{
				.Type = (desc.usage == TextureUsage::Readable) ? D3D12_HEAP_TYPE_READBACK : D3D12_HEAP_TYPE_DEFAULT,
				.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
				.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
				.CreationNodeMask = 1,
				.VisibleNodeMask = 1,
			};

			const D3D12_CLEAR_VALUE* ptrClearValue = nullptr;
			D3D12_CLEAR_VALUE clear = {};
			if (desc.usage == TextureUsage::RenderTexture || desc.usage == TextureUsage::UnorderedAccessRenderTexture)
			{
				clear.Format = formatInfoD3D.dxgiFormat;
				clear.Color[0] = desc.optimizedClearValue.color.red;
				clear.Color[1] = desc.optimizedClearValue.color.green;
				clear.Color[2] = desc.optimizedClearValue.color.blue;
				clear.Color[3] = desc.optimizedClearValue.color.alpha;
				ptrClearValue = &clear;
			}
			else if (desc.usage == TextureUsage::DepthBuffer)
			{
				clear.Format = formatInfoD3D.dxgiFormat;
				clear.DepthStencil.Depth = desc.optimizedClearValue.depth;
				clear.DepthStencil.Stencil = desc.optimizedClearValue.stencil;
				ptrClearValue = &clear;
			}

			HRESULT hr = device->CreateCommittedResource3(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
				D3D12_BARRIER_LAYOUT_UNDEFINED, ptrClearValue, nullptr, 0, nullptr, IID_PPV_ARGS(&currentResource));
			if (FAILED(hr))
			{
				return DetailedResult::Fail(D3D12ErrorMsg("ID3D12Device10::CreateCommittedResource3", hr));
			}

			if (desc.usage == TextureUsage::Readable)
			{
				hr = currentResource->Map(0, nullptr, &implPerFrame[i].mapped);
				if (FAILED(hr))
				{
					return DetailedResult::Fail(D3D12ErrorMsg("ID3D12Resource::Map", hr));
				}
			}
		}

		bool createSRV = desc.usage != TextureUsage::Readable;
		bool createUAV =
			desc.usage == TextureUsage::UnorderedAccess ||
			desc.usage == TextureUsage::UnorderedAccessRenderTexture;
		bool createRTV_DSV =
			desc.usage == TextureUsage::RenderTexture ||
			desc.usage == TextureUsage::UnorderedAccessRenderTexture ||
			desc.usage == TextureUsage::DepthBuffer;

		if (HasFlag(desc.flags, TextureFlags::SkipDescriptorCreation))
		{
			createSRV = false;
			createUAV = false;
			createRTV_DSV = false;
		}

		if (desc.usage == TextureUsage::DepthBuffer && HasFlag(desc.flags, TextureFlags::DepthBuffer_DenyShaderResource))
		{
			createSRV = false;
		}

		if (isMultisampledCubemap) // Multisampled cubemaps can't have SRVs
		{
			createSRV = false;
		}

		if (createSRV)
		{
			impl.srv = heap.AllocatePersistent(DescriptorType::Resource);
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = GenerateD3D12Desc_SRV(desc.format, desc.msaa, desc.mipLevels, desc.numFaces, desc.isCubemap);
			device->CreateShaderResourceView(impl.resource.Get(), &srvDesc, heap.GetD3D12CPUHandle(impl.srv));
		}

		if (createUAV)
		{
			impl.uav = heap.AllocatePersistent(DescriptorType::Resource);
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = GenerateD3D12Desc_UAV(desc.format, desc.msaa, desc.numFaces);
			device->CreateUnorderedAccessView(impl.resource.Get(), nullptr, &uavDesc, heap.GetD3D12CPUHandle(impl.uav));
		}

		if (createRTV_DSV)
		{
			if (desc.usage == TextureUsage::RenderTexture || desc.usage == TextureUsage::UnorderedAccessRenderTexture)
			{
				impl.rtv_dsv = heap.AllocatePersistent(DescriptorType::RenderTexture);
				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = GenerateD3D12Desc_RTV(desc.format, desc.msaa, desc.numFaces);
				device->CreateRenderTargetView(impl.resource.Get(), &rtvDesc, heap.GetD3D12CPUHandle(impl.rtv_dsv));
			}
			else if (desc.usage == TextureUsage::DepthBuffer)
			{
				impl.rtv_dsv = heap.AllocatePersistent(DescriptorType::DepthBuffer);
				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = GenerateD3D12Desc_DSV(desc.format, desc.msaa, desc.numFaces);
				device->CreateDepthStencilView(impl.resource.Get(), &dsvDesc, heap.GetD3D12CPUHandle(impl.rtv_dsv));
			}
		}

		return DetailedResult::Success();
	}

	Descriptor Texture::GetRenderDescriptor() const
	{
		if (!impl.rtv_dsv) Fatal("Texture has no RTV or DSV.");
		return impl.rtv_dsv;
	}

	ID3D12Resource* Texture::GetD3D12Resource() const
	{
		if (desc.usage == TextureUsage::Readable)
		{
			assert(implPerFrame);
			assert(context.GetFrameIndex() < GetPerFrameArrayLength());
			return implPerFrame[context.GetFrameIndex()].resource.Get();
		}
		return impl.resource.Get();
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC Texture::GenerateD3D12Desc_SRV(Format format, MSAA msaa,
		uint32_t mipLevels, uint32_t numFaces, bool isCubemap)
	{
		assert((!isCubemap || numFaces % 6 == 0) && "If cubemap, numFaces must be a multiple of 6");

		D3D12_SHADER_RESOURCE_VIEW_DESC out = {};
		out.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		out.ViewDimension = D3D12_SRV_DIMENSION_UNKNOWN;
		if (msaa == MSAA::Disabled)
		{
			if (isCubemap)
			{
				if (numFaces == 6)
				{
					out.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
					out.TextureCube.MipLevels = mipLevels;
				}
				else
				{
					out.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
					out.TextureCubeArray.MipLevels = mipLevels;
					out.TextureCubeArray.NumCubes = numFaces / 6;
				}
			}
			else
			{
				if (numFaces == 1)
				{
					out.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					out.Texture2D.MipLevels = mipLevels;
				}
				else
				{
					out.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
					out.Texture2DArray.MipLevels = mipLevels;
					out.Texture2DArray.ArraySize = numFaces;
				}
			}
		}
		else
		{
			if (numFaces == 1)
			{
				out.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
			}
			else
			{
				out.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
				out.Texture2DMSArray.ArraySize = numFaces;
			}
		}

		out.Format = GetFormatInfoDXGI(format).dxgiFormat;

		// Translate Depth to Red (D -> R)
		if (out.Format == DXGI_FORMAT_D16_UNORM) out.Format = DXGI_FORMAT_R16_UNORM;
		else if (out.Format == DXGI_FORMAT_D24_UNORM_S8_UINT) out.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		else if (out.Format == DXGI_FORMAT_D32_FLOAT) out.Format = DXGI_FORMAT_R32_FLOAT;
		else if (out.Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT) out.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

		return out;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC Texture::GenerateD3D12Desc_UAV(Format format, MSAA msaa, uint32_t numFaces)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC out = {};
		out.Format = GetFormatInfoDXGI(format).dxgiFormat;

		out.ViewDimension = D3D12_UAV_DIMENSION_UNKNOWN;
		if (numFaces > 1)
		{
			if (msaa == MSAA::Disabled)
			{
				out.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				out.Texture2DArray.ArraySize = numFaces;
				out.Texture2DArray.MipSlice = 0;
				out.Texture2DArray.FirstArraySlice = 0;
			}
			else
			{
				out.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
				out.Texture2DMSArray.ArraySize = numFaces;
				out.Texture2DMSArray.FirstArraySlice = 0;
			}
		}
		else
		{
			if (msaa == MSAA::Disabled)
			{
				out.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				out.Texture2D.MipSlice = 0;
			}
			else
			{
				out.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
			}
		}

		return out;
	}

	D3D12_RENDER_TARGET_VIEW_DESC Texture::GenerateD3D12Desc_RTV(Format format, MSAA msaa, uint32_t numFaces)
	{
		D3D12_RENDER_TARGET_VIEW_DESC out = {};
		out.Format = GetFormatInfoDXGI(format).dxgiFormat;

		out.ViewDimension = D3D12_RTV_DIMENSION_UNKNOWN;
		if (numFaces > 1)
		{
			if (msaa == MSAA::Disabled)
			{
				out.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
				out.Texture2DArray.ArraySize = numFaces;
				out.Texture2DArray.MipSlice = 0;
				out.Texture2DArray.FirstArraySlice = 0;
				out.Texture2DArray.PlaneSlice = 0;
			}
			else
			{
				out.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
				out.Texture2DMSArray.ArraySize = numFaces;
				out.Texture2DMSArray.FirstArraySlice = 0;
			}
		}
		else
		{
			if (msaa == MSAA::Disabled)
			{
				out.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				out.Texture2D.MipSlice = 0;
				out.Texture2D.PlaneSlice = 0;
			}
			else
			{
				out.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
			}
		}

		return out;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC Texture::GenerateD3D12Desc_DSV(Format format, MSAA msaa, uint32_t numFaces)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC out = {};
		out.Format = GetFormatInfoDXGI(format).dxgiFormat;
		out.Flags = D3D12_DSV_FLAG_NONE;

		out.ViewDimension = D3D12_DSV_DIMENSION_UNKNOWN;
		if (numFaces > 1)
		{
			if (msaa == MSAA::Disabled)
			{
				out.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
				out.Texture2DArray.ArraySize = numFaces;
				out.Texture2DArray.MipSlice = 0;
				out.Texture2DArray.FirstArraySlice = 0;
			}
			else
			{
				out.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
				out.Texture2DMSArray.ArraySize = numFaces;
				out.Texture2DMSArray.FirstArraySlice = 0;
			}
		}
		else
		{
			if (msaa == MSAA::Disabled)
			{
				out.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				out.Texture2D.MipSlice = 0;
			}
			else
			{
				out.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
			}
		}

		return out;
	}

	void Buffer::Impl_Destroy()
	{
		DescriptorHeap& heap = context.GetDescriptorHeap();
		if (impl.cbv_srv)
		{
			heap.FreePersistent(impl.cbv_srv);
			impl.cbv_srv.SetToNull();
		}
		if (impl.uav)
		{
			heap.FreePersistent(impl.uav);
			impl.uav.SetToNull();
		}

		if (implPerFrame)
		{
			const uint32_t arrayLength = GetPerFrameArrayLength();
			for (uint32_t i = 0; i < arrayLength; i++)
			{
				auto& frame = implPerFrame[i];

				if (frame.mapped)
				{
					if (desc.usage == BufferUsage::Dynamic)
					{
						frame.resource->Unmap(0, nullptr);
					}
					else if (desc.usage == BufferUsage::Readable)
					{
						D3D12_RANGE noWrite = { 0, 0 };
						frame.resource->Unmap(0, &noWrite);
					}
					else
					{
						Fatal("Unexpected buffer usage for mapped ptr.");
					}
					frame.mapped = nullptr;
				}

				if (frame.cbv_srv)
				{
					heap.FreePersistent(frame.cbv_srv);
					frame.cbv_srv.SetToNull();
				}
				if (frame.uav)
				{
					heap.FreePersistent(frame.uav);
					frame.uav.SetToNull();
				}

				frame.resource = nullptr;
			}
			implPerFrame = nullptr;
		}

		impl.resource = nullptr;
	}

	DetailedResult Buffer::Impl_Create()
	{
		auto device = context.GetD3D12Device();
		DescriptorHeap& heap = context.GetDescriptorHeap();
		const uint32_t perFrameArrayLength = GetPerFrameArrayLength();

		if (desc.usage == BufferUsage::Readable ||
			desc.usage == BufferUsage::Dynamic)
		{
			implPerFrame = std::make_unique<Impl_PerFrameBuffer[]>(perFrameArrayLength);
		}

		uint32_t numBuffers = implPerFrame ? perFrameArrayLength : 1;
		for (uint32_t i = 0; i < numBuffers; i++)
		{
			auto& currentResource = implPerFrame ? implPerFrame[i].resource : impl.resource;

			D3D12_RESOURCE_DESC1 resDesc =
			{
				.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
				.Alignment = 0,
				.Height = 1,
				.DepthOrArraySize = 1,
				.MipLevels = 1,
				.Format = DXGI_FORMAT_UNKNOWN,
				.SampleDesc = {.Count = 1, .Quality = 0},
				.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
				.SamplerFeedbackMipRegion = {.Width = 0, .Height = 0, .Depth = 0},
			};

			resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			if (desc.usage == BufferUsage::UnorderedAccess) resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			resDesc.Width = desc.size;
			if (desc.type == BufferType::ShaderConstant)
			{
				resDesc.Width = AlignUp(desc.size, context.GetGraphicsSpecs().bufferPlacementAlignments.constant);
			}

			D3D12_HEAP_PROPERTIES heapProp =
			{
				.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
				.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
				.CreationNodeMask = 1,
				.VisibleNodeMask = 1,
			};

			heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
			if (desc.usage == BufferUsage::Dynamic) heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
			else if (desc.usage == BufferUsage::Readable) heapProp.Type = D3D12_HEAP_TYPE_READBACK;

			D3D12_BARRIER_LAYOUT barrierLayout = D3D12_BARRIER_LAYOUT_UNDEFINED;

			HRESULT hr = device->CreateCommittedResource3(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, barrierLayout,
				nullptr, nullptr, 0, nullptr, IID_PPV_ARGS(&currentResource));
			if (FAILED(hr))
			{
				return DetailedResult::Fail(D3D12ErrorMsg("ID3D12Device10::CreateCommittedResource3", hr));
			}

			if (desc.usage == BufferUsage::Dynamic ||
				desc.usage == BufferUsage::Readable)
			{
				assert(implPerFrame);
				D3D12_RANGE noRead = { 0, 0 };
				hr = currentResource->Map(0, (desc.usage == BufferUsage::Readable) ? nullptr : &noRead, &implPerFrame[i].mapped);
				if (FAILED(hr))
				{
					return DetailedResult::Fail(D3D12ErrorMsg("ID3D12Resource::Map", hr));
				}
			}
		}

		if (desc.usage != BufferUsage::Readable)
		{
			const uint32_t numDescriptors = implPerFrame ? perFrameArrayLength : 1;
			for (uint32_t i = 0; i < numDescriptors; i++)
			{
				auto& currentResource = implPerFrame ? implPerFrame[i].resource : impl.resource;
				auto& currentCBV_SRV = implPerFrame ? implPerFrame[i].cbv_srv : impl.cbv_srv;
				auto& currentUAV = implPerFrame ? implPerFrame[i].uav : impl.uav;

				if (desc.type == BufferType::ShaderConstant)
				{
					currentCBV_SRV = heap.AllocatePersistent(DescriptorType::Resource);
					D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = GenerateD3D12Desc_CBV(currentResource.Get(),
						context.GetGraphicsSpecs().bufferPlacementAlignments, desc.size);
					device->CreateConstantBufferView(&cbvDesc, heap.GetD3D12CPUHandle(currentCBV_SRV));
				}
				else if (desc.type == BufferType::StructuredBuffer)
				{
					currentCBV_SRV = heap.AllocatePersistent(DescriptorType::Resource);
					D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = GenerateD3D12Desc_SRV_Structured(desc.numElements, desc.stride);
					device->CreateShaderResourceView(currentResource.Get(), &srvDesc, heap.GetD3D12CPUHandle(currentCBV_SRV));

					if (desc.usage == BufferUsage::UnorderedAccess)
					{
						currentUAV = heap.AllocatePersistent(DescriptorType::Resource);
						D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = GenerateD3D12Desc_UAV_Structured(desc.numElements, desc.stride);
						device->CreateUnorderedAccessView(currentResource.Get(), nullptr, &uavDesc, heap.GetD3D12CPUHandle(currentUAV));
					}
				}
				else if (desc.type == BufferType::RawBuffer)
				{
					currentCBV_SRV = heap.AllocatePersistent(DescriptorType::Resource);
					D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = GenerateD3D12Desc_SRV_Raw(desc.size);
					device->CreateShaderResourceView(currentResource.Get(), &srvDesc, heap.GetD3D12CPUHandle(currentCBV_SRV));

					if (desc.usage == BufferUsage::UnorderedAccess)
					{
						currentUAV = heap.AllocatePersistent(DescriptorType::Resource);
						D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = GenerateD3D12Desc_UAV_Raw(desc.size);
						device->CreateUnorderedAccessView(currentResource.Get(), nullptr, &uavDesc, heap.GetD3D12CPUHandle(currentUAV));
					}
				}
			}
		}

		return DetailedResult::Success();
	}

	ID3D12Resource* Buffer::GetD3D12Resource() const
	{
		switch (desc.usage)
		{
		case BufferUsage::Default:
		case BufferUsage::UnorderedAccess:
			return impl.resource.Get();
		case BufferUsage::Readable:
			return implPerFrame[context.GetFrameIndex()].resource.Get();
		case BufferUsage::Dynamic:
			return implPerFrame[dynamicSetCounter].resource.Get();
		default:
			Fatal("Invalid buffer usage.");
		}
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC Buffer::GenerateD3D12Desc_CBV(ID3D12Resource* bufferLocation,
		const BufferPlacementAlignments& bufferPlacementAlignments, uint64_t sizeOfData)
	{
		assert(bufferLocation);
		D3D12_CONSTANT_BUFFER_VIEW_DESC out =
		{
			.BufferLocation = bufferLocation->GetGPUVirtualAddress(),
			.SizeInBytes = (UINT)AlignUp(sizeOfData, bufferPlacementAlignments.constant),
		};
		return out;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC Buffer::GenerateD3D12Desc_SRV_Structured(uint32_t numElements, uint32_t stride, uint64_t firstElement)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC out =
		{
			.Format = DXGI_FORMAT_UNKNOWN,
			.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer =
			{
				.FirstElement = firstElement,
				.NumElements = numElements,
				.StructureByteStride = stride,
				.Flags = D3D12_BUFFER_SRV_FLAG_NONE,
			},
		};
		return out;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC Buffer::GenerateD3D12Desc_SRV_Raw(uint64_t dataSize, uint64_t firstElement)
	{
		assert(dataSize % 4 == 0 && "size must be a multiple of 4");
		D3D12_SHADER_RESOURCE_VIEW_DESC out =
		{
			.Format = DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer =
			{
				.FirstElement = firstElement,
				.NumElements = UINT(dataSize / sizeof(uint32_t)),
				.StructureByteStride = 0,
				.Flags = D3D12_BUFFER_SRV_FLAG_RAW,
			},
		};
		return out;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC Buffer::GenerateD3D12Desc_UAV_Structured(uint32_t numElements, uint32_t stride, uint64_t firstElement)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC out =
		{
			.Format = DXGI_FORMAT_UNKNOWN,
			.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
			.Buffer =
			{
				.FirstElement = firstElement,
				.NumElements = numElements,
				.StructureByteStride = stride,
				.CounterOffsetInBytes = 0,
				.Flags = D3D12_BUFFER_UAV_FLAG_NONE,
			},
		};
		return out;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC Buffer::GenerateD3D12Desc_UAV_Raw(uint64_t dataSize, uint64_t firstElement)
	{
		assert(dataSize % 4 == 0 && "size must be a multiple of 4");
		D3D12_UNORDERED_ACCESS_VIEW_DESC out =
		{
			.Format = DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
			.Buffer =
			{
				.FirstElement = 0,
				.NumElements = UINT(dataSize / sizeof(uint32_t)),
				.StructureByteStride = 0,
				.CounterOffsetInBytes = 0,
				.Flags = D3D12_BUFFER_UAV_FLAG_RAW,
			},
		};
		return out;
	}

	DetailedResult Sampler::Impl_Create(const SamplerDesc& desc)
	{
		auto device = context.GetD3D12Device();
		DescriptorHeap& heap = context.GetDescriptorHeap();

		descriptor = heap.AllocatePersistent(DescriptorType::Sampler);

		D3D12TextureFilter filterD3D = ToD3D12TextureFilter(desc.filter);

		D3D12_SAMPLER_DESC samplerDesc =
		{
			.Filter = filterD3D.filter,
			.AddressU = (D3D12_TEXTURE_ADDRESS_MODE)desc.wrapU,
			.AddressV = (D3D12_TEXTURE_ADDRESS_MODE)desc.wrapV,
			.AddressW = (D3D12_TEXTURE_ADDRESS_MODE)desc.wrapW,
			.MipLODBias = desc.mipMapLODBias,
			.MaxAnisotropy = filterD3D.maxAnisotropy,
			.ComparisonFunc = (D3D12_COMPARISON_FUNC)desc.comparisonFunc,
			.BorderColor =
			{
				desc.borderColor.red,
				desc.borderColor.green,
				desc.borderColor.blue,
				desc.borderColor.alpha,
			},
			.MinLOD = desc.minLOD,
			.MaxLOD = desc.maxLOD,
		};
		device->CreateSampler(&samplerDesc, heap.GetD3D12CPUHandle(descriptor));

		return DetailedResult::Success();
	}

	void UploadHeap::Page::Impl_Free(const IGLOContext& context)
	{
		if (impl.resource)
		{
			impl.resource->Unmap(0, nullptr);
			impl.resource = nullptr;
		}
	}

	UploadHeap::Page UploadHeap::Page::Create(const IGLOContext& context, uint64_t sizeInBytes)
	{
		Page out;

		D3D12_RESOURCE_DESC1 desc =
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0,
			.Width = sizeInBytes,
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {.Count = 1, .Quality = 0},
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE,
			.SamplerFeedbackMipRegion = {.Width = 0, .Height = 0, .Depth = 0},
		};

		D3D12_HEAP_PROPERTIES heapProp =
		{
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1,
		};

		D3D12_BARRIER_LAYOUT barrierLayout = D3D12_BARRIER_LAYOUT_UNDEFINED;

		HRESULT hr = context.GetD3D12Device()->CreateCommittedResource3(&heapProp, D3D12_HEAP_FLAG_NONE, &desc, barrierLayout,
			nullptr, nullptr, 0, nullptr, IID_PPV_ARGS(&out.impl.resource));
		if (FAILED(hr)) Fatal(ToString("Failed to create upload heap buffer with size ", sizeInBytes, "."));

		D3D12_RANGE noRead = { 0, 0 };
		hr = out.impl.resource->Map(0, &noRead, &out.mapped);
		if (FAILED(hr)) Fatal(ToString("Failed to map upload heap buffer with size ", sizeInBytes, "."));

		out.sizeInBytes = sizeInBytes;
		return out;
	}

	void DescriptorHeap::Impl_Destroy()
	{
		impl.bindlessRootSignature = nullptr;

		impl.heap_CBV_SRV_UAV = nullptr;
		impl.heap_Samplers = nullptr;
		impl.heap_RTV = nullptr;
		impl.heap_DSV = nullptr;

		impl.heap_Reusable_UAV = nullptr;
	}

	DetailedResult DescriptorHeap::Impl_Create()
	{
		auto device = context.GetD3D12Device();

		// Bindless root signature
		{
			D3D12_ROOT_PARAMETER1 rootConstants = {};
			rootConstants.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			rootConstants.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootConstants.Constants.Num32BitValues = MAX_PUSH_CONSTANTS_BYTE_SIZE / 4;
			rootConstants.Constants.RegisterSpace = 0;
			rootConstants.Constants.ShaderRegister = 0;

			D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
			versionedRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

			D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc = versionedRootSignatureDesc.Desc_1_1;
			rootSignatureDesc.NumParameters = 1;
			rootSignatureDesc.pParameters = &rootConstants;
			rootSignatureDesc.NumStaticSamplers = 0;
			rootSignatureDesc.pStaticSamplers = nullptr;
			rootSignatureDesc.Flags =
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
				D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
				D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;
			HRESULT hr = D3D12SerializeVersionedRootSignature(&versionedRootSignatureDesc, &signature, &error);
			if (FAILED(hr))
			{
				return DetailedResult::Fail(D3D12ErrorMsg("D3D12SerializeRootSignature", hr));
			}

			hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
				IID_PPV_ARGS(&impl.bindlessRootSignature));
			if (FAILED(hr))
			{
				return DetailedResult::Fail(D3D12ErrorMsg("CreateRootSignature", hr));
			}
		}

		// Shader-visible descriptor heaps
		{
			// Resources
			D3D12_DESCRIPTOR_HEAP_DESC resHeap = {};
			resHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			resHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			resHeap.NumDescriptors = CalcTotalDescriptors(limits.persistentResources, limits.tempResourcesPerFrame, maxFramesInFlight);
			HRESULT hrRes = device->CreateDescriptorHeap(&resHeap, IID_PPV_ARGS(&impl.heap_CBV_SRV_UAV));

			// Samplers
			D3D12_DESCRIPTOR_HEAP_DESC samplerHeap = {};
			samplerHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			samplerHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			samplerHeap.NumDescriptors = limits.samplers;
			HRESULT hrSampler = device->CreateDescriptorHeap(&samplerHeap, IID_PPV_ARGS(&impl.heap_Samplers));

			if (FAILED(hrRes) || FAILED(hrSampler))
			{
				return DetailedResult::Fail("Failed to create shader-visible descriptor heaps.");
			}
		}

		// Non-shader-visible descriptor heaps
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
			rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvDesc.NumDescriptors = limits.renderTextures;

			D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
			dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvDesc.NumDescriptors = limits.depthBuffers;

			HRESULT hrRTV = device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&impl.heap_RTV));
			HRESULT hrDSV = device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&impl.heap_DSV));

			if (FAILED(hrRTV) || FAILED(hrDSV))
			{
				return DetailedResult::Fail("Failed to create non-shader-visible descriptor heaps.");
			}
		}

		// Reusable non-shader-visible descriptor heaps
		{
			D3D12_DESCRIPTOR_HEAP_DESC uavDesc = {};
			uavDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // We just need the UAV
			uavDesc.NumDescriptors = 1;

			HRESULT hrUAV = device->CreateDescriptorHeap(&uavDesc, IID_PPV_ARGS(&impl.heap_Reusable_UAV));

			if (FAILED(hrUAV))
			{
				return DetailedResult::Fail("Failed to create reusable descriptor heaps.");
			}
		}

		impl.descriptorSize_CBV_SRV_UAV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		impl.descriptorSize_Sampler = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		impl.descriptorSize_RTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		impl.descriptorSize_DSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		return DetailedResult::Success();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetD3D12CPUHandle(Descriptor descriptor) const
	{
		ID3D12DescriptorHeap* targetHeap = GetD3D12DescriptorHeap(descriptor.type);
		D3D12_CPU_DESCRIPTOR_HANDLE handle = targetHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += ((SIZE_T)GetD3D12DescriptorSize(descriptor.type) * (SIZE_T)descriptor.heapIndex);
		return handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetD3D12GPUHandle(Descriptor descriptor) const
	{
		ID3D12DescriptorHeap* targetHeap = GetD3D12DescriptorHeap(descriptor.type);
		D3D12_GPU_DESCRIPTOR_HANDLE handle = targetHeap->GetGPUDescriptorHandleForHeapStart();
		handle.ptr += ((UINT64)GetD3D12DescriptorSize(descriptor.type) * (UINT64)descriptor.heapIndex);
		return handle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetD3D12CPUHandle_Reusable_UAV() const
	{
		return impl.heap_Reusable_UAV->GetCPUDescriptorHandleForHeapStart();
	}

	ID3D12DescriptorHeap* DescriptorHeap::GetD3D12DescriptorHeap(DescriptorType type) const
	{
		switch (type)
		{
		case DescriptorType::Resource: return impl.heap_CBV_SRV_UAV.Get();
		case DescriptorType::Sampler: return impl.heap_Samplers.Get();
		case DescriptorType::RenderTexture: return impl.heap_RTV.Get();
		case DescriptorType::DepthBuffer: return impl.heap_DSV.Get();

		default:
			Fatal("Invalid DescriptorType");
		}
	}

	uint32_t DescriptorHeap::GetD3D12DescriptorSize(DescriptorType type) const
	{
		switch (type)
		{
		case DescriptorType::Resource: return impl.descriptorSize_CBV_SRV_UAV;
		case DescriptorType::Sampler: return impl.descriptorSize_Sampler;
		case DescriptorType::RenderTexture: return impl.descriptorSize_RTV;
		case DescriptorType::DepthBuffer: return impl.descriptorSize_DSV;

		default:
			Fatal("Invalid DescriptorType");
		}
	}

	VideoMemoryInfo IGLOContext::QueryVideoMemoryInfo()
	{
		ComPtr<IDXGIAdapter3> adapter3;
		graphics.adapter.As(&adapter3);

		DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
		adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);

		VideoMemoryInfo out;
		out.totalVRAM = info.Budget;
		out.usedVRAM = info.CurrentUsage;
		out.availableVRAM = info.Budget - info.CurrentUsage;
		return out;
	}

	uint32_t IGLOContext::Impl_GetMaxMultiSampleCount(Format textureFormat) const
	{
		DXGI_FORMAT dxgiFormat = GetFormatInfoDXGI(textureFormat).dxgiFormat;

		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS x16 = { dxgiFormat, 16 };
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS x8 = { dxgiFormat, 8 };
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS x4 = { dxgiFormat, 4 };
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS x2 = { dxgiFormat, 2 };

		bool x16_success = SUCCEEDED(graphics.device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &x16, sizeof(x16)));
		bool x8_success = SUCCEEDED(graphics.device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &x8, sizeof(x8)));
		bool x4_success = SUCCEEDED(graphics.device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &x4, sizeof(x4)));
		bool x2_success = SUCCEEDED(graphics.device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &x2, sizeof(x2)));

		uint32_t maxMSAA = 1;
		if (x2_success && x2.NumQualityLevels > 0) maxMSAA = 2;
		if (x4_success && x4.NumQualityLevels > 0) maxMSAA = 4;
		if (x8_success && x8.NumQualityLevels > 0) maxMSAA = 8;
		if (x16_success && x16.NumQualityLevels > 0) maxMSAA = 16;

		return maxMSAA;
	}

	uint32_t IGLOContext::GetCurrentBackBufferIndex() const
	{
		return graphics.swapChain->GetCurrentBackBufferIndex();
	}

	void IGLOContext::SetPresentMode(PresentMode presentMode)
	{
		// D3D12 allows changing present modes dynamically without recreating the swapchain.
		swapChain.presentMode = presentMode;
	}

	void IGLOContext::DestroySwapChainResources()
	{
		for (const auto& surface : swapChain.wrapped)
		{
			descriptorHeap->FreePersistent(surface->GetRenderDescriptor());
		}
		for (const auto& surface : swapChain.wrapped_sRGB_opposite)
		{
			descriptorHeap->FreePersistent(surface->GetRenderDescriptor());
		}

		swapChain = SwapChainInfo();
	}

	DetailedResult IGLOContext::CreateSwapChain(Extent2D extent, Format format, uint32_t numBackBuffers,
		uint32_t numFramesInFlight, PresentMode presentMode)
	{
		DestroySwapChainResources();

		graphics.swapChain = nullptr;

		FormatInfo formatInfo = GetFormatInfo(format);
		Format format_non_sRGB = formatInfo.is_sRGB ? formatInfo.sRGB_opposite : format;

		swapChain.extent = extent;
		swapChain.format = format;
		swapChain.presentMode = presentMode;
		swapChain.numBackBuffers = numBackBuffers;
		swapChain.renderTargetDesc = RenderTargetDesc({ format });
		swapChain.renderTargetDesc_sRGB_opposite = (formatInfo.sRGB_opposite != Format::None)
			? RenderTargetDesc({ formatInfo.sRGB_opposite })
			: RenderTargetDesc({});

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = numBackBuffers;
		swapChainDesc.Width = extent.width;
		swapChainDesc.Height = extent.height;
		swapChainDesc.Format = GetFormatInfoDXGI(format_non_sRGB).dxgiFormat;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SampleDesc.Quality = 0; // Quality level is hardware and driver specific. 0 = MSAA.
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SwapEffect = d3d12SwapEffect;
		swapChainDesc.Flags = d3d12SwapChainFlags;
		ComPtr<IDXGISwapChain1> swapChain1;
		HRESULT hr = graphics.factory->CreateSwapChainForHwnd(commandQueue->GetD3D12CommandQueue(CommandListType::Graphics),
			window.hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
		if (FAILED(hr))
		{
			return DetailedResult::Fail(D3D12ErrorMsg("ID3D12Device::CreateSwapChainForHwnd", hr));
		}
		hr = swapChain1.As(&graphics.swapChain);
		if (FAILED(hr))
		{
			return DetailedResult::Fail(D3D12ErrorMsg("ComPtr::As(IDXGISwapChain3)", hr));
		}
		hr = graphics.swapChain->SetMaximumFrameLatency(numFramesInFlight);
		if (FAILED(hr))
		{
			Log(LogType::Warning, "Failed to set swap chain maximum frame latency.");
		}

		// Don't use the default fullscreen mode
		hr = graphics.factory->MakeWindowAssociation(window.hwnd, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_PRINT_SCREEN);
		if (FAILED(hr))
		{
			Log(LogType::Warning, "Failed to disable IDXGISwapChain Alt-Enter fullscreen toggle.");
		}

		// Wrap backbuffers
		for (uint32_t i = 0; i < numBackBuffers; i++)
		{
			ComPtr<ID3D12Resource> res;
			hr = graphics.swapChain->GetBuffer(i, IID_PPV_ARGS(&res));
			if (FAILED(hr))
			{
				swapChain.wrapped.clear();
				swapChain.wrapped_sRGB_opposite.clear();
				return DetailedResult::Fail(D3D12ErrorMsg("IDXGISwapChain3::GetBuffer", hr));
			}

			const TextureDesc desc =
			{
				.extent = extent,
				.format = format,
				.usage = TextureUsage::RenderTexture,
				.numFaces = 1,
				.mipLevels = 1,
			};

			Impl_Texture impl;
			impl.resource = res;
			impl.rtv_dsv = descriptorHeap->AllocatePersistent(DescriptorType::RenderTexture);

			// Write to RTV
			{
				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = Texture::GenerateD3D12Desc_RTV(format, MSAA::Disabled, 1);
				D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = descriptorHeap->GetD3D12CPUHandle(impl.rtv_dsv);
				graphics.device->CreateRenderTargetView(res.Get(), &rtvDesc, rtvHandle);
			}

			swapChain.wrapped.push_back(std::move(Texture::CreateWrapped(*this, desc, impl)));

			// sRGB opposite views
			if (formatInfo.sRGB_opposite != Format::None)
			{
				impl.rtv_dsv = descriptorHeap->AllocatePersistent(DescriptorType::RenderTexture);

				// Write to RTV
				{
					D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = Texture::GenerateD3D12Desc_RTV(formatInfo.sRGB_opposite, MSAA::Disabled, 1);
					D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = descriptorHeap->GetD3D12CPUHandle(impl.rtv_dsv);
					graphics.device->CreateRenderTargetView(res.Get(), &rtvDesc, rtvHandle);
				}

				swapChain.wrapped_sRGB_opposite.push_back(std::move(Texture::CreateWrapped(*this, desc, impl)));
			}
		}

		return DetailedResult::Success();
	}

	bool IGLOContext::ResizeD3D12SwapChain(Extent2D extent)
	{
		// It's possible that the window related code will call this function
		// before the graphics device has been initialized. Ignore those calls.
		if (!isGraphicsDeviceInitialized) return false;

		const char* errStr = "Failed to resize swapchain. Reason: ";

		if (extent.width == 0 || extent.height == 0)
		{
			Log(LogType::Error, ToString(errStr, "Width and height must be nonzero."));
			return false;
		}

		const Extent2D oldExtent = swapChain.extent;

		WaitForIdleDevice();

		window.activeResizing = false;
		window.activeMenuLoop = false;

		// Must release all backbuffer references first
		swapChain.wrapped.clear();
		swapChain.wrapped_sRGB_opposite.clear();

		FormatInfo formatInfo = GetFormatInfo(swapChain.format);
		Format format_non_sRGB = formatInfo.is_sRGB ? formatInfo.sRGB_opposite : swapChain.format;
		FormatInfoDXGI formatInfoD3D = GetFormatInfoDXGI(format_non_sRGB);

		HRESULT hr = graphics.swapChain->ResizeBuffers(0, extent.width, extent.height, formatInfoD3D.dxgiFormat, d3d12SwapChainFlags);
		if (FAILED(hr))
		{
			Log(LogType::Error, ToString(errStr, D3D12ErrorMsg("IDXGISwapChain3::ResizeBuffers", hr)));
			return false;
		}

		swapChain.extent = extent;

		for (uint32_t i = 0; i < swapChain.numBackBuffers; i++)
		{
			ComPtr<ID3D12Resource> res;
			hr = graphics.swapChain->GetBuffer(i, IID_PPV_ARGS(&res));
			if (FAILED(hr))
			{
				Log(LogType::Error, ToString(errStr, D3D12ErrorMsg("IDXGISwapChain3::GetBuffer", hr)));
				swapChain.wrapped.clear();
				swapChain.wrapped_sRGB_opposite.clear();
				return false;
			}

			const TextureDesc desc =
			{
				.extent = extent,
				.format = swapChain.format,
				.usage = TextureUsage::RenderTexture,
				.numFaces = 1,
				.mipLevels = 1,
			};

			Impl_Texture impl;
			impl.resource = res;
			impl.rtv_dsv = descriptorHeap->AllocatePersistent(DescriptorType::RenderTexture);

			// Write to RTV
			{
				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = Texture::GenerateD3D12Desc_RTV(swapChain.format, MSAA::Disabled, 1);
				D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = descriptorHeap->GetD3D12CPUHandle(impl.rtv_dsv);
				graphics.device->CreateRenderTargetView(res.Get(), &rtvDesc, rtvHandle);
			}

			swapChain.wrapped.push_back(std::move(Texture::CreateWrapped(*this, desc, impl)));

			// sRGB opposite views
			if (formatInfo.sRGB_opposite != Format::None)
			{
				impl.rtv_dsv = descriptorHeap->AllocatePersistent(DescriptorType::RenderTexture);

				// Write to RTV
				{
					D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = Texture::GenerateD3D12Desc_RTV(formatInfo.sRGB_opposite, MSAA::Disabled, 1);
					D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = descriptorHeap->GetD3D12CPUHandle(impl.rtv_dsv);
					graphics.device->CreateRenderTargetView(res.Get(), &rtvDesc, rtvHandle);
				}

				swapChain.wrapped_sRGB_opposite.push_back(std::move(Texture::CreateWrapped(*this, desc, impl)));
			}
		}

		commandQueue->SubmitSignal(CommandListType::Graphics);
		WaitForIdleDevice();

		// If swapchain changed size
		if (oldExtent != swapChain.extent)
		{
			Event event_out;
			event_out.type = EventType::Resize;
			event_out.resize = extent;
			eventQueue.push(event_out);
		}

		return true;
	}

	DetailedResult IsAdapterCompatible(ID3D12Device* device)
	{
		std::vector<std::string> missingFeatures;

		// Shader model 6.6 (required for Bindless rendering)
		D3D12_FEATURE_DATA_SHADER_MODEL sm = { .HighestShaderModel = D3D_SHADER_MODEL_6_6 };
		if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm))) ||
			sm.HighestShaderModel < D3D_SHADER_MODEL_6_6)
		{
			missingFeatures.push_back("Shader Model 6.6");
		}

		// Enhanced barriers
		D3D12_FEATURE_DATA_D3D12_OPTIONS12 opt12 = {};
		if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &opt12, sizeof(opt12))) ||
			!opt12.EnhancedBarriersSupported)
		{
			missingFeatures.push_back("Enhanced Barriers");
		}

		// Resource Binding Tier 3 (required for Bindless rendering)
		D3D12_FEATURE_DATA_D3D12_OPTIONS opt = {};
		if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &opt, sizeof(opt))) ||
			opt.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_3)
		{
			missingFeatures.push_back("Resource Binding Tier 3");
		}

		// Root signature 1.1
		D3D12_FEATURE_DATA_ROOT_SIGNATURE rs = {};
		rs.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rs, sizeof(rs))) ||
			rs.HighestVersion < D3D_ROOT_SIGNATURE_VERSION_1_1)
		{
			missingFeatures.push_back("Root signature 1.1");
		}

		if (missingFeatures.empty()) return DetailedResult::Success();

		std::string errMsg;
		for (const auto& feature : missingFeatures)
		{
			if (!errMsg.empty()) errMsg.append(", ");
			errMsg.append(feature);
		}
		return DetailedResult::Fail(errMsg);
	}

	DetailedResult IGLOContext::Impl_InitGraphicsDevice()
	{
		const char* strLatestDrivers = "Please ensure that you have the latest graphics drivers installed.";
		const char* strMinimumWindowsVersion = "IGLO requires Windows 10 version 1909 or later.";
		HRESULT hr = 0;
		UINT factoryFlags = 0;

#ifndef NDEBUG
		{
			ComPtr<ID3D12Debug> debugInterface;
			hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
			if (SUCCEEDED(hr))
			{
				debugInterface->EnableDebugLayer();
				factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
			}
			else
			{
				Log(LogType::Warning, "Failed to enable the D3D12 debug layer.");
			}
		}
#endif

		hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&graphics.factory));
		if (FAILED(hr))
		{
			return DetailedResult::Fail(ToString(D3D12ErrorMsg("CreateDXGIFactory2", hr), " ", strMinimumWindowsVersion));
		}

		// Find a suitable adapter
		ComPtr<IDXGIAdapter1> adapter;
		std::string adapterErrors;
		const DXGI_GPU_PREFERENCE gpuPref = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
		for (UINT i = 0; graphics.factory->EnumAdapterByGpuPreference(i, gpuPref, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; i++)
		{
			DXGI_ADAPTER_DESC1 adapterDesc = {};
			adapter->GetDesc1(&adapterDesc);

			// Ignore software adapters
			if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				adapter.Reset();
				continue;
			}

			const std::string adapterName = utf16_to_utf8(reinterpret_cast<const char16_t*>(adapterDesc.Description));

			// Create device with this adapter
			ComPtr<ID3D12Device10> device;
			if (FAILED(D3D12CreateDevice(adapter.Get(), d3d12FeatureLevel, IID_PPV_ARGS(&device))))
			{
				if (!adapterErrors.empty()) adapterErrors.append("\n\n");
				adapterErrors.append("[" + adapterName + "]\n" + "Does not support Feature Level " + FeatureLevelToString(d3d12FeatureLevel));
				adapter.Reset();
				continue;
			}

			DetailedResult isCompatible = IsAdapterCompatible(device.Get());
			if (!isCompatible)
			{
				if (!adapterErrors.empty()) adapterErrors.append("\n\n");
				adapterErrors.append("[" + adapterName + "]\nMissing features: " + isCompatible.errorMessage);
				adapter.Reset();
				continue;
			}

			// specs for D3D_FEATURE_LEVEL_12_1
			graphicsSpecs.apiName = "Direct3D 12";
			graphicsSpecs.rendererName = adapterName;
			graphicsSpecs.vendorName = GetGpuVendorNameFromID(adapterDesc.VendorId);
			graphicsSpecs.maxTextureDimension = 16384;
			graphicsSpecs.bufferPlacementAlignments =
			{
				.vertexOrIndexBuffer = 4,
				.rawOrStructuredBuffer = 16,
				.constant = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT,
				.texture = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT,
				.textureRowPitch = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT,
			};
			graphicsSpecs.supportedPresentModes =
			{
				.immediate = true,
				.mailbox = true,
				.vsync = true,
				.vsyncHalf = true,
			};

			graphics.adapter = adapter;
			graphics.device = device;
			return DetailedResult::Success();
		}

		// Couldn't find a suitable adapter
		return DetailedResult::Fail(ToString(
			"Unable to find a hardware adapter that supports the required features.\n\n",
			adapterErrors, "\n\n",
			strLatestDrivers));
	}

	void IGLOContext::Impl_DestroyGraphicsDevice()
	{
		graphics.adapter = nullptr;
		graphics.factory = nullptr;
		graphics.swapChain = nullptr;
		graphics.device = nullptr; // The device is destroyed last
	}

}

#endif