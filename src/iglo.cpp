
#include "iglo.h"
#include "shaders/CS_GenerateMipmaps.h"


#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

/*
	Defining __STDC_LIB_EXT1__ here is a quick workaround to stop stb_image_write.h from causing
	this visual studio compile error: "error C4996: 'sprintf': This function or variable may be unsafe."
*/
#ifdef _WIN32
#define __STDC_LIB_EXT1__
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"


#ifdef __linux__
#include <X11/Xutil.h>
#include <X11/keysymdef.h>
#include <X11/XKBlib.h>
#endif

namespace ig
{
	const BlendDesc BlendDesc::BlendDisabled =
	{
		false, // enabled
		false, // logicOpEnabled
		BlendData::SourceAlpha, BlendData::InverseSourceAlpha, BlendOperation::Add,
		BlendData::One, BlendData::InverseSourceAlpha, BlendOperation::Add,
		LogicOp::NoOperation,
		(byte)ColorWriteMask::All
	};
	const BlendDesc BlendDesc::StraightAlpha =
	{
		true, // enabled
		false, // logicOpEnabled
		BlendData::SourceAlpha, BlendData::InverseSourceAlpha, BlendOperation::Add,
		// Dest alpha is affected by src alpha, which is useful for when drawing to transparent rendertargets.
		BlendData::One, BlendData::InverseSourceAlpha, BlendOperation::Add,
		LogicOp::NoOperation,
		(byte)ColorWriteMask::All
	};
	const BlendDesc BlendDesc::PremultipliedAlpha =
	{
		true, // enabled
		false, // logicOpEnabled
		BlendData::One, BlendData::InverseSourceAlpha, BlendOperation::Add,
		BlendData::One, BlendData::InverseSourceAlpha, BlendOperation::Add,
		LogicOp::NoOperation,
		(byte)ColorWriteMask::All
	};

	const DepthDesc DepthDesc::DepthDisabled =
	{
		false, // enableDepth
		DepthWriteMask::All, // depthWriteMask
		ComparisonFunc::Less, // depthFunc
		false, // enableStencil
		0xff, // stencilReadMask
		0xff, // stencilWriteMask
		StencilOp::Keep,
		StencilOp::Increase,
		StencilOp::Keep,
		ComparisonFunc::Always,
		StencilOp::Keep,
		StencilOp::Decrease,
		StencilOp::Keep,
		ComparisonFunc::Always
	};
	const DepthDesc DepthDesc::DepthEnabled =
	{
		true, // enableDepth
		DepthWriteMask::All, // depthWriteMask
		ComparisonFunc::Less, // depthFunc
		false, // enableStencil
		0xff, // stencilReadMask
		0xff, // stencilWriteMask
		StencilOp::Keep,
		StencilOp::Increase,
		StencilOp::Keep,
		ComparisonFunc::Always,
		StencilOp::Keep,
		StencilOp::Decrease,
		StencilOp::Keep,
		ComparisonFunc::Always
	};
	const DepthDesc DepthDesc::DepthAndStencilEnabled =
	{
		true, // enableDepth
		DepthWriteMask::All, // depthWriteMask
		ComparisonFunc::Less, // depthFunc
		true, // enableStencil
		0xff, // stencilReadMask
		0xff, // stencilWriteMask
		StencilOp::Keep,
		StencilOp::Increase,
		StencilOp::Keep,
		ComparisonFunc::Always,
		StencilOp::Keep,
		StencilOp::Decrease,
		StencilOp::Keep,
		ComparisonFunc::Always
	};

	const RasterizerDesc RasterizerDesc::NoCull =
	{
		false, Cull::Disabled, FrontFace::CW, 0, 0.0f, 0.0f, true, LineRasterizationMode::Smooth, 0, false
	};
	const RasterizerDesc RasterizerDesc::BackCull =
	{
		false, Cull::Back, FrontFace::CW, 0, 0.0f, 0.0f, true, LineRasterizationMode::Smooth, 0, false
	};
	const RasterizerDesc RasterizerDesc::FrontCull =
	{
		false, Cull::Front, FrontFace::CW, 0, 0.0f, 0.0f, true, LineRasterizationMode::Smooth, 0, false
	};

	const SamplerDesc SamplerDesc::PixelatedRepeatSampler =
	{
		TextureFilter::Point, TextureWrapMode::Repeat, TextureWrapMode::Repeat, TextureWrapMode::Repeat,
		0, // minLOD
		IGLO_FLOAT32_MAX, // maxLOD
		0, // mipMapLODBias
		ComparisonFunc::None,
		Colors::Black // borderColor
	};
	const SamplerDesc SamplerDesc::SmoothRepeatSampler =
	{
		TextureFilter::AnisotropicX16, TextureWrapMode::Repeat, TextureWrapMode::Repeat, TextureWrapMode::Repeat,
		0, // minLOD
		IGLO_FLOAT32_MAX, // maxLOD
		0, // mipMapLODBias
		ComparisonFunc::None,
		Colors::Black // borderColor
	};
	const SamplerDesc SamplerDesc::SmoothClampSampler =
	{
		TextureFilter::AnisotropicX16, TextureWrapMode::Clamp, TextureWrapMode::Clamp, TextureWrapMode::Clamp,
		0, // minLOD
		IGLO_FLOAT32_MAX, // maxLOD
		0, // mipMapLODBias
		ComparisonFunc::None,
		Colors::Black // borderColor
	};


#ifdef _WIN32
	unsigned int IGLOContext::numWindows = 0;
	bool IGLOContext::windowClassIsRegistered = false;
#endif

	static CallbackLog logCallback = nullptr;

	void SetLogCallback(CallbackLog logFunc)
	{
		logCallback = logFunc;
	}
	void Log(LogType type, const std::string& message)
	{
		if (logCallback)
		{
			logCallback(type, message);
		}
		else
		{
			std::string typeStr = "IGLO ";

			if (type == LogType::Info) typeStr += "Info: ";
			else if (type == LogType::Warning) typeStr += "Warning: ";
			else if (type == LogType::Error) typeStr += "Error: ";
			else if (type == LogType::FatalError) typeStr += "Fatal error: ";
			else typeStr += ": ";

			Print(typeStr + message + "\n");
		}
	}

	void PopupMessage(const std::string& message, const std::string& caption, const IGLOContext* parent)
	{
#ifdef _WIN32
		HWND hwnd = 0;
		if (parent) hwnd = parent->GetWindowHWND();
		MessageBoxW(hwnd, utf8_to_utf16(message).c_str(), utf8_to_utf16(caption).c_str(), 0);
#endif
#ifdef __linux__
		const std::string q = "\"";
		system(std::string("xmessage -center " + q + caption + "\n" + "\n" + message + q).c_str());
#endif
	}

#ifdef IGLO_D3D12
	TextureFilterD3D12 ConvertTextureFilterToD3D12(TextureFilter filter)
	{
		TextureFilterD3D12 out;
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
		case Format::UINT2_UINT10_UINT10_UINT10:		dxgi = DXGI_FORMAT_R10G10B10A2_UNORM;	typeless = DXGI_FORMAT_R10G10B10A2_TYPELESS; break;
		case Format::UINT2_UINT10_UINT10_UINT10_NotNormalized: dxgi = DXGI_FORMAT_R10G10B10A2_UINT; typeless = DXGI_FORMAT_R10G10B10A2_TYPELESS; break;
		case Format::UFLOAT10_UFLOAT11_UFLOAT11:		dxgi = DXGI_FORMAT_R11G11B10_FLOAT;		typeless = DXGI_FORMAT_R10G10B10A2_TYPELESS; break;
		case Format::UFLOAT5_UFLOAT9_UFLOAT9_UFLOAT9:	dxgi = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;	typeless = DXGI_FORMAT_UNKNOWN; break;
		case Format::BYTE_BYTE_BYTE_BYTE_sRGB:			dxgi = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	typeless = DXGI_FORMAT_R8G8B8A8_TYPELESS; break;
		case Format::BYTE_BYTE_BYTE_BYTE_BGRA_sRGB:		dxgi = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;	typeless = DXGI_FORMAT_B8G8R8A8_TYPELESS; break;
		case Format::BYTE_BYTE_BYTE_BYTE_BGRA:			dxgi = DXGI_FORMAT_B8G8R8A8_UNORM;		typeless = DXGI_FORMAT_B8G8R8A8_TYPELESS; break;
		case Format::BYTE_BYTE_BYTE_BYTE_NotNormalized: dxgi = DXGI_FORMAT_R8G8B8A8_UINT;		typeless = DXGI_FORMAT_R8G8B8A8_TYPELESS; break;
		case Format::BYTE_BYTE_NotNormalized:			dxgi = DXGI_FORMAT_R8G8_UINT;			typeless = DXGI_FORMAT_R8G8_TYPELESS; break;
		case Format::BYTE_NotNormalized:				dxgi = DXGI_FORMAT_R8_UINT;				typeless = DXGI_FORMAT_R8_TYPELESS; break;
		case Format::BC1:						dxgi = DXGI_FORMAT_BC1_UNORM;		typeless = DXGI_FORMAT_BC1_TYPELESS; break;
		case Format::BC1_sRGB:					dxgi = DXGI_FORMAT_BC1_UNORM_SRGB;	typeless = DXGI_FORMAT_BC1_TYPELESS; break;
		case Format::BC2:						dxgi = DXGI_FORMAT_BC2_UNORM;		typeless = DXGI_FORMAT_BC2_TYPELESS; break;
		case Format::BC2_sRGB:					dxgi = DXGI_FORMAT_BC2_UNORM_SRGB;	typeless = DXGI_FORMAT_BC2_TYPELESS; break;
		case Format::BC3:						dxgi = DXGI_FORMAT_BC3_UNORM;		typeless = DXGI_FORMAT_BC3_TYPELESS; break;
		case Format::BC3_sRGB:					dxgi = DXGI_FORMAT_BC3_UNORM_SRGB;	typeless = DXGI_FORMAT_BC3_TYPELESS; break;
		case Format::BC4_UNSIGNED:				dxgi = DXGI_FORMAT_BC4_UNORM;		typeless = DXGI_FORMAT_BC4_TYPELESS; break;
		case Format::BC4_SIGNED:				dxgi = DXGI_FORMAT_BC4_SNORM;		typeless = DXGI_FORMAT_BC4_TYPELESS; break;
		case Format::BC5_UNSIGNED:				dxgi = DXGI_FORMAT_BC5_UNORM;		typeless = DXGI_FORMAT_BC5_TYPELESS; break;
		case Format::BC5_SIGNED:				dxgi = DXGI_FORMAT_BC5_SNORM;		typeless = DXGI_FORMAT_BC5_TYPELESS; break;
		case Format::BC6H_UNSIGNED_FLOAT16:		dxgi = DXGI_FORMAT_BC6H_UF16;		typeless = DXGI_FORMAT_BC6H_TYPELESS; break;
		case Format::BC6H_SIGNED_FLOAT16:		dxgi = DXGI_FORMAT_BC6H_SF16;		typeless = DXGI_FORMAT_BC6H_TYPELESS; break;
		case Format::BC7:						dxgi = DXGI_FORMAT_BC7_UNORM;		typeless = DXGI_FORMAT_BC7_TYPELESS; break;
		case Format::BC7_sRGB:					dxgi = DXGI_FORMAT_BC7_UNORM_SRGB;	typeless = DXGI_FORMAT_BC7_TYPELESS; break;
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

		}
		FormatInfoDXGI out = FormatInfoDXGI();
		out.dxgiFormat = dxgi;
		out.typeless = typeless;
		return out;
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> ConvertVertexElementsToD3D12(const std::vector<VertexElement>& elems)
	{
		UINT append = D3D12_APPEND_ALIGNED_ELEMENT;
		std::vector<D3D12_INPUT_ELEMENT_DESC> converted;
		converted.clear();
		converted.resize(elems.size());
		for (size_t i = 0; i < elems.size(); i++)
		{
			FormatInfoDXGI formatInfoDXGI = GetFormatInfoDXGI(elems[i].format);
			DXGI_FORMAT format = formatInfoDXGI.dxgiFormat;
			D3D12_INPUT_CLASSIFICATION inputClass;
			if (elems[i].inputSlotClass == InputClass::PerInstance)
			{
				inputClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
			}
			else
			{
				inputClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			}
			UINT align = append;
			if (i == 0) align = 0;
			converted[i] = { elems[i].semanticName, elems[i].semanticIndex, format, elems[i].inputSlot, align, inputClass, elems[i].instanceDataStepRate };
		}
		return converted;
	}

#endif

	FormatInfo GetFormatInfo(Format format)
	{
		uint32_t size = 0; // pixel size in bytes.
		uint32_t block = 0; // For block-compressed formats.
		uint32_t elem = 0; // Amount of elements. (Color channels).
		bool isDepth = false;
		Format sRGB_anti = Format::None;
		bool sRGB = false;

		switch (format)
		{
		case Format::BYTE:							elem = 1; size = 1;																	 break;
		case Format::BYTE_BYTE:						elem = 2; size = 2;																	 break;
		case Format::BYTE_BYTE_BYTE_BYTE:			elem = 4; size = 4;  sRGB_anti = Format::BYTE_BYTE_BYTE_BYTE_sRGB;					 break;
		case Format::BYTE_BYTE_BYTE_BYTE_sRGB:		elem = 4; size = 4;	 sRGB_anti = Format::BYTE_BYTE_BYTE_BYTE;			sRGB = true; break;
		case Format::BYTE_BYTE_BYTE_BYTE_BGRA:		elem = 4; size = 4;	 sRGB_anti = Format::BYTE_BYTE_BYTE_BYTE_BGRA_sRGB;				 break;
		case Format::BYTE_BYTE_BYTE_BYTE_BGRA_sRGB: elem = 4; size = 4;	 sRGB_anti = Format::BYTE_BYTE_BYTE_BYTE_BGRA;		sRGB = true; break;

		case Format::BYTE_NotNormalized:			elem = 1; size = 1; break;
		case Format::INT8:							elem = 1; size = 1; break;
		case Format::INT8_NotNormalized:			elem = 1; size = 1; break;
		case Format::UINT16:						elem = 1; size = 2; break;
		case Format::UINT16_NotNormalized:			elem = 1; size = 2; break;
		case Format::INT16:							elem = 1; size = 2; break;
		case Format::INT16_NotNormalized:			elem = 1; size = 2; break;
		case Format::FLOAT16:						elem = 1; size = 2; break;
		case Format::INT32_NotNormalized:			elem = 1; size = 4; break;
		case Format::UINT32_NotNormalized:			elem = 1; size = 4; break;
		case Format::FLOAT:							elem = 1; size = 4; break;

		case Format::BYTE_BYTE_NotNormalized:		elem = 2; size = 2;  break;
		case Format::INT8_INT8:						elem = 2; size = 2;  break;
		case Format::INT8_INT8_NotNormalized:		elem = 2; size = 2;  break;
		case Format::UINT16_UINT16:					elem = 2; size = 4;  break;
		case Format::UINT16_UINT16_NotNormalized:	elem = 2; size = 4;  break;
		case Format::INT16_INT16:					elem = 2; size = 4;  break;
		case Format::INT16_INT16_NotNormalized:		elem = 2; size = 4;	 break;
		case Format::FLOAT16_FLOAT16:				elem = 2; size = 4;  break;
		case Format::UINT32_UINT32_NotNormalized:	elem = 2; size = 8;  break;
		case Format::INT32_INT32_NotNormalized:		elem = 2; size = 8;	 break;
		case Format::FLOAT_FLOAT:					elem = 2; size = 8;  break;

		case Format::UFLOAT10_UFLOAT11_UFLOAT11:			elem = 3; size = 4;  break;
		case Format::UINT32_UINT32_UINT32_NotNormalized:	elem = 3; size = 12; break;
		case Format::INT32_INT32_INT32_NotNormalized:		elem = 3; size = 12; break;
		case Format::FLOAT_FLOAT_FLOAT:						elem = 3; size = 12; break;

		case Format::BYTE_BYTE_BYTE_BYTE_NotNormalized:			elem = 4; size = 4;  break;
		case Format::INT8_INT8_INT8_INT8:						elem = 4; size = 4;  break;
		case Format::INT8_INT8_INT8_INT8_NotNormalized:			elem = 4; size = 4;  break;
		case Format::UINT2_UINT10_UINT10_UINT10:				elem = 4; size = 4;	 break;
		case Format::UINT2_UINT10_UINT10_UINT10_NotNormalized:	elem = 4; size = 4;	 break;
		case Format::UFLOAT5_UFLOAT9_UFLOAT9_UFLOAT9:			elem = 4; size = 4;	 break;
		case Format::UINT16_UINT16_UINT16_UINT16:				elem = 4; size = 8;  break;
		case Format::UINT16_UINT16_UINT16_UINT16_NotNormalized:	elem = 4; size = 8;  break;
		case Format::INT16_INT16_INT16_INT16:					elem = 4; size = 8;  break;
		case Format::INT16_INT16_INT16_INT16_NotNormalized:		elem = 4; size = 8;  break;
		case Format::FLOAT16_FLOAT16_FLOAT16_FLOAT16:			elem = 4; size = 8;  break;
		case Format::UINT32_UINT32_UINT32_UINT32_NotNormalized:	elem = 4; size = 16; break;
		case Format::INT32_INT32_INT32_INT32_NotNormalized:		elem = 4; size = 16; break;
		case Format::FLOAT_FLOAT_FLOAT_FLOAT:					elem = 4; size = 16; break;

		case Format::BC1:					block = 8;	elem = 4; sRGB_anti = Format::BC1_sRGB;				 break;
		case Format::BC1_sRGB:				block = 8;	elem = 4; sRGB_anti = Format::BC1;		sRGB = true; break;
		case Format::BC2:					block = 16;	elem = 4; sRGB_anti = Format::BC2_sRGB;				 break;
		case Format::BC2_sRGB:				block = 16;	elem = 4; sRGB_anti = Format::BC2;		sRGB = true; break;
		case Format::BC3:					block = 16;	elem = 4; sRGB_anti = Format::BC3_sRGB;				 break;
		case Format::BC3_sRGB:				block = 16;	elem = 4; sRGB_anti = Format::BC3;		sRGB = true; break;
		case Format::BC4_UNSIGNED:
		case Format::BC4_SIGNED:			block = 8;	elem = 1; break;
		case Format::BC5_UNSIGNED:
		case Format::BC5_SIGNED:			block = 16; elem = 2; break;
		case Format::BC6H_UNSIGNED_FLOAT16:
		case Format::BC6H_SIGNED_FLOAT16:	block = 16; elem = 4; break;
		case Format::BC7:					block = 16;	elem = 4; sRGB_anti = Format::BC7_sRGB;				 break;
		case Format::BC7_sRGB:				block = 16;	elem = 4; sRGB_anti = Format::BC7;		sRGB = true; break;

		case Format::DEPTHFORMAT_FLOAT:				elem = 1; size = 4; isDepth = true; break;
		case Format::DEPTHFORMAT_UINT16:			elem = 1; size = 2; isDepth = true; break;
		case Format::DEPTHFORMAT_UINT24_BYTE:		elem = 2; size = 4; isDepth = true; break;

			// NOTE: D3D12's GetCopyableFootprints() seems to think that this format is 4 bytes per pixel, not 5 or 8.
			// For code simplicity, size is set to 4 to match the size returned from GetCopyableFootprints().
		case Format::DEPTHFORMAT_FLOAT_BYTE:	elem = 2; size = 4; isDepth = true; break;

		}

		FormatInfo out = FormatInfo();
		out.blockSize = block;
		out.bytesPerPixel = size;
		out.elementCount = elem;
		out.sRGB_opposite = sRGB_anti;
		out.isDepthFormat = isDepth;
		out.is_sRGB = sRGB;
		return out;
	}


	Extent2D IGLOContext::GetActiveMonitorScreenResolution()
	{
		Extent2D out = {};
#ifdef _WIN32
		HMONITOR active_monitor;
		if (windowHWND)
		{
			active_monitor = MonitorFromWindow(windowHWND, MONITOR_DEFAULTTONEAREST);
		}
		else
		{
			active_monitor = MonitorFromWindow(0, MONITOR_DEFAULTTOPRIMARY);
		}
		MONITORINFO minfo;
		minfo.cbSize = sizeof(minfo);
		if (GetMonitorInfo(active_monitor, &minfo))
		{
			out.width = unsigned int(abs(minfo.rcMonitor.right - minfo.rcMonitor.left));
			out.height = unsigned int(abs(minfo.rcMonitor.bottom - minfo.rcMonitor.top));
		}
		else
		{
			Log(LogType::Error, "Failed to get active screen resolution.");
		}
#endif
#ifdef __linux__
		if (screenHandle)
		{
			out.width = screenHandle->width;
			out.height = screenHandle->height;
		}
#endif
		return out;
	}

	std::vector<Extent2D> IGLOContext::GetAvailableScreenResolutions()
	{
#ifdef _WIN32
		DEVMODEW devmode = {};
		std::vector<Extent2D> resolutions;

		for (DWORD i = 0; EnumDisplaySettingsW(nullptr, i, &devmode); i++)
		{
			Extent2D res = { devmode.dmPelsWidth, devmode.dmPelsHeight };
			if (std::find(resolutions.begin(), resolutions.end(), res) == resolutions.end())
			{
				resolutions.push_back(res);
			}
		}
		return resolutions;
#endif
#ifdef __linux__
		//TODO: this
		std::vector<Extent2D> resolutions;
		return resolutions;
#endif
	}

#ifdef _WIN32

	void IGLOContext::CaptureMouse(bool captured)
	{
		if (!this->windowHWND) return;
		if (captured) SetCapture(this->windowHWND);
		else ReleaseCapture();
	}
#endif

#ifdef _WIN32
	void IGLOContext::UpdateWin32Window(IntPoint pos, Extent2D clientSize, bool resizable, bool bordersVisible, bool visible,
		bool topMost, bool gainFocus)
	{
		if (!windowHWND) return;
		UINT style;
		if (bordersVisible)
		{
			if (resizable) style = windowStyleWindowedResizable;
			else style = windowStyleWindowed;
		}
		else
		{
			style = windowStyleBorderless;
		}
		if (visible) style |= WS_VISIBLE;
		UINT resizableFlag = resizable ? WS_THICKFRAME | WS_MAXIMIZEBOX : 0;

		// Calling SetWindowLongPtr can sometimes send a WM_SIZE event with incorrect size.
		// We want to ignore these WM_SIZE messages.
		ignoreSizeMsg = true;
		SetWindowLongPtr(windowHWND, GWL_STYLE, style);
		ignoreSizeMsg = false;

		RECT rc;
		SetRect(&rc, 0, 0, (int)clientSize.width, (int)clientSize.height);
		AdjustWindowRect(&rc, style, false);
		HWND insertAfter = HWND_NOTOPMOST;
		if (topMost) insertAfter = HWND_TOPMOST;
		UINT flags = SWP_FRAMECHANGED;
		if (!gainFocus) flags |= SWP_NOACTIVATE;
		SetWindowPos(windowHWND, insertAfter, pos.x, pos.y, rc.right - rc.left, rc.bottom - rc.top, flags);
	}
#endif

	void IGLOContext::SetWindowIconFromImage(const Image& icon)
	{
#ifdef _WIN32
		if (!this->windowHWND) return;
		FormatInfo info = GetFormatInfo(icon.GetFormat());
		uint32_t bitsPerPixel = info.bytesPerPixel * 8;
		uint32_t numChannels = info.elementCount;
		if (!(bitsPerPixel == 32 && numChannels == 4 && icon.IsLoaded()))
		{
			Log(LogType::Error, "Failed to set window icon. Reason: Icon image must be 4 colors per channel 32 bits per pixel.");
			return;
		}
		std::vector<Color32> pixelsSmall;
		byte* pixelPtrSmall = nullptr;
		if (icon.GetFormat() == Format::BYTE_BYTE_BYTE_BYTE_BGRA ||
			icon.GetFormat() == Format::BYTE_BYTE_BYTE_BYTE_BGRA_sRGB)
		{
			// We can use the image pixel data directly if the format is already BGRA.
			pixelPtrSmall = (byte*)icon.GetPixels();
		}
		else
		{
			// Windows expects BGRA format. Swap blue and red channels to convert RGBA to BGRA.
			pixelsSmall.resize(icon.GetSize() / 4);
			for (size_t i = 0; i < icon.GetSize() / 4; i++)
			{
				Color32* src = (Color32*)icon.GetPixels();
				pixelsSmall[i].red = src[i].blue;
				pixelsSmall[i].green = src[i].green;
				pixelsSmall[i].blue = src[i].red;
				pixelsSmall[i].alpha = src[i].alpha;
			}
			pixelPtrSmall = (byte*)pixelsSmall.data();
		}

		HINSTANCE hInstance = (HINSTANCE)GetModuleHandleW(0);
		if (this->windowIconOwned)
		{
			DestroyIcon(this->windowIconOwned);
			this->windowIconOwned = 0;
		}
		this->windowIconOwned = CreateIcon(hInstance, (int)icon.GetWidth(), (int)icon.GetHeight(), 1, 32, nullptr, pixelPtrSmall);
		SendMessage(this->windowHWND, WM_SETICON, ICON_SMALL, (LPARAM)this->windowIconOwned);
		SendMessage(this->windowHWND, WM_SETICON, ICON_BIG, (LPARAM)this->windowIconOwned);
#endif
#ifdef __linux__
		//TODO: this
#endif
	}

#ifdef _WIN32
	void IGLOContext::SetWindowIconFromResource(int icon)
	{
		if (!this->windowHWND) return;
		// If window used an owned icon, destroy that icon
		if (this->windowIconOwned)
		{
			DestroyIcon(this->windowIconOwned);
			this->windowIconOwned = 0;
		}
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(icon));
		SendMessage(this->windowHWND, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(this->windowHWND, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}

	void IGLOContext::SetWndProcHookCallback(CallbackWndProcHook callback)
	{
		callbackWndProcHook = callback;
	}

#endif

	void IGLOContext::CenterWindow()
	{
		Extent2D res = GetActiveMonitorScreenResolution();
#ifdef _WIN32
		RECT rect = { NULL };
		if (GetWindowRect(windowHWND, &rect))
		{
			int32_t totalWindowWidth = rect.right - rect.left;
			int32_t totalWindowHeight = rect.bottom - rect.top;
			int32_t x = (int32_t(res.width) / 2) - (totalWindowWidth / 2);
			int32_t y = (int32_t(res.height) / 2) - (totalWindowHeight / 2);
			SetWindowPosition(x, y);
		}
#endif
#ifdef __linux__
		//TODO: this
#endif
	}

	void IGLOContext::EnableDragAndDrop(bool enable)
	{
		windowEnableDragAndDrop = enable;
#ifdef _WIN32
		if (!windowHWND) return;
		DragAcceptFiles(windowHWND, enable);
#endif
#ifdef __linux__
		//TODO: this
#endif
	}

	bool IGLOContext::IsMouseButtonDown(MouseButton button) const
	{
		if (!isLoaded) return false;
		if (button == MouseButton::None) return false;
		return mouseButtonIsDown.GetAt((uint64_t)button);
	}

	bool IGLOContext::IsKeyDown(Key key) const
	{
		if (!isLoaded) return false;
		if (key == Key::Unknown) return false;
		return keyIsDown.GetAt((uint64_t)key);
	}

	void IGLOContext::SetWindowVisible(bool visible)
	{
		if (this->windowedVisible == visible) return;
		this->windowedVisible = visible;

		if (this->displayMode == DisplayMode::Windowed)
		{
#ifdef _WIN32
			if (!windowHWND) return;
			if (visible) ShowWindow(windowHWND, SW_SHOW);
			else ShowWindow(windowHWND, SW_HIDE);
#endif
#ifdef __linux__
			if (!displayHandle) return;
			if (!windowHandle) return;
			if (visible) XMapWindow(displayHandle, windowHandle);
			else XUnmapWindow(displayHandle, windowHandle);
#endif
		}
	}

	void IGLOContext::SetWindowTitle(std::string title)
	{
		this->windowTitle = title;
#ifdef _WIN32
		if (!this->windowHWND) return;
		SetWindowTextW(this->windowHWND, utf8_to_utf16(title).c_str());
#endif
#ifdef __linux__
		//TODO: this
#endif
	}

	void IGLOContext::SetWindowSize(uint32_t width, uint32_t height)
	{
		uint32_t cappedWidth = width;
		uint32_t cappedHeight = height;

		if (cappedWidth < this->windowedMinSize.width && this->windowedMinSize.width != 0) cappedWidth = this->windowedMinSize.width;
		if (cappedHeight < this->windowedMinSize.height && this->windowedMinSize.height != 0) cappedHeight = this->windowedMinSize.height;
		if (cappedWidth > this->windowedMaxSize.width && this->windowedMaxSize.width != 0) cappedWidth = this->windowedMaxSize.width;
		if (cappedHeight > this->windowedMaxSize.height && this->windowedMaxSize.height != 0) cappedHeight = this->windowedMaxSize.height;

		if (this->windowedSize == Extent2D(cappedWidth, cappedHeight)) return;
		this->windowedSize = Extent2D(cappedWidth, cappedHeight);

		if (this->displayMode == DisplayMode::Windowed)
		{
#ifdef _WIN32
			UpdateWin32Window(this->windowedPos, this->windowedSize, this->windowedResizable,
				this->windowedBordersVisible, this->windowedVisible, false, false);
#endif
#ifdef __linux__
			//TODO: this
#endif
		}
	}

	void IGLOContext::SetWindowPosition(int32_t x, int32_t y)
	{
		if (this->windowedPos == IntPoint(x, y)) return;
		this->windowedPos = IntPoint(x, y);

		if (this->displayMode == DisplayMode::Windowed)
		{
#ifdef _WIN32
			if (!this->windowHWND) return;
			SetWindowPos(this->windowHWND, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
#ifdef __linux__
			//TODO: this
#endif
		}
	}

	void IGLOContext::SetWindowResizable(bool resizable)
	{
		if (this->windowedResizable == resizable) return;
		this->windowedResizable = resizable;

		if (this->displayMode == DisplayMode::Windowed)
		{
#ifdef _WIN32
			UpdateWin32Window(this->windowedPos, this->windowedSize, this->windowedResizable,
				this->windowedBordersVisible, this->windowedVisible, false, false);
#endif
#ifdef __linux__
			//TODO: this
#endif
		}
	}

	void IGLOContext::SetWindowBordersVisible(bool visible)
	{
		if (this->windowedBordersVisible == visible) return;
		this->windowedBordersVisible = visible;

		if (this->displayMode == DisplayMode::Windowed)
		{
#ifdef _WIN32
			UpdateWin32Window(this->windowedPos, this->windowedSize, this->windowedResizable,
				this->windowedBordersVisible, this->windowedVisible, false, false);
#endif
#ifdef __linux__
			//TODO: this
#endif
		}
	}

	void IGLOContext::ToggleFullscreen()
	{
		if (displayMode == DisplayMode::Windowed)
		{
			EnterFullscreenMode(backBufferFormat);
		}
		else
		{
			EnterWindowedMode(windowedSize.width, windowedSize.height, backBufferFormat);
		}
	}

	void IGLOContext::EnterWindowedMode(uint32_t width, uint32_t height, Format format)
	{
		displayMode = DisplayMode::Windowed;
		windowedSize = Extent2D(width, height);

#ifdef _WIN32
		// If the window is in a minimized state, you can't resize it.
		// Restoring a minimized window with ShowWindow() sends a WM_SIZE event.
		// The WM_SIZE event will trigger a call to ResizeBackBuffer if the size
		// is different from last ResizeBackBuffer call.
		// To prevent ResizeBackBuffer from being called more than once we call it
		// in between ShowWindow() and UpdateWin32Window().
		ShowWindow(windowHWND, SW_RESTORE);
		ShowWindow(windowHWND, windowedVisible ? SW_SHOW : SW_HIDE);
		SetForegroundWindow(windowHWND); // Set keyboard focus to this window
		ResizeBackBuffer(Extent2D(width, height), format);
		UpdateWin32Window(windowedPos, windowedSize, windowedResizable, windowedBordersVisible, windowedVisible, false, true);
#endif
#ifdef __linux__
		//TODO: this
#endif
	}

	void IGLOContext::EnterFullscreenMode(Format format, uint32_t monitor)
	{
		//TODO: Add support for choosing what monitor to enter fullscreen in. At the moment 'monitor' argument does nothing.

		displayMode = DisplayMode::BorderlessFullscreen;

		Extent2D res = GetActiveMonitorScreenResolution();

		// If getting desktop resolution failed, default to the most common resolution
		if (res.width == 0 || res.height == 0)
		{
			res.width = 1920;
			res.height = 1080;
		}

#ifdef _WIN32
		ShowWindow(windowHWND, SW_RESTORE);
		SetForegroundWindow(windowHWND); // Set keyboard focus to this window
		ResizeBackBuffer(res, format);
		UpdateWin32Window(IntPoint(0, 0), res, false, false, true, true, true);
#endif
#ifdef __linux__
		//TODO: this
#endif
	}

	void IGLOContext::SetPresentMode(PresentMode mode)
	{
		presentMode = mode;
	}

	void Pipeline::Unload()
	{
#ifdef IGLO_D3D12
		d3d12PipelineState.Reset();
#endif
		isLoaded = false;
		context = nullptr;
	}

	bool Pipeline::Load(const IGLOContext& context, const PipelineDesc& desc)
	{
		Unload();

		const char* errStr = "Failed to create graphics pipeline state object. Reason: ";

		if (desc.blendStates.size() > IGLO_MAX_SIMULTANEOUS_RENDER_TARGETS)
		{
			Log(LogType::Error, ToString(errStr, "Too many blend states provided."));
			return false;
		}

		if (desc.renderTargetDesc.colorFormats.size() > IGLO_MAX_SIMULTANEOUS_RENDER_TARGETS)
		{
			Log(LogType::Error, ToString(errStr, "Too many color formats provided in the render target description."));
			return false;
		}

#ifdef IGLO_D3D12
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipe = {};

		// Root signature
		pipe.pRootSignature = context.GetD3D12BindlessRootSignature();

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
			pipe.BlendState.RenderTarget[i].BlendEnable = desc.blendStates[i].enabled;
			pipe.BlendState.RenderTarget[i].SrcBlend = (D3D12_BLEND)desc.blendStates[i].srcBlend;
			pipe.BlendState.RenderTarget[i].DestBlend = (D3D12_BLEND)desc.blendStates[i].destBlend;
			pipe.BlendState.RenderTarget[i].BlendOp = (D3D12_BLEND_OP)desc.blendStates[i].blendOp;
			pipe.BlendState.RenderTarget[i].SrcBlendAlpha = (D3D12_BLEND)desc.blendStates[i].srcBlendAlpha;
			pipe.BlendState.RenderTarget[i].DestBlendAlpha = (D3D12_BLEND)desc.blendStates[i].destBlendAlpha;
			pipe.BlendState.RenderTarget[i].BlendOpAlpha = (D3D12_BLEND_OP)desc.blendStates[i].blendOpAlpha;
			pipe.BlendState.RenderTarget[i].RenderTargetWriteMask = (UINT8)desc.blendStates[i].colorWriteMask;
			pipe.BlendState.RenderTarget[i].LogicOpEnable = desc.blendStates[i].logicOpEnabled;
			pipe.BlendState.RenderTarget[i].LogicOp = (D3D12_LOGIC_OP)desc.blendStates[i].logicOp;
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
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = ConvertVertexElementsToD3D12(desc.vertexLayout);
		pipe.InputLayout.NumElements = (UINT)inputLayout.size();
		pipe.InputLayout.pInputElementDescs = inputLayout.data();

		// Sample mask
		pipe.SampleMask = desc.sampleMask;

		// Primitive topology type
		switch (desc.primitiveTopology)
		{
		case Primitive::PointList:
			pipe.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			break;

		case Primitive::LineList:
		case Primitive::LineStrip:
		case Primitive::_LineList_Adj:
		case Primitive::_LineStrip_Adj:
			pipe.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			break;

		case Primitive::TriangleList:
		case Primitive::TriangleStrip:
		case Primitive::_TriangleList_Adj:
		case Primitive::_TriangleStrip_Adj:
			pipe.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			break;

		case Primitive::__1_ControlPointPatchList:
		case Primitive::__2_ControlPointPatchList:
		case Primitive::__3_ControlPointPatchList:
		case Primitive::__4_ControlPointPatchList:
		case Primitive::__5_ControlPointPatchList:
		case Primitive::__6_ControlPointPatchList:
		case Primitive::__7_ControlPointPatchList:
		case Primitive::__8_ControlPointPatchList:
		case Primitive::__9_ControlPointPatchList:
		case Primitive::__10_ControlPointPatchList:
		case Primitive::__11_ControlPointPatchList:
		case Primitive::__12_ControlPointPatchList:
		case Primitive::__13_ControlPointPatchList:
		case Primitive::__14_ControlPointPatchList:
		case Primitive::__15_ControlPointPatchList:
		case Primitive::__16_ControlPointPatchList:
		case Primitive::__17_ControlPointPatchList:
		case Primitive::__18_ControlPointPatchList:
		case Primitive::__19_ControlPointPatchList:
		case Primitive::__20_ControlPointPatchList:
		case Primitive::__21_ControlPointPatchList:
		case Primitive::__22_ControlPointPatchList:
		case Primitive::__23_ControlPointPatchList:
		case Primitive::__24_ControlPointPatchList:
		case Primitive::__25_ControlPointPatchList:
		case Primitive::__26_ControlPointPatchList:
		case Primitive::__27_ControlPointPatchList:
		case Primitive::__28_ControlPointPatchList:
		case Primitive::__29_ControlPointPatchList:
		case Primitive::__30_ControlPointPatchList:
		case Primitive::__31_ControlPointPatchList:
		case Primitive::__32_ControlPointPatchList:
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

		HRESULT hr = context.GetD3D12Device()->CreateGraphicsPipelineState(&pipe, IID_PPV_ARGS(&this->d3d12PipelineState));
		if (FAILED(hr))
		{
			Log(LogType::Error, ToString(errStr, "ID3D12Device::CreateGraphicsPipelineState returned error code: ", (uint32_t)hr, "."));
			Unload();
			return false;
		}

		this->isLoaded = true;
		this->context = &context;
		return true;
#endif
#ifdef IGLO_VULKAN
		return false; //TODO: this
#endif
	}

	bool Pipeline::LoadFromFile(const IGLOContext& context, const std::string& filepathVS, const std::string filepathPS,
		const std::string& entryPointNameVS, const std::string& entryPointNamePS,
		const std::vector<VertexElement>& vertexLayout, Primitive primitive, DepthDesc depth,
		RasterizerDesc rasterizer, const std::vector<BlendDesc>& blend, RenderTargetDesc renderTargetDesc)
	{
		Unload();

		const char* errStr = "Failed to create graphics pipeline state object. Reason: ";

		if (filepathVS.empty() || filepathPS.empty())
		{
			Log(LogType::Error, ToString(errStr, "Couldn't read shader bytecode from file because empty filepath was provided."));
			return false;
		}

		// Vertex shader
		ReadFileResult VS = ReadFile(filepathVS);
		if (!VS.success)
		{
			Log(LogType::Error, ToString(errStr, "Failed to read shader bytecode from file '", filepathVS, "'."));
			return false;
		}

		// Pixel shader
		ReadFileResult PS = ReadFile(filepathPS);
		if (!PS.success)
		{
			Log(LogType::Error, ToString(errStr, "Failed to read shader bytecode from file '", filepathPS, "'."));
			return false;
		}

		return Load(context,
			Shader(VS.fileContent, entryPointNameVS),
			Shader(PS.fileContent, entryPointNamePS),
			vertexLayout, primitive, depth, rasterizer, blend, renderTargetDesc);
	}

	bool Pipeline::Load(const IGLOContext& context, Shader VS, Shader PS, const std::vector<VertexElement>& vertexLayout,
		Primitive primitive, DepthDesc depth, RasterizerDesc rasterizer, const std::vector<BlendDesc>& blend,
		RenderTargetDesc renderTargetDesc)
	{
		PipelineDesc desc;
		desc.VS = VS;
		desc.PS = PS;
		desc.vertexLayout = vertexLayout;
		desc.primitiveTopology = primitive;
		desc.depthState = depth;
		desc.rasterizerState = rasterizer;
		desc.blendStates = blend;
		desc.renderTargetDesc = renderTargetDesc;
		return Load(context, desc);
	}

	bool Pipeline::LoadAsCompute(const IGLOContext& context, Shader CS)
	{
		Unload();

#ifdef IGLO_D3D12
		D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc = {};
		computeDesc.pRootSignature = context.GetD3D12BindlessRootSignature();
		computeDesc.CS.BytecodeLength = CS.bytecodeLength;
		computeDesc.CS.pShaderBytecode = (const void*)CS.shaderBytecode;
		HRESULT hr = context.GetD3D12Device()->CreateComputePipelineState(&computeDesc, IID_PPV_ARGS(&this->d3d12PipelineState));
		if (FAILED(hr))
		{
			Log(LogType::Error, ToString("Failed to create compute pipeline state object."
				" Reason: ID3D12Device::CreateComputePipelineState returned error code: ", (uint32_t)hr, "."));
			Unload();
			return false;
		}

		this->isLoaded = true;
		this->context = &context;
		return true;
#endif
#ifdef IGLO_VULKAN
		return false; //TODO: this
#endif
	}

	void DescriptorHeap::Unload()
	{
#ifdef IGLO_D3D12
		maxPersistentResources = 0;
		maxTemporaryResourcesPerFrame = 0;
		maxSamplers = 0;

		numUsedPersistentResources = 0;
		numUsedSamplers = 0;

		numUsedTempResources.clear();
		numUsedTempResources.shrink_to_fit();

		nextPersistentResource = 0;
		nextSampler = 0;

		freedPersistentResources.clear();
		freedPersistentResources.shrink_to_fit();

		freedSamplers.clear();
		freedSamplers.shrink_to_fit();

		d3d12DescriptorHeap_SRV_CBV_UAV.Reset();
		d3d12DescriptorHeap_Sampler.Reset();
#endif

		isLoaded = false;
		context = nullptr;
		frameIndex = 0;
		numFrames = 0;
	}

	bool DescriptorHeap::LogErrorIfNotLoaded()
	{
		if (!isLoaded)
		{
			Log(LogType::Error, "You can't use a descriptor heap that isn't loaded!");
			return true;
		}
		return false;
	}

	bool DescriptorHeap::Load(const IGLOContext& context, uint32_t maxPersistentResourceDescriptors, uint32_t maxTemporaryResourceDescriptorsPerFrame,
		uint32_t maxSamplerDescriptors)
	{
		Unload();

		// The resource descriptor heaps are divided like this:
		// [maxPersistent][maxTemporaryPerFrame frame 0][maxTemporaryPerFrame frame 1][maxTemporaryPerFrame frame N...]
		// 'Resource' in this context means CBV, SRV, UAV.

		uint32_t numFrames = context.GetNumFramesInFlight();

#ifdef IGLO_D3D12
		// Resource descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC resHeapDesc = {};
		resHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		resHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		resHeapDesc.NumDescriptors = maxPersistentResourceDescriptors + (maxTemporaryResourceDescriptorsPerFrame * numFrames);
		HRESULT hrRes = context.GetD3D12Device()->CreateDescriptorHeap(&resHeapDesc, IID_PPV_ARGS(&this->d3d12DescriptorHeap_SRV_CBV_UAV));

		// Sampler descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
		samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		samplerHeapDesc.NumDescriptors = maxSamplerDescriptors;
		HRESULT hrSampler = context.GetD3D12Device()->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&this->d3d12DescriptorHeap_Sampler));

		if (FAILED(hrRes) || FAILED(hrSampler))
		{
			Log(LogType::Error, ToString("Failed to create descriptor heaps."));
			Unload();
			return false;
		}
#endif
#ifdef IGLO_VULKAN
		//TODO: this
#endif

		this->isLoaded = true;
		this->context = &context;
		this->numFrames = numFrames;
		this->maxPersistentResources = maxPersistentResourceDescriptors;
		this->maxTemporaryResourcesPerFrame = maxTemporaryResourceDescriptorsPerFrame;
		this->maxSamplers = maxSamplerDescriptors;
		this->numUsedTempResources.clear();
		this->numUsedTempResources.resize((size_t)numFrames, 0);
		return true;
	}

	void DescriptorHeap::NextFrame()
	{
		if (LogErrorIfNotLoaded()) return;

		frameIndex = (frameIndex + 1) % numFrames;

		numUsedTempResources[frameIndex] = 0;
	}

	void DescriptorHeap::ClearAllTempResources()
	{
		if (LogErrorIfNotLoaded()) return;

		for (size_t i = 0; i < numUsedTempResources.size(); i++)
		{
			numUsedTempResources[i] = 0;
		}
	}

	Descriptor DescriptorHeap::AllocatePersistentResource()
	{
		if (LogErrorIfNotLoaded()) return Descriptor();

		if (numUsedPersistentResources >= maxPersistentResources)
		{
			Log(LogType::Error, ToString("Failed to allocate persistent resource descriptor."
				" Reason: Reached maximum persistent resource descriptors (", maxPersistentResources, ")."));
			return Descriptor();
		}

		if (freedPersistentResources.size() > 0)
		{
			numUsedPersistentResources++;
			uint32_t heapIndex = freedPersistentResources.back();
			freedPersistentResources.pop_back();
			return Descriptor(heapIndex);
		}

		uint32_t heapIndex = nextPersistentResource;
		nextPersistentResource++;
		numUsedPersistentResources++;
		return Descriptor(heapIndex);
	}

	Descriptor DescriptorHeap::AllocateSampler()
	{
		if (LogErrorIfNotLoaded()) return Descriptor();

		if (numUsedSamplers >= maxSamplers)
		{
			Log(LogType::Error, ToString("Failed to allocate sampler descriptor."
				" Reason: Reached maximum sampler descriptors (", maxSamplers, ")."));
			return Descriptor();
		}

		if (freedSamplers.size() > 0)
		{
			numUsedSamplers++;
			uint32_t heapIndex = freedSamplers.back();
			freedSamplers.pop_back();
			return Descriptor(heapIndex);
		}

		uint32_t heapIndex = nextSampler;
		nextSampler++;
		numUsedSamplers++;
		return Descriptor(heapIndex);
	}

	Descriptor DescriptorHeap::AllocateTemporaryResource()
	{
		if (LogErrorIfNotLoaded()) return Descriptor();

		if (numUsedTempResources[frameIndex] >= maxTemporaryResourcesPerFrame)
		{
			Log(LogType::Error, ToString("Failed to allocate temporary resource descriptor."
				" Reason: Reached maximum temporary resource descriptors per frame (", maxTemporaryResourcesPerFrame, ")."));
			return Descriptor();
		}

		uint32_t indexOffset = maxPersistentResources + (frameIndex * maxTemporaryResourcesPerFrame);
		uint32_t heapIndex = indexOffset + numUsedTempResources[frameIndex];
		numUsedTempResources[frameIndex]++;
		return Descriptor(heapIndex);
	}

	void DescriptorHeap::FreePersistentResource(Descriptor descriptor)
	{
		if (LogErrorIfNotLoaded()) return;

		const char* errStr = "Failed to free persistent resource descriptor. Reason: ";

		if (descriptor.IsNull() || descriptor.heapIndex >= nextPersistentResource)
		{
			Log(LogType::Error, ToString(errStr, "Invalid descriptor."));
			return;
		}

#ifdef _DEBUG
		for (size_t i = 0; i < freedPersistentResources.size(); i++)
		{
			if (freedPersistentResources[i] == descriptor.heapIndex)
			{
				Log(LogType::Error, ToString(errStr, "This descriptor is already freed!"));
				return;
			}
		}
#endif
		if (numUsedPersistentResources == 0) throw std::runtime_error("Something isn't right.");

		numUsedPersistentResources--;
		freedPersistentResources.push_back(descriptor.heapIndex);
	}

	void DescriptorHeap::FreeSampler(Descriptor descriptor)
	{
		if (LogErrorIfNotLoaded()) return;

		const char* errStr = "Failed to free sampler descriptor. Reason: ";

		if (descriptor.IsNull() || descriptor.heapIndex >= nextSampler)
		{
			Log(LogType::Error, ToString(errStr, "Invalid descriptor."));
			return;
		}

#ifdef _DEBUG
		for (size_t i = 0; i < freedSamplers.size(); i++)
		{
			if (freedSamplers[i] == descriptor.heapIndex)
			{
				Log(LogType::Error, ToString(errStr, "This descriptor is already freed!"));
				return;
			}
		}
#endif
		if (numUsedSamplers == 0) throw std::runtime_error("Something isn't right.");

		numUsedSamplers--;
		freedSamplers.push_back(descriptor.heapIndex);
	}

#ifdef IGLO_D3D12
	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetD3D12CPUDescriptorHandleForResource(Descriptor d) const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = d3d12DescriptorHeap_SRV_CBV_UAV->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += SIZE_T((UINT64)context->GetD3D12DescriptorSize_CBV_SRV_UAV() * (UINT64)d.heapIndex);
		return handle;
	}
	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetD3D12GPUDescriptorHandleForResource(Descriptor d) const
	{
		D3D12_GPU_DESCRIPTOR_HANDLE handle = d3d12DescriptorHeap_SRV_CBV_UAV->GetGPUDescriptorHandleForHeapStart();
		handle.ptr += UINT64((UINT64)context->GetD3D12DescriptorSize_CBV_SRV_UAV() * (UINT64)d.heapIndex);
		return handle;
	}
	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetD3D12CPUDescriptorHandleForSampler(Descriptor d) const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = d3d12DescriptorHeap_Sampler->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += SIZE_T((UINT64)context->GetD3D12DescriptorSize_Sampler() * (UINT64)d.heapIndex);
		return handle;
	}
	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetD3D12GPUDescriptorHandleForSampler(Descriptor d) const
	{
		D3D12_GPU_DESCRIPTOR_HANDLE handle = d3d12DescriptorHeap_Sampler->GetGPUDescriptorHandleForHeapStart();
		handle.ptr += UINT64((UINT64)context->GetD3D12DescriptorSize_Sampler() * (UINT64)d.heapIndex);
		return handle;
	}
#endif

	void CommandList::Unload()
	{
#ifdef IGLO_D3D12
		d3d12CommandAllocator.clear();
		d3d12CommandAllocator.shrink_to_fit();

		d3d12GraphicsCommandList.Reset();

		d3d12GlobalBarriers.clear();
		d3d12GlobalBarriers.shrink_to_fit();
		d3d12TextureBarriers.clear();
		d3d12TextureBarriers.shrink_to_fit();
		d3d12BufferBarriers.clear();
		d3d12BufferBarriers.shrink_to_fit();

		numGlobalBarriers = 0;
		numTextureBarriers = 0;
		numBufferBarriers = 0;
#endif

		isLoaded = false;
		context = nullptr;
		numFrames = 0;
		frameIndex = 0;
		commandListType = CommandListType::Graphics;
	}

	bool CommandList::Load(const IGLOContext& context, CommandListType commandListType)
	{
		Unload();

		const char* errStr = "Failed to create command list. Reason: ";

#ifdef IGLO_D3D12

		uint32_t numFrames = context.GetNumFramesInFlight();

		D3D12_COMMAND_LIST_TYPE d3d12CmdType = D3D12_COMMAND_LIST_TYPE_NONE;
		switch (commandListType)
		{
		case CommandListType::Graphics:
			d3d12CmdType = D3D12_COMMAND_LIST_TYPE_DIRECT;
			break;
		case CommandListType::Compute:
			d3d12CmdType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			break;
		case CommandListType::Copy:
			d3d12CmdType = D3D12_COMMAND_LIST_TYPE_COPY;
			break;
		default:
			Log(LogType::Error, "Failed to create command list. Reason: Invalid command list type.");
			Unload();
			return false;
		}

		// Create one commmand allocator for each frame
		this->d3d12CommandAllocator.clear();
		this->d3d12CommandAllocator.shrink_to_fit();
		this->d3d12CommandAllocator.resize((size_t)numFrames, nullptr);
		for (uint32_t i = 0; i < numFrames; i++)
		{
			HRESULT hr = context.GetD3D12Device()->CreateCommandAllocator(d3d12CmdType, IID_PPV_ARGS(&this->d3d12CommandAllocator[i]));
			if (FAILED(hr))
			{
				Log(LogType::Error, ToString(errStr, "ID3D12Device::CreateCommandAllocator returned error code: ", (uint32_t)hr, "."));
				Unload();
				return false;
			}
		}

		// Create command list
		{
			this->frameIndex = 0;
			ComPtr<ID3D12GraphicsCommandList> cmdList0;
			HRESULT hr = context.GetD3D12Device()->CreateCommandList(0, d3d12CmdType, this->d3d12CommandAllocator[frameIndex].Get(),
				nullptr, IID_PPV_ARGS(&cmdList0));
			if (FAILED(hr))
			{
				Log(LogType::Error, ToString(errStr, "ID3D12Device::CreateCommandList returned error code: ", (uint32_t)hr, "."));
				Unload();
				return false;
			}
			hr = cmdList0.As(&this->d3d12GraphicsCommandList);
			if (FAILED(hr))
			{
				Log(LogType::Error, ToString(errStr, "ComPtr::As returned error code: ", (uint32_t)hr, "."));
				Unload();
				return false;
			}
		}
		this->d3d12GraphicsCommandList->Close();

		this->d3d12GlobalBarriers.clear();
		this->d3d12GlobalBarriers.resize(IGLO_MAX_BATCHED_BARRIERS_PER_TYPE);
		this->d3d12TextureBarriers.clear();
		this->d3d12TextureBarriers.resize(IGLO_MAX_BATCHED_BARRIERS_PER_TYPE);
		this->d3d12BufferBarriers.clear();
		this->d3d12BufferBarriers.resize(IGLO_MAX_BATCHED_BARRIERS_PER_TYPE);

		this->numGlobalBarriers = 0;
		this->numTextureBarriers = 0;
		this->numBufferBarriers = 0;

		this->isLoaded = true;
		this->context = &context;
		this->numFrames = numFrames;
		this->commandListType = commandListType;
		return true;
#endif
#ifdef IGLO_VULKAN
		return false; //TODO: this
#endif
	}

	void CommandList::Begin()
	{
		if (!isLoaded)
		{
			Log(LogType::Error, "Failed recording commands. Reason: Command list isn't loaded.");
			return;
		}

#ifdef IGLO_D3D12
		if (FAILED(d3d12CommandAllocator[frameIndex]->Reset()))
		{
			Log(LogType::Error, "Failed to reset d3d12 command allocator.");
		}
		if (FAILED(d3d12GraphicsCommandList->Reset(d3d12CommandAllocator[frameIndex].Get(), nullptr)))
		{
			Log(LogType::Error, "Failed to reset d3d12 command list.");
		}
		if (commandListType == CommandListType::Graphics ||
			commandListType == CommandListType::Compute)
		{
			ID3D12DescriptorHeap* heaps[] =
			{
				context->GetDescriptorHeap().GetD3D12DescriptorHeap_SRV_CBV_UAV(),
				context->GetDescriptorHeap().GetD3D12DescriptorHeap_Sampler()
			};
			d3d12GraphicsCommandList->SetDescriptorHeaps(2, heaps);
			if (commandListType == CommandListType::Graphics)
			{
				d3d12GraphicsCommandList->SetGraphicsRootSignature(context->GetD3D12BindlessRootSignature());
			}
			if (commandListType == CommandListType::Graphics || commandListType == CommandListType::Compute)
			{
				d3d12GraphicsCommandList->SetComputeRootSignature(context->GetD3D12BindlessRootSignature());
			}
		}
#endif
	}

	void CommandList::End()
	{
#ifdef IGLO_D3D12
		if (numGlobalBarriers > 0 || numTextureBarriers > 0 || numBufferBarriers > 0)
		{
			numGlobalBarriers = 0;
			numTextureBarriers = 0;
			numBufferBarriers = 0;
			Log(LogType::Warning, "You forgot to flush some barriers!");
		}
		if (FAILED(d3d12GraphicsCommandList->Close()))
		{
			Log(LogType::Error, "Failed to close d3d12 graphics command list.");
		}
#endif
		// Advance to next frame
		frameIndex = (frameIndex + 1) % numFrames;
	}

	void CommandList::FlushBarriers()
	{
#ifdef IGLO_D3D12
		D3D12_BARRIER_GROUP groups[3] = {};
		uint32_t index = 0;
		if (numGlobalBarriers > 0)
		{
			groups[index].pGlobalBarriers = d3d12GlobalBarriers.data();
			groups[index].NumBarriers = numGlobalBarriers;
			groups[index].Type = D3D12_BARRIER_TYPE_GLOBAL;
			index++;
		}
		if (numTextureBarriers > 0)
		{
			groups[index].pTextureBarriers = d3d12TextureBarriers.data();
			groups[index].NumBarriers = numTextureBarriers;
			groups[index].Type = D3D12_BARRIER_TYPE_TEXTURE;
			index++;
		}
		if (numBufferBarriers > 0)
		{
			groups[index].pBufferBarriers = d3d12BufferBarriers.data();
			groups[index].NumBarriers = numBufferBarriers;
			groups[index].Type = D3D12_BARRIER_TYPE_BUFFER;
			index++;
		}
		if (index > 0)
		{
			d3d12GraphicsCommandList->Barrier(index, groups);
		}

		numGlobalBarriers = 0;
		numTextureBarriers = 0;
		numBufferBarriers = 0;
#endif
	}

	void CommandList::AddGlobalBarrier(BarrierSync syncBefore, BarrierSync syncAfter, BarrierAccess accessBefore, BarrierAccess accessAfter)
	{
#ifdef IGLO_D3D12
		if (numGlobalBarriers >= IGLO_MAX_BATCHED_BARRIERS_PER_TYPE) FlushBarriers();

		D3D12_GLOBAL_BARRIER& barrier = d3d12GlobalBarriers.at(numGlobalBarriers);
		barrier.SyncBefore = (D3D12_BARRIER_SYNC)syncBefore;
		barrier.SyncAfter = (D3D12_BARRIER_SYNC)syncAfter;
		barrier.AccessBefore = (D3D12_BARRIER_ACCESS)accessBefore;
		barrier.AccessAfter = (D3D12_BARRIER_ACCESS)accessAfter;

		numGlobalBarriers++;
#endif
	}

	void CommandList::AddTextureBarrier(const Texture& texture, BarrierSync syncBefore, BarrierSync syncAfter,
		BarrierAccess accessBefore, BarrierAccess accessAfter, BarrierLayout layoutBefore, BarrierLayout layoutAfter, bool discard)
	{
#ifdef IGLO_D3D12
		if (numTextureBarriers >= IGLO_MAX_BATCHED_BARRIERS_PER_TYPE) FlushBarriers();

		D3D12_TEXTURE_BARRIER& barrier = d3d12TextureBarriers.at(numTextureBarriers);
		barrier.pResource = texture.GetD3D12Resource();
		barrier.SyncBefore = (D3D12_BARRIER_SYNC)syncBefore;
		barrier.SyncAfter = (D3D12_BARRIER_SYNC)syncAfter;
		barrier.AccessBefore = (D3D12_BARRIER_ACCESS)accessBefore;
		barrier.AccessAfter = (D3D12_BARRIER_ACCESS)accessAfter;
		barrier.LayoutBefore = (D3D12_BARRIER_LAYOUT)layoutBefore;
		barrier.LayoutAfter = (D3D12_BARRIER_LAYOUT)layoutAfter;
		barrier.Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE;
		if (discard) barrier.Flags = D3D12_TEXTURE_BARRIER_FLAG_DISCARD;
		barrier.Subresources.IndexOrFirstMipLevel = 0xffffffff;

		numTextureBarriers++;
#endif
	}

	void CommandList::AddTextureBarrierAtSubresource(const Texture& texture, BarrierSync syncBefore, BarrierSync syncAfter,
		BarrierAccess accessBefore, BarrierAccess accessAfter, BarrierLayout layoutBefore, BarrierLayout layoutAfter,
		uint32_t faceIndex, uint32_t mipIndex, bool discard)
	{
#ifdef IGLO_D3D12
		if (numTextureBarriers >= IGLO_MAX_BATCHED_BARRIERS_PER_TYPE) FlushBarriers();

		D3D12_TEXTURE_BARRIER& barrier = d3d12TextureBarriers.at(numTextureBarriers);
		barrier.pResource = texture.GetD3D12Resource();
		barrier.SyncBefore = (D3D12_BARRIER_SYNC)syncBefore;
		barrier.SyncAfter = (D3D12_BARRIER_SYNC)syncAfter;
		barrier.AccessBefore = (D3D12_BARRIER_ACCESS)accessBefore;
		barrier.AccessAfter = (D3D12_BARRIER_ACCESS)accessAfter;
		barrier.LayoutBefore = (D3D12_BARRIER_LAYOUT)layoutBefore;
		barrier.LayoutAfter = (D3D12_BARRIER_LAYOUT)layoutAfter;
		barrier.Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE;
		if (discard) barrier.Flags = D3D12_TEXTURE_BARRIER_FLAG_DISCARD;
		barrier.Subresources.IndexOrFirstMipLevel = (faceIndex * texture.GetNumMipLevels()) + mipIndex;

		numTextureBarriers++;
#endif
	}

	SimpleBarrierInfo GetSimpleBarrierInfo(SimpleBarrier simpleBarrier, bool useQueueLayout, CommandListType queueType)
	{
		SimpleBarrierInfo out;

		bool useGraphicsQueue = (useQueueLayout && queueType == CommandListType::Graphics);
		bool useComputeQueue = (useQueueLayout && queueType == CommandListType::Compute);

		switch (simpleBarrier)
		{
		default:
			Log(LogType::Error, "Failed to get simple barrier info. Reason: Invalid simple barrier provided.");
			return SimpleBarrierInfo();

		case SimpleBarrier::Common:
			out.sync = BarrierSync::All;
			out.access = BarrierAccess::Common;
			out.layout = BarrierLayout::Common;
			if (useGraphicsQueue) out.layout = BarrierLayout::_GraphicsQueue_Common;
			if (useComputeQueue) out.layout = BarrierLayout::_ComputeQueue_Common;
			break;

		case SimpleBarrier::PixelShaderResource:
			out.sync = BarrierSync::PixelShading;
			if (queueType == CommandListType::Compute) out.sync = BarrierSync::AllShading;
			out.access = BarrierAccess::ShaderResource;
			out.layout = BarrierLayout::ShaderResource;
			if (useGraphicsQueue) out.layout = BarrierLayout::_GraphicsQueue_ShaderResource;
			if (useComputeQueue) out.layout = BarrierLayout::_ComputeQueue_ShaderResource;
			break;

		case SimpleBarrier::ComputeShaderResource:
			out.sync = BarrierSync::ComputeShading;
			out.access = BarrierAccess::ShaderResource;
			out.layout = BarrierLayout::ShaderResource;
			if (useGraphicsQueue) out.layout = BarrierLayout::_GraphicsQueue_ShaderResource;
			if (useComputeQueue) out.layout = BarrierLayout::_ComputeQueue_ShaderResource;
			break;

		case SimpleBarrier::ComputeShaderUnorderedAccess:
			out.sync = BarrierSync::ComputeShading;
			out.access = BarrierAccess::UnorderedAccess;
			out.layout = BarrierLayout::UnorderedAccess;
			if (useGraphicsQueue) out.layout = BarrierLayout::_GraphicsQueue_UnorderedAccess;
			if (useComputeQueue) out.layout = BarrierLayout::_ComputeQueue_UnorderedAccess;
			break;

		case SimpleBarrier::RenderTarget:
			out.sync = BarrierSync::RenderTarget;
			out.access = BarrierAccess::RenderTarget;
			out.layout = BarrierLayout::RenderTarget;
			break;

		case SimpleBarrier::DepthStencil:
			out.sync = BarrierSync::DepthStencil;
			out.access = BarrierAccess::DepthStencilWrite;
			out.layout = BarrierLayout::DepthStencilWrite;
			break;

		case SimpleBarrier::CopySource:
			out.sync = BarrierSync::Copy;
			out.access = BarrierAccess::CopySource;
			out.layout = BarrierLayout::CopySource;
			if (useGraphicsQueue) out.layout = BarrierLayout::_GraphicsQueue_CopySource;
			if (useComputeQueue) out.layout = BarrierLayout::_ComputeQueue_CopySource;
			break;

		case SimpleBarrier::CopyDest:
			out.sync = BarrierSync::Copy;
			out.access = BarrierAccess::CopyDest;
			out.layout = BarrierLayout::CopyDest;
			if (useGraphicsQueue) out.layout = BarrierLayout::_GraphicsQueue_CopyDest;
			if (useComputeQueue) out.layout = BarrierLayout::_ComputeQueue_CopyDest;
			break;

		case SimpleBarrier::ResolveSource:
			out.sync = BarrierSync::Resolve;
			out.access = BarrierAccess::ResolveSource;
			out.layout = BarrierLayout::ResolveSource;
			break;

		case SimpleBarrier::ResolveDest:
			out.sync = BarrierSync::Resolve;
			out.access = BarrierAccess::ResolveDest;
			out.layout = BarrierLayout::ResolveDest;
			break;
		}

		return out;
	}

	void CommandList::AddTextureBarrier(const Texture& texture, SimpleBarrier before, SimpleBarrier after, bool useQueueSpecificLayout)
	{
		SimpleBarrierInfo infoBefore = GetSimpleBarrierInfo(before, useQueueSpecificLayout, commandListType);
		SimpleBarrierInfo infoAfter = GetSimpleBarrierInfo(after, useQueueSpecificLayout, commandListType);
		AddTextureBarrier(texture, infoBefore.sync, infoAfter.sync, infoBefore.access, infoAfter.access, infoBefore.layout, infoAfter.layout);
	}
	void CommandList::AddTextureBarrierAtSubresource(const Texture& texture, SimpleBarrier before, SimpleBarrier after,
		uint32_t faceIndex, uint32_t mipIndex, bool useQueueSpecificLayout)
	{
		SimpleBarrierInfo infoBefore = GetSimpleBarrierInfo(before, useQueueSpecificLayout, commandListType);
		SimpleBarrierInfo infoAfter = GetSimpleBarrierInfo(after, useQueueSpecificLayout, commandListType);
		AddTextureBarrierAtSubresource(texture, infoBefore.sync, infoAfter.sync, infoBefore.access, infoAfter.access,
			infoBefore.layout, infoAfter.layout, faceIndex, mipIndex);
	}

	void CommandList::AddBufferBarrier(const Buffer& buffer, BarrierSync syncBefore, BarrierSync syncAfter,
		BarrierAccess accessBefore, BarrierAccess accessAfter)
	{
#ifdef IGLO_D3D12
		if (numBufferBarriers >= IGLO_MAX_BATCHED_BARRIERS_PER_TYPE) FlushBarriers();

		D3D12_BUFFER_BARRIER& barrier = d3d12BufferBarriers.at(numBufferBarriers);

		barrier.pResource = buffer.GetD3D12Resource();
		barrier.SyncBefore = (D3D12_BARRIER_SYNC)syncBefore;
		barrier.SyncAfter = (D3D12_BARRIER_SYNC)syncAfter;
		barrier.AccessBefore = (D3D12_BARRIER_ACCESS)accessBefore;
		barrier.AccessAfter = (D3D12_BARRIER_ACCESS)accessAfter;
		barrier.Size = UINT64_MAX;
		barrier.Offset = 0;

		numBufferBarriers++;
#endif
	}

	void CommandList::SetPipeline(const Pipeline& pipeline)
	{
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->SetPipelineState(pipeline.GetD3D12PipelineState());
#endif
	}

	void CommandList::SetRenderTarget(const Texture* renderTexture, const Texture* depthBuffer)
	{
		if (renderTexture)
		{
			const Texture* renderTextures[] = { renderTexture };
			SetRenderTargets(renderTextures, 1, depthBuffer);
		}
		else
		{
			SetRenderTargets(nullptr, 0, depthBuffer);
		}
	}

	void CommandList::SetRenderTargets(const Texture** renderTextures, uint32_t numRenderTextures, const Texture* depthBuffer)
	{
		if (numRenderTextures > 0 && !renderTextures)
		{
			Log(LogType::Error, "Failed to set render targets. Reason: Bad arguments.");
			return;
		}
		if (numRenderTextures > IGLO_MAX_SIMULTANEOUS_RENDER_TARGETS)
		{
			Log(LogType::Error, "Failed to set render targets. Reason: Too many render textures provided.");
			return;
		}

#ifdef IGLO_D3D12
		D3D12_CPU_DESCRIPTOR_HANDLE rtvStart = context->GetD3D12RTVHandle();
		for (uint32_t i = 0; i < numRenderTextures; i++)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE rtvCurrent = { rtvStart.ptr + SIZE_T((uint64_t)context->GetD3D12DescriptorSize_RTV() * i) };
			context->GetD3D12Device()->CreateRenderTargetView(renderTextures[i]->GetD3D12Resource(), nullptr, rtvCurrent);
		}
		const D3D12_CPU_DESCRIPTOR_HANDLE* dsvStart = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = context->GetD3D12DSVHandle();
		if (depthBuffer)
		{
			dsvStart = &dsvHandle;
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = depthBuffer->GenerateD3D12DSVDesc();
			context->GetD3D12Device()->CreateDepthStencilView(depthBuffer->GetD3D12Resource(), &dsvDesc, context->GetD3D12DSVHandle());
		}
		d3d12GraphicsCommandList->OMSetRenderTargets(numRenderTextures, &rtvStart, TRUE, dsvStart);
#endif
	}

	void CommandList::ClearColor(const Texture& renderTexture, Color color, uint32_t numRects, const IntRect* rects)
	{
		if (numRects > 0 && !rects)
		{
			Log(LogType::Error, "Failed to clear render texture. Reason: Bad arguments.");
			return;
		}

#ifdef IGLO_D3D12
		context->GetD3D12Device()->CreateRenderTargetView(renderTexture.GetD3D12Resource(), nullptr, context->GetD3D12RTVHandle());
		d3d12GraphicsCommandList->ClearRenderTargetView(context->GetD3D12RTVHandle(), (FLOAT*)&color, numRects, (D3D12_RECT*)rects);
#endif
	}

	void CommandList::ClearDepth(const Texture& depthBuffer, float depth, byte stencil, bool clearDepth, bool clearStencil,
		uint32_t numRects, const IntRect* rects)
	{
		if (numRects > 0 && !rects)
		{
			Log(LogType::Error, "Failed to clear depth buffer. Reason: Bad arguments.");
			return;
		}

#ifdef IGLO_D3D12
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = depthBuffer.GenerateD3D12DSVDesc();
		context->GetD3D12Device()->CreateDepthStencilView(depthBuffer.GetD3D12Resource(), &dsvDesc, context->GetD3D12DSVHandle());
		D3D12_CLEAR_FLAGS flags = (D3D12_CLEAR_FLAGS)0;
		if (clearDepth) flags |= D3D12_CLEAR_FLAG_DEPTH;
		if (clearStencil) flags |= D3D12_CLEAR_FLAG_STENCIL;
		d3d12GraphicsCommandList->ClearDepthStencilView(context->GetD3D12DSVHandle(), flags, depth, stencil, numRects, (D3D12_RECT*)rects);
#endif
	}

	void CommandList::ClearUnorderedAccessBufferFloat(const Buffer& buffer, const float values[4], uint32_t numRects, const IntRect* rects)
	{
		if (!buffer.IsLoaded() || !buffer.GetUnorderedAccessDescriptor())
		{
			Log(LogType::Error, "Failed to clear unordered access buffer. Reason: Invalid buffer.");
			return;
		}
		if (numRects > 0 && !rects)
		{
			Log(LogType::Error, "Failed to clear unordered access buffer. Reason: Bad arguments.");
			return;
		}

#ifdef IGLO_D3D12
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = buffer.GenerateD3D12UAVDesc();
		context->GetD3D12Device()->CreateUnorderedAccessView(buffer.GetD3D12Resource(), nullptr, &uavDesc, context->GetD3D12UAVHandle());

		d3d12GraphicsCommandList->ClearUnorderedAccessViewFloat(
			context->GetDescriptorHeap().GetD3D12GPUDescriptorHandleForResource(buffer.GetUnorderedAccessDescriptor()->heapIndex),
			context->GetD3D12UAVHandle(), buffer.GetD3D12Resource(), values, numRects, (D3D12_RECT*)rects);
#endif
	}

	void CommandList::ClearUnorderedAccessBufferUInt32(const Buffer& buffer, const uint32_t values[4], uint32_t numRects, const IntRect* rects)
	{
		if (!buffer.IsLoaded() || !buffer.GetUnorderedAccessDescriptor())
		{
			Log(LogType::Error, "Failed to clear unordered access buffer. Reason: Invalid buffer.");
			return;
		}
		if (numRects > 0 && !rects)
		{
			Log(LogType::Error, "Failed to clear unordered access buffer. Reason: Bad arguments.");
			return;
		}

#ifdef IGLO_D3D12
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = buffer.GenerateD3D12UAVDesc();
		context->GetD3D12Device()->CreateUnorderedAccessView(buffer.GetD3D12Resource(), nullptr, &uavDesc, context->GetD3D12UAVHandle());

		d3d12GraphicsCommandList->ClearUnorderedAccessViewUint(
			context->GetDescriptorHeap().GetD3D12GPUDescriptorHandleForResource(buffer.GetUnorderedAccessDescriptor()->heapIndex),
			context->GetD3D12UAVHandle(), buffer.GetD3D12Resource(), values, numRects, (D3D12_RECT*)rects);
#endif
	}

	void CommandList::ClearUnorderedAccessTextureFloat(const Texture& texture, ig::Color values, uint32_t numRects, const IntRect* rects)
	{
		if (!texture.IsLoaded() || !texture.GetUnorderedAccessDescriptor())
		{
			Log(LogType::Error, "Failed to clear unordered access texture. Reason: Invalid texture.");
			return;
		}
		if (numRects > 0 && !rects)
		{
			Log(LogType::Error, "Failed to clear unordered access texture. Reason: Bad arguments.");
			return;
		}

#ifdef IGLO_D3D12
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = texture.GenerateD3D12UAVDesc();
		context->GetD3D12Device()->CreateUnorderedAccessView(texture.GetD3D12Resource(), nullptr, &uavDesc, context->GetD3D12UAVHandle());

		d3d12GraphicsCommandList->ClearUnorderedAccessViewFloat(
			context->GetDescriptorHeap().GetD3D12GPUDescriptorHandleForResource(texture.GetUnorderedAccessDescriptor()->heapIndex),
			context->GetD3D12UAVHandle(), texture.GetD3D12Resource(), (FLOAT*)&values, numRects, (D3D12_RECT*)rects);
#endif
	}

	void CommandList::ClearUnorderedAccessTextureUInt32(const Texture& texture, const uint32_t values[4], 
		uint32_t numRects, const IntRect* rects)
	{
		if (!texture.IsLoaded() || !texture.GetUnorderedAccessDescriptor())
		{
			Log(LogType::Error, "Failed to clear unordered access texture. Reason: Invalid texture.");
			return;
		}
		if (numRects > 0 && !rects)
		{
			Log(LogType::Error, "Failed to clear unordered access texture. Reason: Bad arguments.");
			return;
		}

#ifdef IGLO_D3D12
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = texture.GenerateD3D12UAVDesc();
		context->GetD3D12Device()->CreateUnorderedAccessView(texture.GetD3D12Resource(), nullptr, &uavDesc, context->GetD3D12UAVHandle());

		d3d12GraphicsCommandList->ClearUnorderedAccessViewUint(
			context->GetDescriptorHeap().GetD3D12GPUDescriptorHandleForResource(texture.GetUnorderedAccessDescriptor()->heapIndex),
			context->GetD3D12UAVHandle(), texture.GetD3D12Resource(), values, numRects, (D3D12_RECT*)rects);
#endif
	}

	bool CommandList::LogErrorIfInvalidPushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes)
	{
		if (sizeInBytes > IGLO_PUSH_CONSTANT_TOTAL_BYTE_SIZE)
		{
			Log(LogType::Error, ToString("Failed to set push constants of size ", sizeInBytes, ".",
				" Reason: Max push constant size is ", IGLO_PUSH_CONSTANT_TOTAL_BYTE_SIZE, "."));
			return false;
		}
		if (sizeInBytes % 4 != 0 || destOffsetInBytes % 4 != 0)
		{
			Log(LogType::Error, ToString("Failed to set push constants of size ", sizeInBytes, " and offset ", destOffsetInBytes, "."
				" Reason: Size and offset must be multiples of 4."));
			return false;
		}
		if (!data || sizeInBytes == 0)
		{
			Log(LogType::Error, ToString("Failed to set push constants of size ", sizeInBytes, ". Reason: No data provided."));
			return false;
		}
		return true;
	}

	void CommandList::SetPushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes)
	{
		if (!LogErrorIfInvalidPushConstants(data, sizeInBytes, destOffsetInBytes)) return;
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->SetGraphicsRoot32BitConstants(0, sizeInBytes / 4, data, destOffsetInBytes / 4);
#endif
	}
	void CommandList::SetComputePushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes)
	{
		if (!LogErrorIfInvalidPushConstants(data, sizeInBytes, destOffsetInBytes)) return;
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->SetComputeRoot32BitConstants(0, sizeInBytes / 4, data, destOffsetInBytes / 4);
#endif
	}

	void CommandList::SetPrimitiveTopology(Primitive primitiveTopology)
	{
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->IASetPrimitiveTopology((D3D12_PRIMITIVE_TOPOLOGY)primitiveTopology);
#endif
	}

	void CommandList::SetVertexBuffer(const Buffer& vertexBuffer, uint32_t slot)
	{
#ifdef IGLO_D3D12
		if ((uint64_t)slot + 1 > D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT)
		{
			Log(LogType::Error, "Failed setting vertex buffer. Reason: Vertex buffer slot out of bounds!");
			return;
		}
		D3D12_VERTEX_BUFFER_VIEW view = {};
		view.BufferLocation = vertexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
		view.SizeInBytes = (UINT)vertexBuffer.GetSize();
		view.StrideInBytes = vertexBuffer.GetStride();
		d3d12GraphicsCommandList->IASetVertexBuffers(slot, 1, &view);
#endif
	}

	void CommandList::SetVertexBuffers(const Buffer** vertexBuffers, uint32_t count, uint32_t startSlot)
	{
#ifdef IGLO_D3D12
		if (count == 0 || !vertexBuffers)
		{
			Log(LogType::Error, "Failed setting vertex buffers. Reason: No vertex buffers provided.");
			return;
		}
		if (count + startSlot > D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT)
		{
			Log(LogType::Error, "Failed setting vertex buffers. Reason: Vertex buffer slot out of bounds!");
			return;
		}
		D3D12_VERTEX_BUFFER_VIEW views[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
		for (uint32_t i = 0; i < count; i++)
		{
			views[i].BufferLocation = vertexBuffers[i]->GetD3D12Resource()->GetGPUVirtualAddress();
			views[i].SizeInBytes = (UINT)vertexBuffers[i]->GetSize();
			views[i].StrideInBytes = vertexBuffers[i]->GetStride();
		}
		d3d12GraphicsCommandList->IASetVertexBuffers(startSlot, count, views);
#endif
	}

	void CommandList::SetTempVertexBuffer(const void* data, uint64_t sizeInBytes, uint32_t vertexStride, uint32_t slot)
	{
#ifdef IGLO_D3D12
		if ((uint64_t)slot + 1 > D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT)
		{
			Log(LogType::Error, "Failed setting temporary vertex buffer. Reason: Vertex buffer slot out of bounds!");
			return;
		}
#endif

		TempBuffer vb = context->GetBufferAllocator().AllocateTempBuffer(sizeInBytes,
			(uint32_t)BufferPlacementAlignment::VertexOrIndexBuffer);
		if (vb.IsNull())
		{
			Log(LogType::Error, "Failed setting temporary vertex buffer. Reason: Failed to allocate temporary buffer.");
			return;
		}

		memcpy(vb.data, data, sizeInBytes);

#ifdef IGLO_D3D12
		D3D12_VERTEX_BUFFER_VIEW view = {};
		view.BufferLocation = vb.d3d12Resource->GetGPUVirtualAddress() + vb.offset;
		view.SizeInBytes = (UINT)sizeInBytes;
		view.StrideInBytes = (UINT)vertexStride;
		d3d12GraphicsCommandList->IASetVertexBuffers(slot, 1, &view);
#endif
	}

	void CommandList::SetIndexBuffer(const Buffer& indexBuffer)
	{
#ifdef IGLO_D3D12
		D3D12_INDEX_BUFFER_VIEW view = {};
		view.BufferLocation = indexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
		view.SizeInBytes = (UINT)indexBuffer.GetSize();
		view.Format = (indexBuffer.GetStride() == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
		d3d12GraphicsCommandList->IASetIndexBuffer(&view);
#endif
	}

	void CommandList::Draw(uint32_t vertexCount, uint32_t startVertexLocation)
	{
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->DrawInstanced(vertexCount, 1, startVertexLocation, 0);
#endif
	}
	void CommandList::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation,
		uint32_t startInstanceLocation)
	{
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
#endif
	}
	void CommandList::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation)
	{
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
#endif
	}
	void CommandList::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
		int32_t baseVertexLocation, uint32_t startInstanceLocation)
	{
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation,
			baseVertexLocation, startInstanceLocation);
#endif
	}

	void CommandList::SetViewport(float width, float height)
	{
		Viewport viewport(0, 0, width, height);
		SetViewport(viewport);
	}
	void CommandList::SetViewport(Viewport viewPort)
	{
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->RSSetViewports(1, (D3D12_VIEWPORT*)&viewPort);
#endif
	}
	void CommandList::SetViewports(const Viewport* viewPorts, uint32_t count)
	{
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->RSSetViewports(count, (D3D12_VIEWPORT*)viewPorts);
#endif
	}

	void CommandList::SetScissorRectangle(int32_t width, int32_t height)
	{
#ifdef IGLO_D3D12
		D3D12_RECT rect = { 0, 0, width, height };
		d3d12GraphicsCommandList->RSSetScissorRects(1, &rect);
#endif

	}
	void CommandList::SetScissorRectangle(IntRect scissorRect)
	{
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->RSSetScissorRects(1, (D3D12_RECT*)(&scissorRect));
#endif
	}
	void CommandList::SetScissorRectangles(const IntRect* scissorRects, uint32_t count)
	{
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->RSSetScissorRects(count, (D3D12_RECT*)scissorRects);
#endif
	}

	void CommandList::DispatchCompute(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
	{
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
#endif
	}

	void CommandList::CopyTexture(const Texture& source, const Texture& destination)
	{
		if (source.GetUsage() == TextureUsage::Readable)
		{
			Log(LogType::Error, "Failed to issue a copy texture command. Reason: The source texture must not have Readable usage.");
			return;
		}
		if (destination.GetUsage() == TextureUsage::Readable)
		{
			CopyTextureToReadableTexture(source, destination);
			return;
		}

#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->CopyResource(destination.GetD3D12Resource(), source.GetD3D12Resource());
#endif
	}

	void CommandList::CopyTextureSubresource(const Texture& source, uint32_t sourceFaceIndex, uint32_t sourceMipIndex,
		const Texture& destination, uint32_t destFaceIndex, uint32_t destMipIndex)
	{
		if (source.GetUsage() == TextureUsage::Readable)
		{
			Log(LogType::Error, "Failed to issue a copy texture subresource command. Reason: The source texture must not have Readable usage.");
			return;
		}
		if (destination.GetUsage() == TextureUsage::Readable)
		{
			//TODO: implement this
			Log(LogType::Error, "Failed to issue a copy texture subresource command."
				" Reason: The ability to copy a texture subresource to a readable texture subresource is not yet implemented.");
			return;
		}

#ifdef IGLO_D3D12
		D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
		srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		srcLoc.pResource = source.GetD3D12Resource();
		srcLoc.SubresourceIndex = (sourceFaceIndex * source.GetNumMipLevels()) + sourceMipIndex;

		D3D12_TEXTURE_COPY_LOCATION destLoc = {};
		destLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destLoc.pResource = destination.GetD3D12Resource();
		destLoc.SubresourceIndex = (destFaceIndex * destination.GetNumMipLevels()) + destMipIndex;

		d3d12GraphicsCommandList->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);
#endif
	}

	void CommandList::CopyTextureToReadableTexture(const Texture& source, const Texture& destination)
	{
		if (!source.IsLoaded() ||
			!destination.IsLoaded() ||
			source.GetUsage() == TextureUsage::Readable ||
			destination.GetUsage() != TextureUsage::Readable)
		{
			Log(LogType::Error, "Unabled to copy texture to readable texture."
				" Reason: Destination texture usage must be Readable, and source texture usage must be non-readable.");
			return;
		}

#ifdef IGLO_D3D12
		D3D12_RESOURCE_DESC desc = source.GetD3D12Resource()->GetDesc();
		uint32_t numSubResources = source.GetNumFaces() * source.GetNumMipLevels();
		uint64_t currentOffset = 0;
		for (uint32_t i = 0; i < numSubResources; i++)
		{
			D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
			srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			srcLoc.pResource = source.GetD3D12Resource();
			srcLoc.SubresourceIndex = i;

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
			uint64_t totalBytes = 0;
			context->GetD3D12Device()->GetCopyableFootprints(&desc, i, 1, 0, &footprint, nullptr, nullptr, &totalBytes);

			D3D12_TEXTURE_COPY_LOCATION destLoc = {};
			destLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			destLoc.pResource = destination.GetD3D12Resource();
			destLoc.PlacedFootprint = footprint;
			destLoc.PlacedFootprint.Offset = currentOffset;
			currentOffset += AlignUp(totalBytes, (uint64_t)BufferPlacementAlignment::TextureRowPitch);

			d3d12GraphicsCommandList->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);
		}
#endif
	}

	void CommandList::CopyBuffer(const Buffer& source, const Buffer& destination)
	{
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->CopyResource(destination.GetD3D12Resource(), source.GetD3D12Resource());
#endif
	}

	void CommandList::CopyTempBufferToTexture(const TempBuffer& source, const Texture& destination)
	{
#ifdef IGLO_D3D12
		D3D12_RESOURCE_DESC desc = destination.GetD3D12Resource()->GetDesc();
		uint32_t numSubResources = desc.MipLevels * desc.DepthOrArraySize;

		uint64_t currentOffset = source.offset;
		for (uint32_t i = 0; i < numSubResources; i++)
		{
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
			uint64_t totalBytes = 0;
			context->GetD3D12Device()->GetCopyableFootprints(&desc, i, 1, 0, &footprint, nullptr, nullptr, &totalBytes);

			D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
			srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			srcLoc.pResource = source.d3d12Resource;
			srcLoc.PlacedFootprint = footprint;
			srcLoc.PlacedFootprint.Offset = currentOffset;
			currentOffset += AlignUp(totalBytes, (uint64_t)BufferPlacementAlignment::TextureRowPitch);

			D3D12_TEXTURE_COPY_LOCATION destLoc = {};
			destLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			destLoc.pResource = destination.GetD3D12Resource();
			destLoc.SubresourceIndex = i;

			d3d12GraphicsCommandList->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);
		}
#endif
	}

	void CommandList::CopyTempBufferToTextureSubresource(const TempBuffer& source, const Texture& destination, uint32_t destFaceIndex, uint32_t destMipIndex)
	{
#ifdef IGLO_D3D12
		D3D12_RESOURCE_DESC desc = destination.GetD3D12Resource()->GetDesc();
		uint32_t subResourceIndex = (destFaceIndex * destination.GetNumMipLevels()) + destMipIndex;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		uint64_t totalBytes = 0;
		context->GetD3D12Device()->GetCopyableFootprints(&desc, subResourceIndex, 1, 0, &footprint, nullptr, nullptr, &totalBytes);

		D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
		srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLoc.pResource = source.d3d12Resource;
		srcLoc.PlacedFootprint = footprint;
		srcLoc.PlacedFootprint.Offset = source.offset;

		D3D12_TEXTURE_COPY_LOCATION destLoc = {};
		destLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destLoc.pResource = destination.GetD3D12Resource();
		destLoc.SubresourceIndex = subResourceIndex;

		d3d12GraphicsCommandList->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);
#endif
	}

	void CommandList::CopyTempBufferToBuffer(const TempBuffer& source, const Buffer& destination)
	{
#ifdef IGLO_D3D12
		d3d12GraphicsCommandList->CopyBufferRegion(destination.GetD3D12Resource(), 0, source.d3d12Resource,
			source.offset, destination.GetSize());
#endif
	}

	void CommandList::ResolveTexture(const Texture& source, const Texture& destination)
	{
#ifdef IGLO_D3D12
		//TODO: Add support for resolving cubemaps
		d3d12GraphicsCommandList->ResolveSubresource(destination.GetD3D12Resource(), 0, source.GetD3D12Resource(), 0,
			GetFormatInfoDXGI(source.GetFormat()).dxgiFormat);
#endif
	}

	void BufferAllocator::Unload()
	{
#ifdef IGLO_D3D12
		for (size_t i = 0; i < perFrame.size(); i++)
		{
			for (size_t p = 0; p < perFrame[i].largePages.size(); p++)
			{
				perFrame[i].largePages[p].d3d12Resource->Unmap(0, nullptr);
			}
			for (size_t p = 0; p < perFrame[i].linearPages.size(); p++)
			{
				perFrame[i].linearPages[p].d3d12Resource->Unmap(0, nullptr);
			}
		}
#endif
		perFrame.clear();
		perFrame.shrink_to_fit();

		isLoaded = false;
		context = nullptr;
		frameIndex = 0;
		numFrames = 0;
		pageSize = 0;
	}

	bool BufferAllocator::Load(const IGLOContext& context, uint64_t pageSizeInBytes)
	{
		Unload();

		const char* errStr = "Failed to create buffer allocator. Reason: ";

		uint32_t numFrames = context.GetNumFramesInFlight();

		perFrame.clear();
		perFrame.shrink_to_fit();

		// Each frame has a set number of persistent pages, and a changing number of temporary pages placed ontop.
		// The temporary pages are created as needed when the persistent pages run out of space.
		// The temporary pages are deleted on a per-frame basis.
		// The persistent pages are reused frame by frame.

		for (size_t i = 0; i < numFrames; i++)
		{
			PerFrame frame;
			frame.linearNextByte = 0;

			for (size_t p = 0; p < numPersistentPages; p++)
			{
				Page page = CreatePage(context, pageSizeInBytes);
				if (page.IsNull())
				{
					Log(LogType::Error, ToString(errStr, "Failed to create page."));
					Unload();
					return false;
				}
				frame.linearPages.push_back(page);
			}

			this->perFrame.push_back(frame);
		}

		this->isLoaded = true;
		this->context = &context;
		this->frameIndex = 0;
		this->numFrames = numFrames;
		this->pageSize = pageSizeInBytes;
		return true;
	}

	BufferAllocator::Page BufferAllocator::CreatePage(const IGLOContext& context, uint64_t sizeInBytes)
	{

#ifdef IGLO_D3D12
		Page out;

		D3D12_RESOURCE_DESC1 desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = sizeInBytes;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		desc.SamplerFeedbackMipRegion.Width = 0;
		desc.SamplerFeedbackMipRegion.Height = 0;
		desc.SamplerFeedbackMipRegion.Depth = 0;

		D3D12_HEAP_PROPERTIES heapProp = {};
		heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;

		D3D12_BARRIER_LAYOUT barrierLayout = D3D12_BARRIER_LAYOUT_UNDEFINED;

		HRESULT hr = context.GetD3D12Device()->CreateCommittedResource3(&heapProp, D3D12_HEAP_FLAG_NONE, &desc, barrierLayout,
			nullptr, nullptr, 0, nullptr, IID_PPV_ARGS(&out.d3d12Resource));
		if (FAILED(hr)) return Page();

		D3D12_RANGE noRead = { 0, 0 };
		hr = out.d3d12Resource->Map(0, &noRead, &out.mapped);
		if (FAILED(hr)) return Page();

		return out;
#endif
#ifdef IGLO_VULKAN
		return Page(); //TODO: this
#endif
	}

	bool BufferAllocator::LogErrorIfNotLoaded()
	{
		if (!isLoaded)
		{
			Log(LogType::Error, "You can't use a buffer allocator that isn't loaded!");
			return true;
		}
		return false;
	}

	void BufferAllocator::NextFrame()
	{
		if (LogErrorIfNotLoaded()) return;

		frameIndex = (frameIndex + 1) % numFrames;

		ClearTempPagesAtFrame(perFrame.at(frameIndex));
	}

	void BufferAllocator::ClearAllTempPages()
	{
		if (LogErrorIfNotLoaded()) return;

		for (size_t i = 0; i < perFrame.size(); i++)
		{
			ClearTempPagesAtFrame(perFrame.at(i));
		}
	}

	void BufferAllocator::ClearTempPagesAtFrame(PerFrame& perFrame)
	{
#ifdef IGLO_D3D12

		// Delete extra linear pages
		while (perFrame.linearPages.size() > numPersistentPages)
		{
			perFrame.linearPages.back().d3d12Resource->Unmap(0, nullptr);
			perFrame.linearPages.back().d3d12Resource.Reset();
			perFrame.linearPages.pop_back();
		}

		// Delete large pages
		for (size_t i = 0; i < perFrame.largePages.size(); i++)
		{
			perFrame.largePages[i].d3d12Resource->Unmap(0, nullptr);
			perFrame.largePages[i].d3d12Resource.Reset();
		}
		perFrame.largePages.clear();

#endif

		perFrame.linearNextByte = 0;
	}

	TempBuffer BufferAllocator::AllocateTempBuffer(uint64_t sizeInBytes, uint32_t alignment)
	{
		if (LogErrorIfNotLoaded()) return TempBuffer();

		PerFrame& current = perFrame.at(frameIndex);

		uint64_t alignedStart = AlignUp(current.linearNextByte, (uint64_t)alignment);
		uint64_t alignedSize = AlignUp(sizeInBytes, (uint64_t)alignment);

		// New large page
		if (alignedSize > pageSize)
		{
			Page large = CreatePage(*context, alignedSize);
			if (large.IsNull())
			{
				Log(LogType::Error, ToString("Failed to allocate a temporary buffer of size ", sizeInBytes, "."));
				return TempBuffer();
			}
			current.largePages.push_back(large);

			TempBuffer out;
			out.offset = 0;
			out.data = large.mapped;
#ifdef IGLO_D3D12
			out.d3d12Resource = large.d3d12Resource.Get();
#endif
			return out;
		}

		// New linear page
		if (alignedStart + alignedSize > pageSize)
		{
			Page page = CreatePage(*context, pageSize);
			if (page.IsNull())
			{
				Log(LogType::Error, ToString("Failed to allocate a temporary buffer of size ", sizeInBytes, "."));
				return TempBuffer();
			}
			current.linearPages.push_back(page);
			current.linearNextByte = sizeInBytes;

			TempBuffer out;
			out.offset = 0;
			out.data = page.mapped;
#ifdef IGLO_D3D12
			out.d3d12Resource = page.d3d12Resource.Get();
#endif
			return out;
		}

		// Use existing linear page
		{
			if (current.linearPages.size() == 0) throw std::runtime_error("This should be impossible.");

			// Move position forward by actual size, not aligned size.
			// Aligning the size is only useful for preventing temporary buffer from extending outside bounds.
			// If code reaches this point, we already know it won't extend too far.
			current.linearNextByte = alignedStart + sizeInBytes;

			TempBuffer out;
			out.offset = alignedStart;
			out.data = (void*)((size_t)current.linearPages.back().mapped + (size_t)alignedStart);
#ifdef IGLO_D3D12
			out.d3d12Resource = current.linearPages.back().d3d12Resource.Get();
#endif
			return out;
		}
	}

	size_t BufferAllocator::GetNumUsedPages() const
	{
		if (!isLoaded) return 0;
		const PerFrame& current = perFrame.at(frameIndex);
		return current.largePages.size() + current.linearPages.size();
	}

	bool IGLOContext::LoadWindow(WindowSettings windowSettings, bool showPopupIfFailed)
	{
		this->windowTitle = windowSettings.title;
		this->windowedPos = IntPoint(0, 0);
		this->windowedSize = Extent2D(windowSettings.width, windowSettings.height);
		this->windowedVisible = windowSettings.visible;
		this->windowedBordersVisible = windowSettings.bordersVisible;
		this->windowedResizable = windowSettings.resizable;

		this->mousePosition = IntPoint(0, 0);
		this->mouseButtonIsDown.Clear();
		this->mouseButtonIsDown.Resize(6, false); // 6 mouse buttons
		this->keyIsDown.Clear();
		this->keyIsDown.Resize(256, false); // There are max 256 different keys

#ifdef _WIN32
		HINSTANCE hInstance = (HINSTANCE)GetModuleHandleW(0);
		HCURSOR ArrowCursor = LoadCursorW(0, IDC_ARROW);

		//Create a window class
		WNDCLASSEXW wc = {};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.hInstance = hInstance;
		wc.lpszClassName = windowClassName;
		wc.lpfnWndProc = WndProc;
		wc.style = windowClassStyle;
		wc.hbrBackground = 0; // Assigning a brush to hbrBackground causes the window to flicker when being dragged, so leave this at 0.
		wc.hCursor = ArrowCursor;

		// Only register the window class if it isn't already registered.
		// Registering an already registered class will result in an error.
		if (!windowClassIsRegistered)
		{
			if (RegisterClassExW(&wc))
			{
				windowClassIsRegistered = true;
			}
			else
			{
				OnFatalError("Failed to create window. Reason: Failed to register window class.", showPopupIfFailed);
				UnloadWindow();
				return false;
			}
		}

		//Create a window
		windowHWND = CreateWindowExW(0, windowClassName, utf8_to_utf16(windowTitle).c_str(), windowStyleWindowed,
			CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0, hInstance, this);
		if (windowHWND)
		{
			numWindows++;
		}
		else
		{
			OnFatalError("Failed to create window.", showPopupIfFailed);
			UnloadWindow();
			return false;
		}

		//windowHDC = GetDC(windowHWND);

		// Get window location
		RECT rect = {};
		if (GetWindowRect(windowHWND, &rect)) windowedPos = IntPoint(rect.left, rect.top);

		// Don't make the window visible yet. Window should be made visible after graphics device has been loaded.
		UpdateWin32Window(windowedPos, windowedSize, windowedResizable, windowedBordersVisible, false, false, false);

		SetCursor(ArrowCursor);
		this->windowIconOwned = 0;
#endif

#ifdef __linux__
		// X11 window creation code from public domain source:
		// https://github.com/gamedevtech/X11OpenGLWindow

		displayHandle = XOpenDisplay(0);
		if (!displayHandle)
		{
			OnFatalError("Failed to create window. Reason: Failed to open display.", showPopupIfFailed);
			UnloadWindow();
			return false;
		}
		screenHandle = DefaultScreenOfDisplay(displayHandle);
		screenId = DefaultScreen(displayHandle);
		x11fileDescriptor = ConnectionNumber(this->displayHandle);

		int depth = 24;
		int visualClass = TrueColor;

		if (!XMatchVisualInfo(displayHandle, screenId, depth, visualClass, &visualInfo))
		{
			OnFatalError("Failed to create window. Reason: No matching visual found.", showPopupIfFailed);
			UnloadWindow();
			return false;
		}

		if (screenId != visualInfo.screen)
		{
			OnFatalError("Failed to create window. Reason: screenId does not match visualInfo.", showPopupIfFailed);
			UnloadWindow();
			return false;
		}

		// Open the window
		windowAttributes.border_pixel = BlackPixel(displayHandle, screenId);
		windowAttributes.background_pixel = BlackPixel(displayHandle, screenId);
		windowAttributes.override_redirect = True;
		windowAttributes.colormap = XCreateColormap(displayHandle, RootWindow(displayHandle, screenId), visualInfo.visual, AllocNone);
		windowAttributes.event_mask = ExposureMask;
		windowHandle = XCreateWindow(displayHandle, RootWindow(displayHandle, screenId), 0, 0, windowedSize.width, windowedSize.height,
			0, visualInfo.depth, InputOutput, visualInfo.visual, CWBackPixel | CWColormap | CWBorderPixel | CWEventMask, &windowAttributes);

		// Redirect Close
		atomWmDeleteWindow = XInternAtom(displayHandle, "WM_DELETE_WINDOW", False);
		XSetWMProtocols(displayHandle, windowHandle, &atomWmDeleteWindow, 1);

		// Add all event callbacks except for 'PointerMotionHintMask', 'ResizeRedirectMask and 
		// 'SubstructureNotifyMask'
		XSelectInput(displayHandle, windowHandle,
			KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
			EnterWindowMask | LeaveWindowMask | PointerMotionMask | 0 |
			Button1MotionMask | Button2MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask | ButtonMotionMask |
			KeymapStateMask | ExposureMask | VisibilityChangeMask | StructureNotifyMask | 0 | 0 | SubstructureRedirectMask |
			FocusChangeMask | PropertyChangeMask | ColormapChangeMask | OwnerGrabButtonMask);

		// Disable fake key release events
		int autoRepeatIsSupported = 0;
		if (!XkbSetDetectableAutoRepeat(displayHandle, 1, &autoRepeatIsSupported))
		{
			Log(LogType::Warning, "Unable to disable X11 auto repeat for keys being held down.");
		}

		XClearWindow(displayHandle, windowHandle);
		XMapRaised(displayHandle, windowHandle);

#endif

		if (windowSettings.centered)
		{
			CenterWindow();
		}

		return true;
	}

	bool IGLOContext::ResizeBackBuffer(Extent2D dimensions, Format format)
	{
		const char* errStr = "Failed to resize backbuffer (or change backbuffer format). Reason: ";

		if (dimensions.width == 0 || dimensions.height == 0)
		{
			Log(LogType::Error, ToString(errStr, "Width and height must be nonzero."));
			return false;
		}

		Extent2D oldSize = this->GetBackBufferSize();

		if (commandQueue.IsLoaded())
		{
			commandQueue.WaitForIdle();
			ClearAllTempResources();
		}

		backBufferSize = dimensions;
		backBufferFormat = format;

		if (isLoaded)
		{
#ifdef IGLO_D3D12
			windowActiveResizing = false;
			windowActiveMenuLoop = false;

			FormatInfoDXGI formatInfoD3D = GetFormatInfoDXGI(format);
			if (formatInfoD3D.dxgiFormat == DXGI_FORMAT_UNKNOWN)
			{
				Log(LogType::Error, ToString(errStr, "This iglo format is not supported in D3D12: ", (int)format, "."));
				return false;
			}

			backBuffers.clear();
			backBuffers.shrink_to_fit();

			HRESULT hr = d3d12SwapChain->ResizeBuffers(0, dimensions.width, dimensions.height, formatInfoD3D.dxgiFormat, swapChainFlags);
			if (FAILED(hr))
			{
				Log(LogType::Error, ToString(errStr, "IDXGISwapChain3::ResizeBuffers returned error code: ", (uint32_t)hr, "."));
				return false;
			}

			for (uint32_t i = 0; i < numBackBuffers; i++)
			{
				ComPtr<ID3D12Resource> res;
				hr = d3d12SwapChain->GetBuffer(i, IID_PPV_ARGS(&res));
				if (FAILED(hr))
				{
					Log(LogType::Error, ToString(errStr, "IDXGISwapChain3::GetBuffer returned error code : ", (uint32_t)hr, "."));
					backBuffers.clear();
					backBuffers.shrink_to_fit();
					return false;
				}
				std::unique_ptr<Texture> currentBackBuffer = std::make_unique<Texture>();
				if (!currentBackBuffer->LoadAsWrapped(*this, { res }, dimensions.width, dimensions.height, format,
					TextureUsage::RenderTexture, MSAA::Disabled, false, 1, 1))
				{
					Log(LogType::Error, ToString(errStr, "Failed to wrap back buffer."));
					backBuffers.clear();
					backBuffers.shrink_to_fit();
					return false;
				}
				backBuffers.push_back(std::move(currentBackBuffer));
			}
#endif
		}

		if (commandQueue.IsLoaded())
		{
			commandQueue.SignalFence(CommandListType::Graphics);
			commandQueue.WaitForIdle();
		}

		frameIndex = 0;

		if (isLoaded)
		{
			// If backbuffer has changed size
			if (oldSize != GetBackBufferSize())
			{
				Event event_out;
				event_out.type = EventType::Resize;
				event_out.resize = dimensions;
				eventQueue.push(event_out);
			}
		}
		return true;
	}

	bool IGLOContext::CreateSwapChain(Extent2D dimensions, Format format, uint32_t bufferCount, std::string& out_errStr)
	{
		out_errStr = "";

#ifdef IGLO_D3D12
		backBuffers.clear();
		backBuffers.shrink_to_fit();

		backBufferSize = dimensions;
		backBufferFormat = format;
		numBackBuffers = bufferCount;

		d3d12SwapChain.Reset();

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = bufferCount;
		swapChainDesc.Width = dimensions.width;
		swapChainDesc.Height = dimensions.height;
		swapChainDesc.Format = GetFormatInfoDXGI(format).dxgiFormat;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SampleDesc.Quality = 0; // Quality level is hardware and driver specific. 0 = MSAA.
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SwapEffect = this->d3d12SwapEffect;
		swapChainDesc.Flags = swapChainFlags;
		ComPtr<IDXGISwapChain1> swapChain1;
		HRESULT hr = d3d12Factory->CreateSwapChainForHwnd(this->commandQueue.GetD3D12CommandQueue(CommandListType::Graphics),
			this->windowHWND, &swapChainDesc, nullptr, nullptr, &swapChain1);
		if (FAILED(hr))
		{
			out_errStr = ToString("ID3D12Device::CreateSwapChainForHwnd returned error code: ", (uint32_t)hr, ".");
			return false;
		}
		hr = swapChain1.As(&this->d3d12SwapChain);
		if (FAILED(hr))
		{
			out_errStr = ToString("ComPtr::As(IDXGISwapChain3) returned error code: ", (uint32_t)hr, ".");
			return false;
		}
		hr = this->d3d12SwapChain->SetMaximumFrameLatency(bufferCount);
		if (FAILED(hr))
		{
			Log(LogType::Warning, "Failed to set swap chain maximum frame latency.");
		}

		// Don't use the default fullscreen mode
		hr = d3d12Factory->MakeWindowAssociation(this->windowHWND, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_PRINT_SCREEN);
		if (FAILED(hr))
		{
			Log(LogType::Warning, "Failed to disable IDXGISwapChain Alt-Enter fullscreen toggle.");
		}

		// Wrap back buffer
		for (uint32_t i = 0; i < bufferCount; i++)
		{
			ComPtr<ID3D12Resource> res;
			hr = this->d3d12SwapChain->GetBuffer(i, IID_PPV_ARGS(&res));
			if (FAILED(hr))
			{
				out_errStr = ToString("IDXGISwapChain3::GetBuffer returned error code : ", (uint32_t)hr, ".");
				backBuffers.clear();
				backBuffers.shrink_to_fit();
				return false;
			}
			std::unique_ptr<Texture> currentBackBuffer = std::make_unique<Texture>();
			if (!currentBackBuffer->LoadAsWrapped(*this, { res }, dimensions.width, dimensions.height,
				format, TextureUsage::RenderTexture, MSAA::Disabled, false, 1, 1))
			{
				out_errStr = ToString("Failed to wrap back buffer.");
				backBuffers.clear();
				backBuffers.shrink_to_fit();
				return false;
			}
			backBuffers.push_back(std::move(currentBackBuffer));
		}

		return true;
#endif
#ifdef IGLO_VULKAN
		return false; //TODO: this
#endif
	}

	void IGLOContext::SetNumBackBuffers(uint32_t numBackBuffers)
	{
		if (numBackBuffers == this->numBackBuffers) return;

		commandQueue.WaitForIdle();
		std::string errMsg = "";
		if (!CreateSwapChain(backBufferSize, backBufferFormat, numBackBuffers, errMsg))
		{
			Log(LogType::Error, ToString("Failed to set number of back buffers to ", numBackBuffers, ". Reason: ", errMsg));
		}
	}

	const Texture& IGLOContext::GetBackBuffer() const
	{
#ifdef IGLO_D3D12
		uint32_t backBufferIndex = d3d12SwapChain->GetCurrentBackBufferIndex();
		return *backBuffers[backBufferIndex];
#endif
#ifdef IGLO_VULKAN
		static Texture tempDummyTexture; //TODO: remove this later
		return tempDummyTexture;//TODO: this
#endif
	}

	MSAA IGLOContext::GetMaxMultiSampleCount(Format textureFormat) const
	{
		if (!isLoaded)
		{
			Log(LogType::Error, "Failed to get max multisample count. Reason: IGLOContext must be loaded first.");
			return MSAA::Disabled;
		}

#ifdef IGLO_D3D12
		DXGI_FORMAT dxgiFormat = GetFormatInfoDXGI(textureFormat).dxgiFormat;

		if (maxD3D12MSAAPerFormat.size() == 0) maxD3D12MSAAPerFormat.resize(256, 0);

		// If the max MSAA has already been retreived for this format, then return it.
		if (maxD3D12MSAAPerFormat.at(dxgiFormat) != 0) return (MSAA)maxD3D12MSAAPerFormat.at(dxgiFormat);

		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS x16 = { dxgiFormat, 16 };
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS x8 = { dxgiFormat, 8 };
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS x4 = { dxgiFormat, 4 };
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS x2 = { dxgiFormat, 2 };

		bool x16_success = SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &x16, sizeof(x16)));
		bool x8_success = SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &x8, sizeof(x8)));
		bool x4_success = SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &x4, sizeof(x4)));
		bool x2_success = SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &x2, sizeof(x2)));

		byte maxMSAA = 1;
		if (x2_success && x2.NumQualityLevels > 0) maxMSAA = 2;
		if (x4_success && x4.NumQualityLevels > 0) maxMSAA = 4;
		if (x8_success && x8.NumQualityLevels > 0) maxMSAA = 8;
		if (x16_success && x16.NumQualityLevels > 0) maxMSAA = 16;

		maxD3D12MSAAPerFormat[dxgiFormat] = maxMSAA;
		return (MSAA)maxMSAA;
#endif
#ifdef IGLO_VULKAN
		return MSAA::Disabled; //TODO: this
#endif
	}

	void IGLOContext::OnFatalError(const std::string& message, bool popupMessage)
	{
		Log(LogType::FatalError, message);
		if (popupMessage) PopupMessage(message, this->windowTitle, this);
	}

	bool IGLOContext::Load(WindowSettings windowSettings, RenderSettings renderSettings, bool showPopupIfFailed)
	{
		if (isLoaded)
		{
			Log(LogType::Warning, "You are attempting to load an already loaded IGLOContext. The existing context will be replaced with a new one.");
		}
		Unload(); // Unload previous context if there is one

		if (LoadWindow(windowSettings, showPopupIfFailed))
		{
			if (LoadGraphicsDevice(renderSettings, showPopupIfFailed, this->windowedSize))
			{
#ifdef _WIN32
				// Show the window after the graphics device has been created
				if (windowedVisible) ShowWindow(windowHWND, SW_NORMAL);
#endif

				// Window and graphics device context has loaded successfully
				isLoaded = true;
				return true;
			}
		}
		// Failed to load atleast one of them.
		Unload(); // Clean up
		return false;
	}

	void IGLOContext::Unload()
	{
		if (commandQueue.IsLoaded()) commandQueue.WaitForIdle();

		isLoaded = false;

		UnloadGraphicsDevice();
		UnloadWindow();
	}

	void IGLOContext::UnloadWindow()
	{
		eventQueue = {};

		windowTitle = "";
		windowMinimizedState = false;
		windowEnableDragAndDrop = false;
		windowedPos = IntPoint();
		windowedSize = Extent2D();
		windowedMinSize = Extent2D();
		windowedMaxSize = Extent2D();
		windowedVisible = false;
		windowedResizable = false;
		windowedBordersVisible = false;

		callbackModalLoop = nullptr;
		callbackModalLoopUserData = nullptr;
		insideModalLoopCallback = false;

		mousePosition = IntPoint();
		mouseButtonIsDown.Clear();
		keyIsDown.Clear();

#ifdef _WIN32
		// Cleanup window
		windowActiveResizing = false;
		windowActiveMenuLoop = false;
		ignoreSizeMsg = false;
		callbackWndProcHook = nullptr;
		if (windowIconOwned)
		{
			DestroyIcon(windowIconOwned);
			windowIconOwned = 0;
		}
		if (windowHWND)
		{
			DestroyWindow(windowHWND);
			windowHWND = 0;
			if (numWindows > 0)
			{
				numWindows--;
				// Only unregister the window class if this was the last existing window.
				if (numWindows == 0 && windowClassIsRegistered)
				{
					UnregisterClassW(windowClassName, 0);
					windowClassIsRegistered = false;
				}
			}
		}
#endif
#ifdef __linux__
		// Cleanup X11
		if (displayHandle)
		{
			if (windowHandle)
			{
				XFreeColormap(displayHandle, windowAttributes.colormap);
				XDestroyWindow(displayHandle, windowHandle);
			}
			XCloseDisplay(displayHandle);
		}
		screenHandle = nullptr;
		displayHandle = nullptr;
		windowHandle = 0;
		screenId = 0;
		x11fileDescriptor = 0;
#endif
	}

	void IGLOContext::UnloadGraphicsDevice()
	{
		backBuffers.clear();
		backBuffers.shrink_to_fit();

		backBufferSize = Extent2D();
		backBufferFormat = Format::None;

		endOfFrame.clear();
		endOfFrame.shrink_to_fit();

		graphicsAPIFullName = "";
		graphicsAPIShortName = "";
		graphicsAPIVendor = "";
		graphicsAPIRenderer = "";

		displayMode = DisplayMode::Windowed;
		presentMode = PresentMode::Vsync;

		numFramesInFlight = 0;
		numBackBuffers = 0;
		frameIndex = 0;

		generateMipmapsPipeline.Unload();
		bilinearClampSampler.Unload();

		descriptorHeap.Unload();
		bufferAllocator.Unload();
		commandQueue.Unload();

		callbackOnDeviceRemoved = nullptr;

#ifdef IGLO_D3D12
		d3d12BindlessRootSignature.Reset();

		d3d12RTVDescriptorHeap.Reset();
		d3d12DSVDescriptorHeap.Reset();
		d3d12UAVDescriptorHeap.Reset();

		d3d12DescriptorSize_RTV = 0;
		d3d12DescriptorSize_DSV = 0;
		d3d12DescriptorSize_Sampler = 0;
		d3d12DescriptorSize_CBV_SRV_UAV = 0;

		maxD3D12MSAAPerFormat.clear();
		maxD3D12MSAAPerFormat.shrink_to_fit();

		d3d12SwapChain.Reset();
		d3d12Device.Reset();
#endif
	}

	bool IGLOContext::LoadGraphicsDevice(RenderSettings renderSettings, bool showPopupIfFailed, Extent2D backBufferSize)
	{
		this->numFramesInFlight = renderSettings.numFramesInFlight;
		this->numBackBuffers = renderSettings.numBackBuffers;
		this->frameIndex = 0;

#ifdef IGLO_D3D12
		HRESULT hr = 0;
		UINT factoryFlags = 0;
		const char* errStr = "Failed to initialize D3D12 device. Reason: ";

#ifdef _DEBUG
		{
			ComPtr<ID3D12Debug> debugInterface;
			hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
			if (SUCCEEDED(hr))
			{
				debugInterface->EnableDebugLayer();
				factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
			}
		}
#endif

		hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&this->d3d12Factory));
		if (FAILED(hr))
		{
			OnFatalError(ToString(errStr, "CreateDXGIFactory2 returned error code: ", (uint32_t)hr, "."), showPopupIfFailed);
			return false;
		}

		ComPtr<IDXGIAdapter1> hardwareAdapter;
		{
			// Look for D3D12 compatible adapter
			bool adapterFound = false;
			for (UINT i = 0; this->d3d12Factory->EnumAdapters1(i, &hardwareAdapter) != DXGI_ERROR_NOT_FOUND; i++)
			{
				DXGI_ADAPTER_DESC1 adapterDesc = {};
				hardwareAdapter->GetDesc1(&adapterDesc);

				// We don't want the software adapter
				if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					continue;
				}

				// If device creation succeeds with this adapter
				if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(), this->d3d12FeatureLevel, _uuidof(ID3D12Device), nullptr)))
				{
					// Get graphics API renderer and vendor
					this->graphicsAPIRenderer = utf16_to_utf8(adapterDesc.Description);
					switch (adapterDesc.VendorId)
					{
					case 4318:
						this->graphicsAPIVendor = "NVIDIA Corporation";
						break;

					case 4098:
					case 4130:
						this->graphicsAPIVendor = "AMD";
						break;

					case 5692:
					case 32902:
					case 32903:
						this->graphicsAPIVendor = "INTEL";
						break;

					default:
						this->graphicsAPIVendor = "Unknown vendor";
						break;
					}

					// Found a compatible adapter
					adapterFound = true;
					break;
				}
			}

			// Couldn't find a compatible adapter
			if (!adapterFound)
			{
				OnFatalError(ToString(errStr, "No D3D12 compatible hardware adaptor found."), showPopupIfFailed);
				return false;
			}
		}

		// ---- Create D3D12 Device ---- //
		{
			ComPtr<ID3D12Device> deviceVer0;
			hr = D3D12CreateDevice(hardwareAdapter.Get(), this->d3d12FeatureLevel, IID_PPV_ARGS(&deviceVer0));
			if (FAILED(hr))
			{
				OnFatalError(ToString(errStr, "D3D12CreateDevice returned error code: ", (uint32_t)hr, "."), showPopupIfFailed);
				return false;
			}
			hr = deviceVer0.As(&this->d3d12Device);
			if (FAILED(hr))
			{
				OnFatalError(ToString(errStr, "ComPtr::As returned error code: ", (uint32_t)hr, "."), showPopupIfFailed);
				return false;
			}
		}

		// specs for D3D_FEATURE_LEVEL_12_0
		this->graphicsAPIShortName = "D3D12";
		this->graphicsAPIFullName = "Direct3D 12.0";
		this->maxTextureSize = 16384;
		this->maxVerticesPerDrawCall = 4294967295;

		this->d3d12DescriptorSize_RTV = this->d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		this->d3d12DescriptorSize_DSV = this->d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		this->d3d12DescriptorSize_Sampler = this->d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		this->d3d12DescriptorSize_CBV_SRV_UAV = this->d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		if (!this->descriptorHeap.Load(*this, renderSettings.maxPersistentResourceDescriptors,
			renderSettings.maxTemporaryResourceDescriptorsPerFrame, renderSettings.maxSamplerDescriptors))
		{
			OnFatalError(ToString(errStr, "Failed to create descriptor heaps."), showPopupIfFailed);
			return false;
		}
		if (!this->bufferAllocator.Load(*this, renderSettings.bufferAllocatorPageSize))
		{
			OnFatalError(ToString(errStr, "Failed to create buffer allocator."), showPopupIfFailed);
			return false;
		}
		if (!this->commandQueue.Load(*this))
		{
			OnFatalError(ToString(errStr, "Failed to create command queues."), showPopupIfFailed);
			return false;
		}
		this->endOfFrame.clear();
		this->endOfFrame.shrink_to_fit();
		this->endOfFrame.resize(renderSettings.numFramesInFlight);

		// ---- Create Swap Chain ---- //
		{
			std::string swapChainErrMsg = "";
			if (!CreateSwapChain(backBufferSize, renderSettings.backBufferFormat,
				renderSettings.numBackBuffers, swapChainErrMsg))
			{
				OnFatalError(ToString(errStr, swapChainErrMsg), showPopupIfFailed);
				return false;
			}
		}

		// ---- Check shader model support ---- //
		{
			D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};
			shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
			bool supportsSM_6_6 = false;
			if (SUCCEEDED(this->d3d12Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
			{
				if (shaderModel.HighestShaderModel == D3D_SHADER_MODEL_6_6)
				{
					supportsSM_6_6 = true;
				}
			}
			if (!supportsSM_6_6)
			{
				OnFatalError("No support for shader model 6.6 detected.\n"
					"Make sure you have the latest graphics drivers installed.",
					showPopupIfFailed);
				return false;
			}
		}

		// ---- Check enhanced barrier support ---- //
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {};
			bool supportsEnhancedBarriers = false;
			if (SUCCEEDED(this->d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12))))
			{
				supportsEnhancedBarriers = options12.EnhancedBarriersSupported;
			}
			if (!supportsEnhancedBarriers)
			{
				OnFatalError("No support for D3D12 enhanced barriers detected.\n"
					"Make sure you have the latest graphics drivers installed.",
					showPopupIfFailed);
				return false;
			}
		}

		// ---- Create Bindless Root Signature ---- //
		{
			D3D12_ROOT_PARAMETER rootConstants = {};
			rootConstants.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			rootConstants.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootConstants.Constants.Num32BitValues = IGLO_PUSH_CONSTANT_TOTAL_BYTE_SIZE / 4;
			rootConstants.Constants.RegisterSpace = 0;
			rootConstants.Constants.ShaderRegister = 0;

			D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
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
			HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error);
			if (FAILED(hr))
			{
				Log(LogType::Error, ToString(errStr, "D3D12SerializeRootSignature returned error code: ", (uint32_t)hr, "."));
				Unload();
				return false;
			}

			hr = this->d3d12Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
				IID_PPV_ARGS(&this->d3d12BindlessRootSignature));
			if (FAILED(hr))
			{
				Log(LogType::Error, ToString(errStr, "CreateRootSignature returned error code : ", (uint32_t)hr, "."));
				Unload();
				return false;
			}
		}

		// ---- Create RTV, DSV and UAV descriptor heaps ---- //
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
			rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvDesc.NumDescriptors = IGLO_MAX_SIMULTANEOUS_RENDER_TARGETS;

			D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
			dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvDesc.NumDescriptors = 1;

			D3D12_DESCRIPTOR_HEAP_DESC uavDesc = {};
			uavDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			uavDesc.NumDescriptors = 1;

			HRESULT hrRTV = this->d3d12Device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&this->d3d12RTVDescriptorHeap));
			HRESULT hrDSV = this->d3d12Device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&this->d3d12DSVDescriptorHeap));
			HRESULT hrUAV = this->d3d12Device->CreateDescriptorHeap(&uavDesc, IID_PPV_ARGS(&this->d3d12UAVDescriptorHeap));

			if (FAILED(hrRTV) || FAILED(hrDSV) || FAILED(hrUAV))
			{
				Log(LogType::Error, ToString(errStr, "Failed to create RTV, DSV and UAV descriptor heaps."));
				Unload();
				return false;
			}
		}

#endif

#ifdef IGLO_VULKAN
		//TODO: this
#endif

		// Load the mipmap generation pipeline
		{
			if (!generateMipmapsPipeline.LoadAsCompute(*this, Shader(g_CS_GenerateMipmaps, sizeof(g_CS_GenerateMipmaps), "CSMain")))
			{
				Log(LogType::Warning, "Failed to load mipmap generation pipeline.");
			}

			SamplerDesc samplerDesc;
			samplerDesc.filter = TextureFilter::Bilinear;
			samplerDesc.wrapU = TextureWrapMode::Clamp;
			samplerDesc.wrapV = TextureWrapMode::Clamp;
			samplerDesc.wrapW = TextureWrapMode::Clamp;
			if (!bilinearClampSampler.Load(*this, samplerDesc))
			{
				Log(LogType::Warning, "Failed to load mipmap generation sampler.");
			}
		}

		SetPresentMode(renderSettings.presentMode);
		return true;
	}

	void CommandQueue::Unload()
	{
#ifdef IGLO_D3D12
		for (uint32_t i = 0; i < 3; i++)
		{
			d3d12CommandQueue[i].Reset();
			d3d12Fence[i].Reset();
			d3d12FenceValue[i] = 0;
		}
		if (fenceEvent) CloseHandle(fenceEvent);
		fenceEvent = nullptr;
#endif

		isLoaded = false;
		context = nullptr;
	}

	bool CommandQueue::Load(const IGLOContext& context)
	{
		Unload();

#ifdef IGLO_D3D12
		// ---- Create Command Queues ---- //
		{
			D3D12_COMMAND_QUEUE_DESC queueDesc[3] = {};

			// Graphics queue
			queueDesc[0].Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			queueDesc[0].Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

			// Compute queue
			queueDesc[1].Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			queueDesc[1].Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

			// Copy queue
			queueDesc[2].Type = D3D12_COMMAND_LIST_TYPE_COPY;
			queueDesc[2].Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

			for (uint32_t i = 0; i < 3; i++)
			{
				if (FAILED(context.GetD3D12Device()->CreateCommandQueue(&queueDesc[i], IID_PPV_ARGS(&this->d3d12CommandQueue[i]))))
				{
					Log(LogType::Error, "Failed to create command queue.");
					Unload();
					return false;
				}

				if (FAILED(context.GetD3D12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->d3d12Fence[i]))))
				{
					Log(LogType::Error, "Failed to create fence for command queue.");
					Unload();
					return false;
				}

				this->d3d12FenceValue[i] = 0;
			}

			this->fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (!this->fenceEvent)
			{
				Log(LogType::Error, "Failed to create fence event for command queue.");
				Unload();
				return false;
			}
		}
#endif

		this->isLoaded = true;
		this->context = &context;
		return true;
	}

	bool CommandQueue::LogErrorIfInvalidCommandListType(CommandListType type) const
	{
		switch (type)
		{
		case CommandListType::Graphics:
			return false;
		case CommandListType::Compute:
			return false;
		case CommandListType::Copy:
			return false;
		default:
			Log(LogType::Error, "Invalid CommandListType in CommandQueue.");
			return true;
		}
	}

	Receipt CommandQueue::SubmitCommands(const CommandList& commandList)
	{
		if (!commandList.IsLoaded())
		{
			Log(LogType::Error, "Failed to submit commands. Reason: The specified command list isn't loaded.");
			return Receipt();
		}
		if (LogErrorIfInvalidCommandListType(commandList.GetCommandListType())) return Receipt();

#ifdef IGLO_D3D12
		ID3D12CommandList* d3d12cmdLists[] = { commandList.GetD3D12GraphicsCommandList() };
		d3d12CommandQueue[(uint32_t)commandList.GetCommandListType()]->ExecuteCommandLists(1, d3d12cmdLists);
#endif

		return SignalFence(commandList.GetCommandListType());
	}

	Receipt CommandQueue::SubmitCommands(const CommandList** commandLists, uint32_t numCommandLists)
	{
		if (!commandLists || numCommandLists == 0)
		{
			Log(LogType::Error, "Failed to submit commands. Reason: No command lists specified.");
			return Receipt();
		}

		CommandListType cmdType = commandLists[0]->GetCommandListType();
		if (LogErrorIfInvalidCommandListType(cmdType)) return Receipt();

		for (uint32_t i = 0; i < numCommandLists; i++)
		{
			if (!commandLists[i]->IsLoaded())
			{
				Log(LogType::Error, "Failed to submit commands. Reason: Atleast one specified command list isn't loaded.");
				return Receipt();
			}
			if (commandLists[i]->GetCommandListType() != cmdType)
			{
				Log(LogType::Error, "Failed to submit commands. Reason: Submitting multiple commands at once requires all"
					" specified command lists to be of the same command list type.");
				return Receipt();
			}
		}

#ifdef IGLO_D3D12
		std::vector<ID3D12CommandList*> d3d12cmdLists;
		d3d12cmdLists.resize(numCommandLists, nullptr);

		for (uint32_t i = 0; i < numCommandLists; i++)
		{
			d3d12cmdLists[i] = commandLists[i]->GetD3D12GraphicsCommandList();
		}

		d3d12CommandQueue[(uint32_t)cmdType]->ExecuteCommandLists(numCommandLists, d3d12cmdLists.data());
#endif

		return SignalFence(cmdType);
	}

	Receipt CommandQueue::SignalFence(CommandListType type)
	{
#ifdef IGLO_D3D12
		std::lock_guard<std::mutex> lockGuard(receiptMutex);
		uint32_t t = (uint32_t)type;
		d3d12FenceValue[t]++;
		d3d12CommandQueue[t]->Signal(d3d12Fence[t].Get(), d3d12FenceValue[t]);
		return { d3d12Fence[t].Get(), d3d12FenceValue[t] };
#endif
#ifdef IGLO_VULKAN
		return Receipt(); //TODO: this
#endif
	}

	bool CommandQueue::IsComplete(Receipt receipt) const
	{
		if (receipt.IsNull()) return true;

#ifdef IGLO_D3D12
		return receipt.d3d12Fence->GetCompletedValue() >= receipt.fenceValue;
#endif
#ifdef IGLO_VULKAN
		return false; //TODO: this
#endif
	}

	bool CommandQueue::IsIdle() const
	{
		for (uint32_t i = 0; i < 3; i++)
		{
#ifdef IGLO_D3D12
			if (!IsComplete({ d3d12Fence[i].Get(), d3d12FenceValue[i] })) return false;
#endif
		}
		return true;
	}

	void CommandQueue::WaitForCompletion(Receipt receipt)
	{
		if (receipt.IsNull()) return;
		if (IsComplete(receipt)) return;

#ifdef IGLO_D3D12
		{
			std::lock_guard<std::mutex> lockGuard(waitMutex);
			receipt.d3d12Fence->SetEventOnCompletion(receipt.fenceValue, fenceEvent);
			WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
		}
#endif
	}

	void CommandQueue::WaitForIdle()
	{
		for (uint32_t i = 0; i < 3; i++)
		{
#ifdef IGLO_D3D12
			WaitForCompletion({ d3d12Fence[i].Get(), d3d12FenceValue[i] });
#endif
		}
	}

	void Image::Unload()
	{
		if (mustFreeSTBI && pixelsPtr) // Release stb_image data
		{
			stbi_image_free((void*)pixelsPtr);
		}
		mustFreeSTBI = false;
		pixelsPtr = nullptr;

		ownedBuffer.clear();
		ownedBuffer.shrink_to_fit();

		size = 0;
		width = 0;
		height = 0;
		mipLevels = 0;
		numFaces = 0;
		format = Format::None;
		arrangement = Arrangement::CompleteMipChains;
		isCubemap = false;
	}

	Image::Image(Image&& other) noexcept
	{
		*this = std::move(other);
	}
	Image& Image::operator=(Image&& other) noexcept
	{
		Unload();
		std::swap(this->size, other.size);
		std::swap(this->width, other.width);
		std::swap(this->height, other.height);
		std::swap(this->mipLevels, other.mipLevels);
		std::swap(this->numFaces, other.numFaces);
		std::swap(this->format, other.format);
		std::swap(this->arrangement, other.arrangement);
		std::swap(this->isCubemap, other.isCubemap);

		std::swap(this->pixelsPtr, other.pixelsPtr);
		std::swap(this->mustFreeSTBI, other.mustFreeSTBI);

		this->ownedBuffer.swap(other.ownedBuffer);

		return *this;
	}

	bool Image::LogErrorIfNotValid(uint32_t width, uint32_t height, Format format, uint32_t mipLevels, uint32_t numFaces, bool isCubemap)
	{
		if (width == 0 || height == 0)
		{
			Log(LogType::Error, "Failed to create image. Reason: Width and height must be nonzero.");
			return false;
		}
		if (format == Format::None)
		{
			Log(LogType::Error, "Failed to create image. Reason: Bad format.");
			return false;
		}
		if (mipLevels < 1 || numFaces < 1)
		{
			Log(LogType::Error, "Failed to create image. Reason: mipLevels and numFaces must be 1 or higher.");
			return false;
		}
		if (isCubemap && (numFaces % 6 != 0))
		{
			Log(LogType::Error, "Failed to create cubemap image. Reason: The number of faces in a cubemap image must be a multiple of 6.");
			return false;
		}
		FormatInfo info = GetFormatInfo(format);
		if (info.blockSize > 0)
		{
			bool widthIsMultipleOf4 = (width % 4 == 0);
			bool heightIsMultipleOf4 = (height % 4 == 0);
			if (!widthIsMultipleOf4 || !heightIsMultipleOf4)
			{
				Log(LogType::Error, "Failed to create image."
					" Reason: Width and height must be multiples of 4 when using a block compression format.");
				return false;
			}
		}
		return true;
	}

	bool Image::Load(uint32_t width, uint32_t height, Format format,
		uint32_t mipLevels, uint32_t numFaces, bool isCubemap, Arrangement arrangement)
	{
		Unload();
		if (!LogErrorIfNotValid(width, height, format, mipLevels, numFaces, isCubemap)) return false;
		this->width = width;
		this->height = height;
		this->format = format;
		this->mipLevels = mipLevels;
		this->numFaces = numFaces;
		this->arrangement = arrangement;
		this->isCubemap = isCubemap;
		this->mustFreeSTBI = false;

		this->size = CalculateTotalSize(mipLevels, numFaces, width, height, format);
		this->ownedBuffer.resize(this->size, 0);
		this->pixelsPtr = this->ownedBuffer.data();
		return true;
	}

	bool Image::LoadAsPointer(const void* pixels, uint32_t width, uint32_t height, Format format, uint32_t mipLevels, uint32_t numFaces,
		bool isCubemap, Arrangement arrangement)
	{
		Unload();
		if (!LogErrorIfNotValid(width, height, format, mipLevels, numFaces, isCubemap)) return false;
		this->width = width;
		this->height = height;
		this->format = format;
		this->mipLevels = mipLevels;
		this->numFaces = numFaces;
		this->arrangement = arrangement;
		this->isCubemap = isCubemap;
		this->mustFreeSTBI = false;

		this->size = CalculateTotalSize(mipLevels, numFaces, width, height, format);
		this->ownedBuffer.clear();
		this->ownedBuffer.shrink_to_fit();
		this->pixelsPtr = (byte*)pixels; // Store pointer
		return true;
	}

	size_t Image::GetMipSize(uint32_t mipIndex) const
	{
		return CalculateMipSize(mipIndex, width, height, format);
	}

	uint32_t Image::GetMipRowPitch(uint32_t mipIndex) const
	{
		return CalculateMipRowPitch(mipIndex, width, height, format);
	}

	Extent2D Image::GetMipExtent(uint32_t mipIndex) const
	{
		return CalculateMipExtent(mipIndex, width, height);
	}

	size_t Image::CalculateTotalSize(uint32_t mipLevels, uint32_t numFaces, uint32_t width, uint32_t height, Format format)
	{
		size_t totalSize = 0;
		for (uint32_t i = 0; i < mipLevels; i++)
		{
			totalSize += CalculateMipSize(i, width, height, format);
		}
		return totalSize * numFaces;
	}
	size_t Image::CalculateMipSize(uint32_t mipIndex, uint32_t width, uint32_t height, Format format)
	{
		FormatInfo info = GetFormatInfo(format);
		uint32_t mipWidth = std::max(uint32_t(1), width >> mipIndex);
		uint32_t mipHeight = std::max(uint32_t(1), height >> mipIndex);
		if (info.blockSize > 0) // Block-compressed format
		{
			return (size_t)std::max(uint32_t(1), ((mipWidth + 3) / 4)) * (size_t)std::max(uint32_t(1), ((mipHeight + 3) / 4)) * info.blockSize;
		}
		else
		{
			return std::max(uint32_t(1), mipWidth * mipHeight * info.bytesPerPixel);
		}
	}

	uint32_t Image::CalculateMipRowPitch(uint32_t mipIndex, uint32_t width, uint32_t height, Format format)
	{
		FormatInfo info = GetFormatInfo(format);
		uint32_t mipWidth = std::max(uint32_t(1), width >> mipIndex);
		if (info.blockSize > 0) // Block-compressed format
		{
			return std::max(uint32_t(1), ((mipWidth + 3) / 4)) * info.blockSize;
		}
		else
		{
			return std::max(uint32_t(1), mipWidth * info.bytesPerPixel);
		}
	}

	Extent2D Image::CalculateMipExtent(uint32_t mipIndex, uint32_t width, uint32_t height)
	{
		Extent2D mipExtent;
		mipExtent.width = std::max(uint32_t(1), width >> mipIndex);
		mipExtent.height = std::max(uint32_t(1), height >> mipIndex);
		return mipExtent;
	}

	uint32_t Image::CalculateNumMips(uint32_t width, uint32_t height)
	{
		uint32_t numLevels = 1;
		while (width > 1 && height > 1)
		{
			width = std::max(width / 2, (uint32_t)1);
			height = std::max(height / 2, (uint32_t)1);
			numLevels++;
		}
		return numLevels;
	}

	void* Image::GetMipPixels(uint32_t faceIndex, uint32_t mipIndex) const
	{
		if (!IsLoaded()) return nullptr;

		byte* out = pixelsPtr;
		if (arrangement == Arrangement::CompleteMipChains)
		{
			for (uint32_t f = 0; f < numFaces; f++)
			{
				for (uint32_t m = 0; m < mipLevels; m++)
				{
					if (f == faceIndex && m == mipIndex) return out;
					out += GetMipSize(m);
				}
			}
		}
		else if (arrangement == Arrangement::MipStrips)
		{
			for (uint32_t m = 0; m < mipLevels; m++)
			{
				for (uint32_t f = 0; f < numFaces; f++)
				{
					if (f == faceIndex && m == mipIndex) return out;
					out += GetMipSize(m);
				}
			}
		}
		return (void*)out;
	}

	bool Image::LoadFromFile(const std::string& filename)
	{
		Unload();
		ReadFileResult file = ReadFile(filename);
		if (!file.success)
		{
			Log(LogType::Error, "Failed to load image from file. Reason: Couldn't open '" + filename + "'.");
			return false;
		}
		if (file.fileContent.size() == 0)
		{
			Log(LogType::Error, "Failed to load image from file. Reason: File '" + filename + "' is empty.");
			return false;
		}
		if (LoadFromMemory(file.fileContent.data(), file.fileContent.size(), false))
		{
			bool ownsThePixelData = false;
			if (this->ownedBuffer.size() > 0) ownsThePixelData = true;
			if (this->mustFreeSTBI) ownsThePixelData = true;

			// The file data will be stored in this image if image depends on it for its pixel data.
			if (!ownsThePixelData)
			{
				this->ownedBuffer.swap(file.fileContent);
			}
			return true;
		}
		else
		{
			Unload();
			return false;
		}
	}

	bool Image::FileIsOfTypeDDS(const byte* fileData, size_t numBytes)
	{
		if (numBytes < 8) return false;
		uint32_t* f32 = (uint32_t*)fileData;
		uint8_t* f8 = (uint8_t*)fileData;
		if (f8[0] != 'D') return false;
		if (f8[1] != 'D') return false;
		if (f8[2] != 'S') return false;
		if (f8[3] != ' ') return false;
		if (f32[1] != 124) return false;
		return true;
	}

	// This struct was taken from public domain source SOIL: https://github.com/littlstar/soil/blob/master/include/SOIL/stbi_dds_aug_c.h
	struct DDS_header
	{
		uint32_t    dwMagic;
		uint32_t    dwSize;
		uint32_t    dwFlags;
		uint32_t    dwHeight;
		uint32_t    dwWidth;
		uint32_t    dwPitchOrLinearSize;
		uint32_t    dwDepth;
		uint32_t    dwMipMapCount;
		uint32_t    dwReserved1[11];
		struct // DDPIXELFORMAT
		{
			uint32_t    dwSize;
			uint32_t    dwFlags;
			uint32_t    dwFourCC;
			uint32_t    dwRGBBitCount;
			uint32_t    dwRBitMask;
			uint32_t    dwGBitMask;
			uint32_t    dwBBitMask;
			uint32_t    dwAlphaBitMask;
		}sPixelFormat;
		struct // DDCAPS2
		{
			uint32_t    dwCaps1;
			uint32_t    dwCaps2;
			uint32_t    dwDDSX;
			uint32_t    dwReserved;
		} sCaps;
		uint32_t dwReserved2;
	};

	//	the following constants were copied directly off the MSDN website
	//	The dwFlags member of the original DDSURFACEDESC2 structure
	//	can be set to one or more of the following values.
#define DDSD_CAPS			0x00000001
#define DDSD_HEIGHT			0x00000002
#define DDSD_WIDTH			0x00000004
#define DDSD_PITCH			0x00000008
#define DDSD_PIXELFORMAT	0x00001000
#define DDSD_MIPMAPCOUNT	0x00020000
#define DDSD_LINEARSIZE		0x00080000
#define DDSD_DEPTH			0x00800000

//	DirectDraw Pixel Format
#define DDPF_ALPHAPIXELS	0x00000001
#define DDPF_FOURCC			0x00000004
#define DDPF_RGB			0x00000040

//	The dwCaps1 member of the DDSCAPS2 structure can be
//	set to one or more of the following values.
#define DDSCAPS_COMPLEX	0x00000008
#define DDSCAPS_TEXTURE	0x00001000
#define DDSCAPS_MIPMAP	0x00400000

//	The dwCaps2 member of the DDSCAPS2 structure can be
//	set to one or more of the following values.
#define DDSCAPS2_CUBEMAP			0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX	0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX	0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY	0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY	0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ	0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ	0x00008000
#define DDSCAPS2_VOLUME				0x00200000

	// Code in this function is taken from public domain source SOIL: https://github.com/littlstar/soil/blob/master/src/soil.c
	bool Image::LoadFromDDS(const byte* fileData, size_t numBytes, bool forceCopy)
	{
		Unload();

		const char* errStr = "Failed to load image from file data. Reason: ";

		if (numBytes < sizeof(DDS_header))
		{
			Log(LogType::Error, ToString(errStr, "The DDS file is corrupted (too small for expected header)."));
			return false;
		}
		DDS_header* header = (DDS_header*)fileData;
		size_t buffer_index = 0;
		buffer_index = sizeof(DDS_header);

		bool failed = false;

		uint32_t flag = ('D' << 0) | ('D' << 8) | ('S' << 16) | (' ' << 24);
		if (header->dwMagic != flag) failed = true;
		if (header->dwSize != 124) failed = true;
		flag = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
		if ((header->dwFlags & flag) != flag) failed = true;
		if ((header->sCaps.dwCaps1 & DDSCAPS_TEXTURE) == 0) failed = true;
		/*	make sure it is a type we can upload	*/
		if ((header->sPixelFormat.dwFlags & DDPF_FOURCC) &&
			!(
				(header->sPixelFormat.dwFourCC == (('D' << 0) | ('X' << 8) | ('T' << 16) | ('1' << 24))) ||
				(header->sPixelFormat.dwFourCC == (('D' << 0) | ('X' << 8) | ('T' << 16) | ('3' << 24))) ||
				(header->sPixelFormat.dwFourCC == (('D' << 0) | ('X' << 8) | ('T' << 16) | ('5' << 24)))))
		{
			failed = true;
		}
		if (failed)
		{
			Log(LogType::Error, ToString(errStr, "The DDS file is either corrupted or unsupported."));
			return false;
		}

		/*	OK, validated the header, let's load the image data	*/
		uint32_t tempWidth = header->dwWidth;
		uint32_t tempHeight = header->dwHeight;
		uint32_t tempMipLevel = 1;
		Format tempFormat = Format::None;
		uint32_t tempNumFaces = 1;
		bool tempIsCubemap = false;
		bool mustConvertBGRToBGRA = false;
		Arrangement tempArrangement = Arrangement::CompleteMipChains;

		int32_t cubemap = (header->sCaps.dwCaps2 & DDSCAPS2_CUBEMAP) / DDSCAPS2_CUBEMAP;
		if (cubemap)
		{
			tempIsCubemap = true;
			tempNumFaces = 6; // A cubemap has 6 faces
		}

		int32_t volume = (header->sCaps.dwCaps2 & DDSCAPS2_VOLUME) / DDSCAPS2_VOLUME;
		if (volume)
		{
			Log(LogType::Error, ToString(errStr, "The DDS file contains an unsupported format (3D volume texture)."));
			return false;
		}

		int32_t uncompressed = 1 - (header->sPixelFormat.dwFlags & DDPF_FOURCC) / DDPF_FOURCC;
		if (uncompressed)
		{
			if (header->sPixelFormat.dwRGBBitCount == 32)
			{
				// 32 bits per pixel BGRA or BGRX.
				// BGRX is compatible with BGRA, so just use BGRA for both
				tempFormat = Format::BYTE_BYTE_BYTE_BYTE_BGRA;
				if (header->sPixelFormat.dwRBitMask == 0x000000ff)
				{
					tempFormat = Format::BYTE_BYTE_BYTE_BYTE; // Order is RGBA, not BGRA
				}
			}
			else if (header->sPixelFormat.dwRGBBitCount == 24)
			{
				// 24 bits per pixel BGR. Must be converted to BGRA.
				mustConvertBGRToBGRA = true;
				tempFormat = Format::BYTE_BYTE_BYTE_BYTE_BGRA;
			}
			else if (header->sPixelFormat.dwRGBBitCount == 16)
			{
				Log(LogType::Error, ToString(errStr, "The DDS file contains an unsupported format (16-bits per pixel uncompressed)."));
				return false;
			}
			else if (header->sPixelFormat.dwRGBBitCount == 8)
			{
				tempFormat = Format::BYTE;
			}
			else
			{
				Log(LogType::Error, ToString(errStr, "The DDS file contains an unknown format."));
				return false;
			}
		}
		else
		{
			/*	well, we know it is DXT1/3/5, because we checked above	*/
			switch ((header->sPixelFormat.dwFourCC >> 24) - '0')
			{
				// DXT1 uses BC1.
				// DXT2 and DXT3 both use BC2. DXT2 uses premultiplied alpha, DXT3 uses regular alpha.
				// DXT4 and DXT5 both use BC3. DXT4 uses premultiplied alpha, DXT5 uses regular alpha.

			case 1: tempFormat = Format::BC1; break;
			case 3: tempFormat = Format::BC2; break;
			case 5: tempFormat = Format::BC3; break;
			}
		}

		// If mipmaps exist
		if ((header->sCaps.dwCaps1 & DDSCAPS_MIPMAP) && (header->dwMipMapCount > 1))
		{
			tempMipLevel = header->dwMipMapCount;
		}

		byte* ddsPixels = (byte*)&fileData[buffer_index];
		failed = false;
		if (mustConvertBGRToBGRA || forceCopy)
		{
			// If we must convert to different format, then this image must create its own pixel buffer first
			if (!Load(tempWidth, tempHeight, tempFormat, tempMipLevel, tempNumFaces, tempIsCubemap, tempArrangement))
			{
				failed = true;
			}
		}
		else
		{
			// Store pointer to existing pixel data
			if (!LoadAsPointer(ddsPixels, tempWidth, tempHeight, tempFormat, tempMipLevel, tempNumFaces, tempIsCubemap, tempArrangement))
			{
				failed = true;
			}
		}
		if (failed)
		{
			Log(LogType::Error, ToString(errStr, "Unable to create the image. This error shouldn't be able to happen."));
			return false;
		}

		size_t expected = this->size;
		if (mustConvertBGRToBGRA)
		{
			expected -= (expected / 4); // BGRA -> BGR is 1/4 smaller
		}
		size_t remaining = numBytes - buffer_index;
		if (remaining < expected)
		{
			Unload();
			Log(LogType::Error, ToString(errStr, "The DDS file is corrupted (it is smaller than expected)."));
			return false;
		}
		else if (remaining > expected)
		{
			Log(LogType::Warning, "DDS file is larger than expected, some contents of the DDS file may not have been loaded properly.");
		}

		if (mustConvertBGRToBGRA)
		{
			size_t i = 0;
			for (size_t j = 0; j < this->size; j += 4)
			{
				this->pixelsPtr[j] = ddsPixels[i];
				this->pixelsPtr[j + 1] = ddsPixels[i + 1];
				this->pixelsPtr[j + 2] = ddsPixels[i + 2];
				this->pixelsPtr[j + 3] = 255; // Add an alpha channel to RGB pixels
				i += 3;
			}
		}
		else
		{
			if (forceCopy)
			{
				// Copy pixel data from DDS file to this image
				memcpy(this->pixelsPtr, ddsPixels, this->size);
			}
		}
		return true;
	}

	bool Image::FileIsOfTypeKTX(const byte* fileData, size_t numBytes)
	{
		const uint32_t identifierSize = 12;
		byte identifier[identifierSize] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };
		if (numBytes < identifierSize) return false;
		uint8_t* f8 = (uint8_t*)fileData;
		for (uint32_t i = 0; i < identifierSize; i++)
		{
			if (f8[i] != identifier[i]) return false;
		}
		return true;
	}
	bool Image::LoadFromKTX(const byte* fileData, size_t numBytes, bool forceCopy)
	{
		Unload();

		uint32_t tempWidth = 0;
		uint32_t tempHeight = 0;
		uint32_t tempMipLevel = 1;
		Format tempFormat = Format::None;
		uint32_t tempNumFaces = 1;
		bool tempIsCubemap = false;
		bool mustConvertBGRToBGRA = false;
		Arrangement tempArrangement = Arrangement::MipStrips;


		//TODO: Implement this function.


		Log(LogType::Error, "Failed to load image from file data. Reason: Support for KTX file format is not yet implemented.");
		return false;
	}

	bool Image::LoadFromMemory(const byte* fileData, size_t numBytes, bool forceCopy)
	{
		Unload();

		const char* errStr = "Failed to load image from file data. Reason: ";

		if (numBytes == 0 || fileData == nullptr)
		{
			Log(LogType::Error, ToString(errStr, "No data provided."));
			return false;
		}

		if (FileIsOfTypeDDS(fileData, numBytes)) // .DDS
		{
			return LoadFromDDS(fileData, numBytes, forceCopy);
		}

		if (FileIsOfTypeKTX(fileData, numBytes)) // .KTX
		{
			return LoadFromKTX(fileData, numBytes, forceCopy);
		}

		int pixelType = 0;
		if (stbi_is_hdr_from_memory(fileData, (int)numBytes)) // .HDR
		{
			pixelType = 2; // FLOAT (32-bit)
		}
		else if (stbi_is_16_bit_from_memory(fileData, (int)numBytes)) // .PNG or .PSD
		{
			pixelType = 1; // UINT16 (16-bit)
		}
		else
		{
			pixelType = 0; // BYTE (8-bit)
		}
		int w = 0, h = 0, channels = 0;
		if (!stbi_info_from_memory(fileData, (int)numBytes, &w, &h, &channels)) // .JPEG .PNG .TGA .BMP .PSD .GIF .PIC .PNM
		{
			const char* stbErrorStr = stbi_failure_reason();
			Log(LogType::Error, ToString(errStr, "stb_image returned error: ", stbErrorStr, "."));
			return false;
		}
		int forceChannels = 0;
		// Graphics API's do not like triple channel texture formats. Force 4 channels if 3 channels detected.
		if (channels == 3) forceChannels = 4;
		byte* stbiPixels = nullptr;
		if (pixelType == 0) // 8 bits per color channel
		{
			stbiPixels = stbi_load_from_memory(fileData, (int)numBytes, &w, &h, &channels, forceChannels);
		}
		else if (pixelType == 1) // 16 bits per color channel
		{
			stbiPixels = (byte*)stbi_load_16_from_memory(fileData, (int)numBytes, &w, &h, &channels, forceChannels);
		}
		else if (pixelType == 2) // 32 bits per color channel
		{
			stbiPixels = (byte*)stbi_loadf_from_memory(fileData, (int)numBytes, &w, &h, &channels, forceChannels);
		}
		if (forceChannels != 0) channels = forceChannels;
		if (!stbiPixels)
		{
			const char* stbErrorStr = stbi_failure_reason();
			Log(LogType::Error, ToString(errStr, "stb_image returned error: ", stbErrorStr, "."));
			return false;
		}
		Format tempFormat = Format::None;
		if (channels == 0)
		{
			Log(LogType::Error, ToString(errStr, "0 color channels detected."));
			stbi_image_free(stbiPixels);
			return false;
		}
		else if (channels == 1)
		{
			if (pixelType == 0) tempFormat = Format::BYTE;
			else if (pixelType == 1) tempFormat = Format::UINT16;
			else if (pixelType == 2) tempFormat = Format::FLOAT;
		}
		else if (channels == 2)
		{
			if (pixelType == 0) tempFormat = Format::BYTE_BYTE;
			else if (pixelType == 1) tempFormat = Format::UINT16_UINT16;
			else if (pixelType == 2) tempFormat = Format::FLOAT_FLOAT;
		}
		else if (channels == 3)
		{
			Log(LogType::Error, ToString(errStr, "stb_image failed forcing 4 color channels for triple channel texture."
				" This error shouldn't be able to happen."));
			stbi_image_free(stbiPixels);
			return false;
		}
		else if (channels == 4)
		{
			if (pixelType == 0) tempFormat = Format::BYTE_BYTE_BYTE_BYTE;
			else if (pixelType == 1) tempFormat = Format::UINT16_UINT16_UINT16_UINT16;
			else if (pixelType == 2) tempFormat = Format::FLOAT_FLOAT_FLOAT_FLOAT;
		}
		else
		{
			Log(LogType::Error, ToString(errStr, "Color channels must not exceed 4. Color channels detected: ", channels, "."));
			stbi_image_free(stbiPixels);
			return false;
		}
		if (tempFormat == Format::None)
		{
			Log(LogType::Error, ToString(errStr, "Unable to figure out which iglo format to use. This shouldn't be able to happen."));
			stbi_image_free(stbiPixels);
			return false;
		}

		// Store a pointer to the pixel data stb_image created
		if (!LoadAsPointer(stbiPixels, w, h, tempFormat, 1, 1, false))
		{
			Log(LogType::Error, ToString(errStr, "Unable to create the image. This error shouldn't be able to happen."));
			stbi_image_free(stbiPixels);
			return false;
		}

		// At this point, 'pixelsPtr' points to the pixel data stb_image created.
		// It must be freed with stbi_image_free() in Unload() later.
		this->mustFreeSTBI = true;

		return true;
	}

	bool Image::SaveToFile(const std::string& filename) const
	{
		if (!IsLoaded())
		{
			Log(LogType::Error, "Failed to save image to file '" + filename + "'. Reason: Image is not loaded with any data.");
			return false;
		}
		if (filename.size() == 0)
		{
			Log(LogType::Error, "Failed to save image to file. Reason: No filename provided.");
			return false;
		}
		FormatInfo info = GetFormatInfo(format);
		if (info.blockSize > 0)
		{
			Log(LogType::Error, "Failed to save image to file '" + filename +
				"'. Reason: Image uses block-compression format, which is not supported being saved to file.");
			return false;
		}

		std::string fileExt = ToLowercase(GetFileExtension(filename));
		bool success = false;
		bool validExtension = false;
		byte* tempPixels = pixelsPtr;

		std::vector<byte> tempPixelBuffer;
		if (format == Format::BYTE_BYTE_BYTE_BYTE_BGRA ||
			format == Format::BYTE_BYTE_BYTE_BYTE_BGRA_sRGB) // BGRA must be converted to RGBA
		{
			tempPixelBuffer.resize(size);
			tempPixels = tempPixelBuffer.data();
			Color32* colorDest = (Color32*)tempPixelBuffer.data();
			Color32* colorSrc = (Color32*)pixelsPtr;
			for (size_t i = 0; i < size / 4; i++)
			{
				colorDest[i].red = colorSrc[i].blue; // Swap blue and red colors
				colorDest[i].green = colorSrc[i].green;
				colorDest[i].blue = colorSrc[i].red; // Swap blue and red colors
				colorDest[i].alpha = colorSrc[i].alpha;
			}
		}

		switch (format)
		{
		case Format::BYTE:
		case Format::BYTE_NotNormalized:
		case Format::BYTE_BYTE:
		case Format::BYTE_BYTE_NotNormalized:
		case Format::BYTE_BYTE_BYTE_BYTE:
		case Format::BYTE_BYTE_BYTE_BYTE_sRGB:
		case Format::BYTE_BYTE_BYTE_BYTE_NotNormalized:
		case Format::BYTE_BYTE_BYTE_BYTE_BGRA:
		case Format::BYTE_BYTE_BYTE_BYTE_BGRA_sRGB:
			if (fileExt == ".png")
			{
				success = (bool)stbi_write_png(filename.c_str(), width, height, info.elementCount, (byte*)tempPixels, 0);
				validExtension = true;
			}
			else if (fileExt == ".bmp")
			{
				success = (bool)stbi_write_bmp(filename.c_str(), width, height, info.elementCount, (byte*)tempPixels);
				validExtension = true;
			}
			else if (fileExt == ".tga")
			{
				success = (bool)stbi_write_tga(filename.c_str(), width, height, info.elementCount, (byte*)tempPixels);
				validExtension = true;
			}
			else if (fileExt == ".jpg" || fileExt == ".jpeg")
			{
				success = (bool)stbi_write_jpg(filename.c_str(), width, height, info.elementCount, (byte*)tempPixels, 90);
				validExtension = true;
			}
			break;
		case Format::FLOAT_FLOAT:
		case Format::FLOAT_FLOAT_FLOAT:
		case Format::FLOAT_FLOAT_FLOAT_FLOAT:
			if (fileExt == ".hdr")
			{
				success = (bool)stbi_write_hdr(filename.c_str(), width, height, info.elementCount, (float*)tempPixels);
				validExtension = true;
			}
			break;
		default:
			break;
		}

		if (success)
		{
			return true;
		}
		if (!validExtension)
		{
			Log(LogType::Error, "Failed to save image to file. Reason: Unsupported format or file extension.");
			return false;
		}
		else
		{
			Log(LogType::Error, "Failed to save image to file. Reason: stbi failed to write image.");
			return false;
		}
	}

	void Image::SetSRGB(bool sRGB)
	{
		// If already using requested format, do nothing
		if (IsSRGB() == sRGB) return;

		FormatInfo info = GetFormatInfo(format);
		// If sRGB opposite format is found, use it
		if (info.sRGB_opposite != Format::None)
		{
			format = info.sRGB_opposite;
		}
	}
	bool Image::IsSRGB() const
	{
		return GetFormatInfo(format).is_sRGB;
	}

	void Image::ReplaceColors(Color32 colorA, Color32 colorB)
	{
		if (!IsLoaded()) return;

		if (format == ig::Format::BYTE_BYTE_BYTE_BYTE ||
			format == Format::BYTE_BYTE_BYTE_BYTE_NotNormalized ||
			format == Format::BYTE_BYTE_BYTE_BYTE_sRGB ||
			format == ig::Format::BYTE_BYTE_BYTE_BYTE_BGRA ||
			format == Format::BYTE_BYTE_BYTE_BYTE_BGRA_sRGB)
		{
			if (colorA == colorB)
			{
				return; // nothing needs to be done
			}

			uint32_t findValue = colorA.rgba;
			uint32_t replaceValue = colorB.rgba;

			// Convert colorA and colorB from RGBA to BGRA
			if (format == ig::Format::BYTE_BYTE_BYTE_BYTE_BGRA ||
				format == Format::BYTE_BYTE_BYTE_BYTE_BGRA_sRGB)
			{
				findValue = Color32(colorA.blue, colorA.green, colorA.red, colorA.alpha).rgba;
				replaceValue = Color32(colorB.blue, colorB.green, colorB.red, colorB.alpha).rgba;
			}
			uint32_t* p32 = (uint32_t*)pixelsPtr;
			size_t pixelCount = this->size / 4;
			for (size_t i = 0; i < pixelCount; i++)
			{
				if (p32[i] == findValue) p32[i] = replaceValue;
			}
		}
		else
		{
			Log(LogType::Warning, "Failed to replace colors in image. Reason: Incompatible format.");
		}
	}

	void IGLOContext::SetModalLoopCallback(CallbackModalLoop callbackModalLoop, void* callbackModalLoopUserData)
	{
		if (!isLoaded)
		{
			Log(LogType::Error, "Failed to set modal loop callback. Reason: IGLOContext must be loaded!");
			return;
		}

		this->callbackModalLoop = callbackModalLoop;
		this->callbackModalLoopUserData = callbackModalLoopUserData;
	}
	void IGLOContext::GetModalLoopCallback(CallbackModalLoop& out_callbackModalLoop, void*& out_callbackModalLoopUserData)
	{
		out_callbackModalLoop = this->callbackModalLoop;
		out_callbackModalLoopUserData = this->callbackModalLoopUserData;
	}

	void IGLOContext::SetOnDeviceRemovedCallback(CallbackOnDeviceRemoved callbackOnDeviceRemoved)
	{
		if (!isLoaded)
		{
			Log(LogType::Error, "Failed to set OnDeviceRemoved callback. Reason: IGLOContext must be loaded!");
			return;
		}

		this->callbackOnDeviceRemoved = callbackOnDeviceRemoved;
	}

	bool IGLOContext::GetEvent(Event& out_event)
	{
#ifdef _WIN32
		if (this->windowHWND && !this->insideModalLoopCallback)
		{
			MSG msg;
			while (PeekMessage(&msg, this->windowHWND, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
#endif
#ifdef __linux__
		if (this->displayHandle)
		{
			while (XPending(this->displayHandle) > 0)
			{
				XEvent e;
				XNextEvent(this->displayHandle, &e);
				PrivateLinuxProc(e);
			}
		}
#endif
		if (this->eventQueue.empty())
		{
			return false;
		}
		else
		{
			out_event = this->eventQueue.front();
			this->eventQueue.pop();
			return true;
		}
	}

	bool IGLOContext::WaitForEvent(Event& out_event, uint32_t timeoutMilliseconds)
	{
		if (this->eventQueue.empty())
		{
#ifdef _WIN32
			if (this->windowHWND && !this->insideModalLoopCallback)
			{
				if (MsgWaitForMultipleObjects(0, NULL, FALSE, timeoutMilliseconds, QS_ALLINPUT) == WAIT_OBJECT_0)
				{
					MSG msg;
					while (PeekMessage(&msg, this->windowHWND, 0, 0, PM_REMOVE))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
			}
#endif
#ifdef __linux__
			if (this->displayHandle)
			{
				fd_set fileDescSet;
				timeval duration;
				FD_ZERO(&fileDescSet);
				FD_SET(x11fileDescriptor, &fileDescSet);
				duration.tv_usec = timeoutMilliseconds * 1000; // Timer microseconds
				duration.tv_sec = 0; // Timer seconds

				// Waits until either the timer fires or an event is pending, whichever comes first.
				select(x11fileDescriptor + 1, &fileDescSet, NULL, NULL, &duration);

				while (XPending(this->displayHandle) > 0)
				{
					XEvent e;
					XNextEvent(this->displayHandle, &e);
					PrivateLinuxProc(e);
				}
			}
#endif
		}
		if (this->eventQueue.empty())
		{
			return false;
		}
		else
		{
			out_event = this->eventQueue.front();
			this->eventQueue.pop();
			return true;
		}
	}

	void IGLOContext::Present()
	{
#ifdef IGLO_D3D12

		HRESULT hr = 0;
		switch (presentMode)
		{
		case PresentMode::ImmediateWithTearing:
			hr = d3d12SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
			break;
		case PresentMode::Immediate:
			hr = d3d12SwapChain->Present(0, 0);
			break;
		case PresentMode::Vsync:
			hr = d3d12SwapChain->Present(1, 0);
			break;
		case PresentMode::VsyncHalf:
			hr = d3d12SwapChain->Present(2, 0);
			break;
		default:
			Log(LogType::Error, "Failed to present. Reason: Invalid present mode.");
			break;
		}
		if (FAILED(hr))
		{
			Log(LogType::Error, ToString("Failed presenting backbuffer."
				" Reason: IDXGISwapChain::Present returned error code: ", (uint32_t)hr, "."));

			hr = d3d12Device->GetDeviceRemovedReason();
			if (FAILED(hr))
			{
				std::string reasonStr;
				if (hr == DXGI_ERROR_DEVICE_HUNG)
				{
					reasonStr = "The device has hung.";
				}
				else if (hr == DXGI_ERROR_DEVICE_REMOVED)
				{
					reasonStr = "The device was removed.";
				}
				else if (hr == DXGI_ERROR_DEVICE_RESET)
				{
					reasonStr = "The device was reset.";
				}
				else
				{
					reasonStr = "Unknown device removal reason.";
				}
				Log(LogType::Error, "Device removal detected! Reason: " + reasonStr);
				if (callbackOnDeviceRemoved) callbackOnDeviceRemoved();
			}
		}
#endif
		// Presenting the swap chain submits commands to the graphics command queue, as specified at swap chain creation.
		// We must signal a fence to know when those commands finish executing.
		endOfFrame[frameIndex].graphicsReceipt = commandQueue.SignalFence(CommandListType::Graphics);

		MoveToNextFrame();
	}

	Receipt IGLOContext::Submit(const CommandList& commandList)
	{
		Receipt out = commandQueue.SubmitCommands(commandList);
		if (!out.IsNull())
		{
			switch (commandList.GetCommandListType())
			{
			case CommandListType::Graphics:
				endOfFrame[frameIndex].graphicsReceipt = out;
				break;
			case CommandListType::Compute:
				endOfFrame[frameIndex].computeReceipt = out;
				break;
			case CommandListType::Copy:
				endOfFrame[frameIndex].copyReceipt = out;
				break;
			default:
				throw std::runtime_error("This should be impossible.");
			}
		}
		return out;
	}

	Receipt IGLOContext::Submit(const CommandList** commandLists, uint32_t numCommandLists)
	{
		Receipt out = commandQueue.SubmitCommands(commandLists, numCommandLists);
		if (!out.IsNull())
		{
			switch (commandLists[0]->GetCommandListType())
			{
			case CommandListType::Graphics:
				endOfFrame[frameIndex].graphicsReceipt = out;
				break;
			case CommandListType::Compute:
				endOfFrame[frameIndex].computeReceipt = out;
				break;
			case CommandListType::Copy:
				endOfFrame[frameIndex].copyReceipt = out;
				break;
			default:
				break;
			}
		}
		return out;
	}

	bool IGLOContext::IsComplete(Receipt receipt) const
	{
		return commandQueue.IsComplete(receipt);
	}

	void IGLOContext::WaitForCompletion(Receipt receipt)
	{
		commandQueue.WaitForCompletion(receipt);

		if (commandQueue.IsIdle())
		{
			// When GPU is idle, it's safe to clean up temporary resources
			ClearAllTempResources();
		}
	}

	void IGLOContext::WaitForIdleDevice()
	{
		commandQueue.WaitForIdle();

		// When GPU is idle, it's safe to clean up temporary resources
		ClearAllTempResources();
	}

	void IGLOContext::ClearAllTempResources()
	{
		descriptorHeap.ClearAllTempResources();
		bufferAllocator.ClearAllTempPages();
		for (size_t i = 0; i < endOfFrame.size(); i++)
		{
			endOfFrame[i].delayedUnloadTextures.clear();
		}
	}

	void IGLOContext::MoveToNextFrame()
	{
		frameIndex = (frameIndex + 1) % numFramesInFlight;

		commandQueue.WaitForCompletion(endOfFrame.at(frameIndex).graphicsReceipt);
		commandQueue.WaitForCompletion(endOfFrame.at(frameIndex).computeReceipt);
		commandQueue.WaitForCompletion(endOfFrame.at(frameIndex).copyReceipt);

		descriptorHeap.NextFrame();
		bufferAllocator.NextFrame();

		// The delayed unload textures are held for max 1 full frame cycle
		endOfFrame.at(frameIndex).delayedUnloadTextures.clear();
	}

	void IGLOContext::DelayedTextureUnload(std::shared_ptr<Texture> texture) const
	{
		endOfFrame.at(frameIndex).delayedUnloadTextures.push_back(texture);
	}

	Descriptor IGLOContext::CreateTempConstant(const void* data, uint64_t numBytes) const
	{
		const char* errStr = "Failed to create temporary shader constant. Reason: ";

		Descriptor outDescriptor = descriptorHeap.AllocateTemporaryResource();
		if (outDescriptor.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate temporary resource descriptor."));
			return Descriptor();
		}

		TempBuffer tempBuffer = bufferAllocator.AllocateTempBuffer(numBytes, (uint32_t)BufferPlacementAlignment::Constant);
		if (tempBuffer.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate temporary buffer."));
			return Descriptor();
		}

		memcpy(tempBuffer.data, data, numBytes);

#ifdef IGLO_D3D12
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
		desc.BufferLocation = tempBuffer.d3d12Resource->GetGPUVirtualAddress() + tempBuffer.offset;
		desc.SizeInBytes = (UINT)AlignUp(numBytes, (uint64_t)BufferPlacementAlignment::Constant);
		d3d12Device->CreateConstantBufferView(&desc, descriptorHeap.GetD3D12CPUDescriptorHandleForResource(outDescriptor));
#endif

		return outDescriptor;
	}

	Descriptor IGLOContext::CreateTempStructuredBuffer(const void* data, uint32_t elementStride, uint32_t numElements) const
	{
		const char* errStr = "Failed to create temporary structured buffer. Reason: ";

		uint64_t numBytes = (uint64_t)elementStride * numElements;

		if (numBytes == 0)
		{
			Log(LogType::Error, ToString(errStr, "Size of buffer can't be zero."));
			return Descriptor();
		}

		Descriptor outDescriptor = descriptorHeap.AllocateTemporaryResource();
		if (outDescriptor.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate temporary resource descriptor."));
			return Descriptor();
		}

		TempBuffer tempBuffer = bufferAllocator.AllocateTempBuffer(numBytes + elementStride, 0);
		if (tempBuffer.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate temporary buffer."));
			return Descriptor();
		}

#ifdef IGLO_D3D12
		UINT64 firstElement = (tempBuffer.offset / elementStride) + 1;
		byte* beginning = (byte*)tempBuffer.data - tempBuffer.offset;
		memcpy(beginning + (firstElement * elementStride), data, numBytes);

		D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
		srv.Format = DXGI_FORMAT_UNKNOWN;
		srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv.Buffer.FirstElement = firstElement;
		srv.Buffer.NumElements = numElements;
		srv.Buffer.StructureByteStride = elementStride;
		srv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		d3d12Device->CreateShaderResourceView(tempBuffer.d3d12Resource, &srv,
			descriptorHeap.GetD3D12CPUDescriptorHandleForResource(outDescriptor));
#endif

		return outDescriptor;
	}

	Descriptor IGLOContext::CreateTempRawBuffer(const void* data, uint64_t numBytes) const
	{
		const char* errStr = "Failed to create temporary raw buffer. Reason: ";

		if (numBytes == 0)
		{
			Log(LogType::Error, ToString(errStr, "Size of buffer can't be zero."));
			return Descriptor();
		}
		if (numBytes % 4 != 0)
		{
			Log(LogType::Error, ToString(errStr, "The size of this type of buffer must be a multiple of 4."));
			return Descriptor();
		}

		Descriptor outDescriptor = descriptorHeap.AllocateTemporaryResource();
		if (outDescriptor.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate temporary resource descriptor."));
			return Descriptor();
		}

		TempBuffer tempBuffer = bufferAllocator.AllocateTempBuffer(numBytes,
			(uint32_t)BufferPlacementAlignment::RawBuffer);
		if (tempBuffer.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate temporary buffer."));
			return Descriptor();
		}

		memcpy(tempBuffer.data, data, numBytes);

#ifdef IGLO_D3D12
		D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
		srv.Format = DXGI_FORMAT_R32_TYPELESS;
		srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv.Buffer.FirstElement = tempBuffer.offset / 4;
		srv.Buffer.NumElements = UINT(numBytes / 4);
		srv.Buffer.StructureByteStride = 0;
		srv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

		d3d12Device->CreateShaderResourceView(tempBuffer.d3d12Resource, &srv,
			descriptorHeap.GetD3D12CPUDescriptorHandleForResource(outDescriptor));
#endif

		return outDescriptor;
	}

#ifdef _WIN32
	LRESULT CALLBACK IGLOContext::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		IGLOContext* me;
		if (msg == WM_NCCREATE)
		{
			LPCREATESTRUCT createInfo = reinterpret_cast<LPCREATESTRUCT>(lParam);
			me = static_cast<IGLOContext*>(createInfo->lpCreateParams);
			if (me)
			{
				me->windowHWND = hwnd;
				SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(me));
			}
		}
		else
		{
			me = reinterpret_cast<IGLOContext*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}
		if (me) return me->PrivateWndProc(msg, wParam, lParam);
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	LRESULT CALLBACK IGLOContext::PrivateWndProc(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (callbackWndProcHook)
		{
			// Let hook callback handle the message
			if (callbackWndProcHook(windowHWND, msg, wParam, lParam)) return 0;
		}

		Event event_out;
		event_out.type = EventType::None;

		switch (msg)
		{
		case WM_DROPFILES:
		{
			event_out.type = EventType::DragAndDrop;
			event_out.dragAndDrop.filenames.clear();

			HDROP hDrop = (HDROP)wParam;
			UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, 0, 0);
			UINT maxLength = 0;
			for (UINT i = 0; i < fileCount; i++)
			{
				UINT fileNameLength = DragQueryFileW(hDrop, i, 0, 0) + 1;
				if (fileNameLength > maxLength)
				{
					maxLength = fileNameLength;
				}
			}
			std::vector<wchar_t> filename;
			filename.clear();
			filename.resize(maxLength);
			for (UINT i = 0; i < fileCount; i++)
			{
				DragQueryFileW(hDrop, i, filename.data(), maxLength);

				event_out.dragAndDrop.filenames.push_back(utf16_to_utf8(filename.data()));
			}
			DragFinish(hDrop);
			eventQueue.push(event_out);
			break;
		}

		// Disable MessageBeep on Invalid Syskeypress
		case WM_MENUCHAR:
			return MNC_CLOSE << 16;

		case WM_PAINT:
			// Use the modal loop callback if there is a modal operation active
			if (isLoaded)
			{
				if (!insideModalLoopCallback && callbackModalLoop && (windowActiveResizing || windowActiveMenuLoop))
				{
					insideModalLoopCallback = true;
					callbackModalLoop(callbackModalLoopUserData);
					insideModalLoopCallback = false;
					return 0;
				}
			}
			break;

		case WM_ERASEBKGND:
			// Paint the window background black to prevent white flash after the window has just been created.
			// The background should not be painted here if window is being dragged/moved, as that causes flickering.
			if (!windowActiveResizing)
			{
				RECT rc;
				GetClientRect(windowHWND, &rc);
				FillRect((HDC)wParam, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
				return 0;
			}
			break;

		case WM_KILLFOCUS:
			// When alt-tabbing away in fullscreen mode, hide
			if (displayMode == DisplayMode::BorderlessFullscreen)
			{
				ShowWindow(windowHWND, SW_MINIMIZE);
			}
			event_out.type = EventType::LostFocus;
			eventQueue.push(event_out);
			break;

		case WM_SETFOCUS:
			// When alt-tabbing toward this window in fullscreen mode, show
			event_out.type = EventType::GainedFocus;
			eventQueue.push(event_out);
			break;

		case WM_CLOSE:
			event_out.type = EventType::CloseRequest;
			eventQueue.push(event_out);
			return 0;

		case WM_SETCURSOR:
			// the HIWORD(lParam) tells what the cursor is doing.
			// the LOWORD(lParam) tells where the cursor is. 1 = inside the window
			// If the cursor is inside the window, then use custom cursor
			if (LOWORD(lParam) == 1 && windowCursor != NULL)
			{
				//TODO: Implement support for custom cursors at some point
				SetCursor(windowCursor);
				return true;
			}
			break;

		case WM_ENTERSIZEMOVE:
			windowActiveResizing = true;
			RedrawWindow(windowHWND, NULL, NULL, RDW_INVALIDATE);
			break;

		case WM_ENTERMENULOOP:
			windowActiveMenuLoop = true;
			RedrawWindow(windowHWND, NULL, NULL, RDW_INVALIDATE);
			break;

		case WM_EXITSIZEMOVE:
			windowActiveResizing = false;
			if (displayMode == DisplayMode::Windowed)
			{
				RECT rect = {};
				if (GetWindowRect(windowHWND, &rect))
				{
					windowedPos = IntPoint(rect.left, rect.top);
					if (windowedPos.x < 0) windowedPos.x = 0;
					if (windowedPos.y < 0) windowedPos.y = 0;
				}
				RECT clientArea = {};
				GetClientRect(windowHWND, &clientArea);
				Extent2D resize = Extent2D(clientArea.right - clientArea.left, clientArea.bottom - clientArea.top);
				if (resize != backBufferSize && resize.width != 0 && resize.height != 0)
				{
					windowedSize = resize;
					ResizeBackBuffer(resize, backBufferFormat);
				}
			}
			break;

		case WM_EXITMENULOOP:
			windowActiveMenuLoop = false;
			break;

		case WM_SIZE:
			// Restored, minimized and maximized messages should never be completely ignored, because we rely on these messages to
			// know when to send Minimized and Restored events to the iglo app.
			if (wParam == SIZE_MAXIMIZED ||
				wParam == SIZE_RESTORED ||
				wParam == SIZE_MINIMIZED)
			{
				bool oldMinimizedState = windowMinimizedState;
				windowMinimizedState = (wParam == SIZE_MINIMIZED);

				if (oldMinimizedState != windowMinimizedState)
				{
					event_out.type = windowMinimizedState ? EventType::Minimized : EventType::Restored;
					eventQueue.push(event_out);
				}
				if (displayMode == DisplayMode::Windowed && !windowActiveResizing && !ignoreSizeMsg)
				{
					Extent2D resize = Extent2D(LOWORD(lParam), HIWORD(lParam));

					if (resize != backBufferSize && resize.width != 0 && resize.height != 0)
					{
						windowedSize = resize;
						ResizeBackBuffer(resize, backBufferFormat);
					}
				}
			}
			break;

		case WM_GETMINMAXINFO:
			if (displayMode != DisplayMode::Windowed) break;
			if (ignoreSizeMsg) break;
			// Get difference between client size and window size
			RECT rc;
			rc.left = 0;
			rc.top = 0;
			rc.right = 0;
			rc.bottom = 0;
			AdjustWindowRect(&rc, GetWindowLong(windowHWND, GWL_STYLE), false);
			POINT diff;
			diff.x = rc.right - rc.left;
			diff.y = rc.bottom - rc.top;
			// Set minimum and maximum window size
			if (windowedMinSize.width != 0) ((MINMAXINFO FAR*) lParam)->ptMinTrackSize.x = (LONG)windowedMinSize.width + diff.x;
			if (windowedMinSize.height != 0) ((MINMAXINFO FAR*) lParam)->ptMinTrackSize.y = (LONG)windowedMinSize.height + diff.y;
			if (windowedMaxSize.width != 0) ((MINMAXINFO FAR*) lParam)->ptMaxTrackSize.x = (LONG)windowedMaxSize.width + diff.x;
			if (windowedMaxSize.height != 0) ((MINMAXINFO FAR*) lParam)->ptMaxTrackSize.y = (LONG)windowedMaxSize.height + diff.y;
			break;

		case WM_MOUSEMOVE:
			event_out.type = EventType::MouseMove;
			event_out.mouse.x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
			event_out.mouse.y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
			event_out.mouse.button = MouseButton::None;
			event_out.mouse.scrollWheel = 0;
			mousePosition.x = event_out.mouse.x;
			mousePosition.y = event_out.mouse.y;
			eventQueue.push(event_out);
			break;

		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONDBLCLK:
			event_out.type = EventType::MouseButtonDown;
			event_out.mouse.x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
			event_out.mouse.y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
			event_out.mouse.scrollWheel = 0;
			if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK)
			{
				event_out.mouse.button = MouseButton::Right;
			}
			else if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK)
			{
				event_out.mouse.button = MouseButton::Left;
			}
			else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK)
			{
				event_out.mouse.button = MouseButton::Middle;
			}
			else if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK)
			{
				event_out.mouse.button = MouseButton::XButton1;
				if (HIWORD(wParam) == 1) event_out.mouse.button = MouseButton::XButton1;
				if (HIWORD(wParam) == 2) event_out.mouse.button = MouseButton::XButton2;
			}
			else
			{
				event_out.mouse.button = MouseButton::Left; // Unknown button. Defaulting to left button.
			}
			//bool control = ((wParam & MK_CONTROL) == MK_CONTROL);
			//bool shift = ((wParam & MK_SHIFT) == MK_SHIFT);

			CaptureMouse(true);
			mouseButtonIsDown.SetTrue((uint64_t)event_out.mouse.button);
			mousePosition.x = event_out.mouse.x;
			mousePosition.y = event_out.mouse.y;
			eventQueue.push(event_out);
			break;

		case WM_RBUTTONUP:
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
			event_out.type = EventType::MouseButtonUp;
			event_out.mouse.x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
			event_out.mouse.y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
			event_out.mouse.scrollWheel = 0;
			if (msg == WM_RBUTTONUP)event_out.mouse.button = MouseButton::Right;
			else if (msg == WM_LBUTTONUP)event_out.mouse.button = MouseButton::Left;
			else if (msg == WM_MBUTTONUP)event_out.mouse.button = MouseButton::Middle;
			else if (msg == WM_XBUTTONUP)
			{
				event_out.mouse.button = MouseButton::XButton1;
				if (HIWORD(wParam) == 1) event_out.mouse.button = MouseButton::XButton1;
				if (HIWORD(wParam) == 2) event_out.mouse.button = MouseButton::XButton2;
			}
			else
			{
				event_out.mouse.button = MouseButton::Left; // Unknown button. Defaulting to left button.
			}

			CaptureMouse(false);
			mouseButtonIsDown.SetFalse((uint64_t)event_out.mouse.button);
			mousePosition.x = event_out.mouse.x;
			mousePosition.y = event_out.mouse.y;
			eventQueue.push(event_out);
			break;

		case WM_MOUSEWHEEL:
			if ((int)wParam != 0)
			{
				event_out.type = EventType::MouseWheel;
				event_out.mouse.scrollWheel = GET_WHEEL_DELTA_WPARAM(wParam);
				// WM_MOUSEWHEEL gives incorrect mouse coordinates, so use previous mouse coords instead
				event_out.mouse.x = mousePosition.x;
				event_out.mouse.y = mousePosition.y;
				event_out.mouse.button = MouseButton::None;
				eventQueue.push(event_out);
			}
			break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			event_out.key = Key::Unknown;
			UINT scancode = (lParam & 0x00ff0000) >> 16;
			int extended = (lParam & 0x01000000) != 0;
			switch (wParam)
			{
			case 'A': event_out.key = Key::A; break;
			case 'B': event_out.key = Key::B; break;
			case 'C': event_out.key = Key::C; break;
			case 'D': event_out.key = Key::D; break;
			case 'E': event_out.key = Key::E; break;
			case 'F': event_out.key = Key::F; break;
			case 'G': event_out.key = Key::G; break;
			case 'H': event_out.key = Key::H; break;
			case 'I': event_out.key = Key::I; break;
			case 'J': event_out.key = Key::J; break;
			case 'K': event_out.key = Key::K; break;
			case 'L': event_out.key = Key::L; break;
			case 'M': event_out.key = Key::M; break;
			case 'N': event_out.key = Key::N; break;
			case 'O': event_out.key = Key::O; break;
			case 'P': event_out.key = Key::P; break;
			case 'Q': event_out.key = Key::Q; break;
			case 'R': event_out.key = Key::R; break;
			case 'S': event_out.key = Key::S; break;
			case 'T': event_out.key = Key::T; break;
			case 'U': event_out.key = Key::U; break;
			case 'V': event_out.key = Key::V; break;
			case 'W': event_out.key = Key::W; break;
			case 'X': event_out.key = Key::X; break;
			case 'Y': event_out.key = Key::Y; break;
			case 'Z': event_out.key = Key::Z; break;
			case '0': event_out.key = Key::Num0; break;
			case '1': event_out.key = Key::Num1; break;
			case '2': event_out.key = Key::Num2; break;
			case '3': event_out.key = Key::Num3; break;
			case '4': event_out.key = Key::Num4; break;
			case '5': event_out.key = Key::Num5; break;
			case '6': event_out.key = Key::Num6; break;
			case '7': event_out.key = Key::Num7; break;
			case '8': event_out.key = Key::Num8; break;
			case '9': event_out.key = Key::Num9; break;
			case VK_NUMPAD0: event_out.key = Key::Numpad0; break;
			case VK_NUMPAD1: event_out.key = Key::Numpad1; break;
			case VK_NUMPAD2: event_out.key = Key::Numpad2; break;
			case VK_NUMPAD3: event_out.key = Key::Numpad3; break;
			case VK_NUMPAD4: event_out.key = Key::Numpad4; break;
			case VK_NUMPAD5: event_out.key = Key::Numpad5; break;
			case VK_NUMPAD6: event_out.key = Key::Numpad6; break;
			case VK_NUMPAD7: event_out.key = Key::Numpad7; break;
			case VK_NUMPAD8: event_out.key = Key::Numpad8; break;
			case VK_NUMPAD9: event_out.key = Key::Numpad9; break;
			case VK_F1: event_out.key = Key::F1; break;
			case VK_F2: event_out.key = Key::F2; break;
			case VK_F3: event_out.key = Key::F3; break;
			case VK_F4: event_out.key = Key::F4; break;
			case VK_F5: event_out.key = Key::F5; break;
			case VK_F6: event_out.key = Key::F6; break;
			case VK_F7: event_out.key = Key::F7; break;
			case VK_F8: event_out.key = Key::F8; break;
			case VK_F9: event_out.key = Key::F9; break;
			case VK_F10: event_out.key = Key::F10; break;
			case VK_F11: event_out.key = Key::F11; break;
			case VK_F12: event_out.key = Key::F12; break;
			case VK_F13: event_out.key = Key::F13; break;
			case VK_F14: event_out.key = Key::F14; break;
			case VK_F15: event_out.key = Key::F15; break;
			case VK_F16: event_out.key = Key::F16; break;
			case VK_F17: event_out.key = Key::F17; break;
			case VK_F18: event_out.key = Key::F18; break;
			case VK_F19: event_out.key = Key::F19; break;
			case VK_F20: event_out.key = Key::F20; break;
			case VK_F21: event_out.key = Key::F21; break;
			case VK_F22: event_out.key = Key::F22; break;
			case VK_F23: event_out.key = Key::F23; break;
			case VK_F24: event_out.key = Key::F24; break;
			case VK_CONTROL:
				event_out.key = Key::LeftControl;
				if (extended) event_out.key = Key::RightControl;
				break;
			case VK_SHIFT:
				if (MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) event_out.key = Key::LeftShift;
				else event_out.key = Key::RightShift;
				break;
			case VK_MENU:
				event_out.key = Key::LeftAlt;
				if (extended) event_out.key = Key::RightAlt;
				break;
			case VK_LWIN: event_out.key = Key::LeftSystem; break;
			case VK_RWIN: event_out.key = Key::RightSystem; break;
			case VK_PRIOR: event_out.key = Key::PageUp; break;
			case VK_NEXT: event_out.key = Key::PageDown; break;
			case VK_END: event_out.key = Key::End; break;
			case VK_HOME: event_out.key = Key::Home; break;
			case VK_ESCAPE: event_out.key = Key::Escape; break;
			case VK_SPACE: event_out.key = Key::Space; break;
			case VK_CAPITAL: event_out.key = Key::CapsLock; break;
			case VK_PAUSE: event_out.key = Key::Pause; break;
			case VK_RETURN: event_out.key = Key::Enter; break;
			case VK_CLEAR: event_out.key = Key::Clear; break;
			case VK_BACK: event_out.key = Key::Backspace; break;
			case VK_TAB: event_out.key = Key::Tab; break;
			case VK_INSERT: event_out.key = Key::Insert; break;
			case VK_DELETE: event_out.key = Key::Delete; break;
			case VK_SNAPSHOT: event_out.key = Key::PrintScreen; break;
			case VK_LEFT: event_out.key = Key::Left; break;
			case VK_UP: event_out.key = Key::Up; break;
			case VK_RIGHT: event_out.key = Key::Right; break;
			case VK_DOWN: event_out.key = Key::Down; break;
			case VK_SLEEP: event_out.key = Key::Sleep; break;
			case VK_MULTIPLY: event_out.key = Key::Multiply; break;
			case VK_ADD: event_out.key = Key::Add; break;
			case VK_SEPARATOR: event_out.key = Key::Seperator; break;
			case VK_SUBTRACT: event_out.key = Key::Subtract; break;
			case VK_DECIMAL: event_out.key = Key::Decimal; break;
			case VK_DIVIDE: event_out.key = Key::Divide; break;
			case VK_NUMLOCK: event_out.key = Key::NumLock; break;
			case VK_SCROLL: event_out.key = Key::ScrollLock; break;
			case VK_APPS: event_out.key = Key::Apps; break;
			case VK_VOLUME_MUTE: event_out.key = Key::VolumeMute; break;
			case VK_VOLUME_DOWN: event_out.key = Key::VolumeDown; break;
			case VK_VOLUME_UP: event_out.key = Key::VolumeUp; break;
			case VK_MEDIA_NEXT_TRACK: event_out.key = Key::MediaNextTrack; break;
			case VK_MEDIA_PREV_TRACK: event_out.key = Key::MediaPrevTrack; break;
			case VK_MEDIA_STOP: event_out.key = Key::MediaStop; break;
			case VK_MEDIA_PLAY_PAUSE: event_out.key = Key::MediaPlayPause; break;
			case VK_OEM_PLUS: event_out.key = Key::Plus; break;
			case VK_OEM_COMMA: event_out.key = Key::Comma; break;
			case VK_OEM_MINUS: event_out.key = Key::Minus; break;
			case VK_OEM_PERIOD: event_out.key = Key::Period; break;
			case VK_OEM_1: event_out.key = Key::OEM_1; break;
			case VK_OEM_2: event_out.key = Key::OEM_2; break;
			case VK_OEM_3: event_out.key = Key::OEM_3; break;
			case VK_OEM_4: event_out.key = Key::OEM_4; break;
			case VK_OEM_5: event_out.key = Key::OEM_5; break;
			case VK_OEM_6: event_out.key = Key::OEM_6; break;
			case VK_OEM_7: event_out.key = Key::OEM_7; break;
			case VK_OEM_8: event_out.key = Key::OEM_8; break;
			case VK_OEM_102: event_out.key = Key::OEM_102; break;
			default:
				break;
			}

			if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN)
			{
				if (event_out.key != Key::Unknown && (uint64_t)event_out.key < keyIsDown.GetSize())
				{
					keyIsDown.SetTrue((uint64_t)event_out.key);
				}
				event_out.type = EventType::KeyPress;
				bool keyIsDownBefore = lParam & (1 << 30);
				if (!keyIsDownBefore) // If this key wasn't down before...
				{
					Event bonusKeyDownEvent = event_out;
					bonusKeyDownEvent.type = EventType::KeyDown;
					eventQueue.push(bonusKeyDownEvent); //...then create a KeyDown event
				}
			}
			else // WM_KEYUP or WM_SYSKEYUP
			{
				if (event_out.key != Key::Unknown && (uint64_t)event_out.key < keyIsDown.GetSize())
				{
					keyIsDown.SetFalse((uint64_t)event_out.key);
				}
				event_out.type = EventType::KeyUp;
			}
			eventQueue.push(event_out);
		}
		break;

		case WM_CHAR:
			event_out.type = EventType::TextEntered;
			event_out.textEntered.character_utf32 = (uint32_t)wParam;
			eventQueue.push(event_out);
			break;

		} // switch (msg)

		return DefWindowProc(windowHWND, msg, wParam, lParam);
	}
#endif
#ifdef __linux__
	void IGLOContext::PrivateLinuxProc(XEvent e)
	{
		char tempStr[25]{};
		KeySym keysym = 0;
		int len = 0;
		Event event_out;
		MouseButton button;
		switch (e.type)
		{
		case ButtonPress:
			button = MouseButton::Left; // Unknown button. Defaulting to left button.
			if (e.xbutton.button == 1) button = MouseButton::Left;
			else if (e.xbutton.button == 2) button = MouseButton::Middle;
			else if (e.xbutton.button == 3) button = MouseButton::Right;
			else if (e.xbutton.button == 8) button = MouseButton::XButton1;
			else if (e.xbutton.button == 9) button = MouseButton::XButton2;
			else if (e.xbutton.button == 4)
			{
				event_out.type = EventType::MouseWheel;
				event_out.mouse.scrollWheel = 1; // Scroll up
				event_out.mouse.button = MouseButton::None;
				event_out.mouse.x = mousePosition.x;
				event_out.mouse.y = mousePosition.y;
				eventQueue.push(event_out);
				break;
			}
			else if (e.xbutton.button == 5)
			{
				event_out.type = EventType::MouseWheel;
				event_out.mouse.scrollWheel = -1; // Scroll down
				event_out.mouse.button = MouseButton::None;
				event_out.mouse.x = mousePosition.x;
				event_out.mouse.y = mousePosition.y;
				eventQueue.push(event_out);
				break;
			}
			mouseButtonIsDown.SetTrue((uint64_t)button);
			mousePosition.x = e.xbutton.x;
			mousePosition.y = e.xbutton.y;
			event_out.type = EventType::MouseButtonDown;
			event_out.mouse.button = button;
			event_out.mouse.x = mousePosition.x;
			event_out.mouse.y = mousePosition.y;
			event_out.mouse.scrollWheel = 0;
			eventQueue.push(event_out);
			break;
		case ButtonRelease:
			button = MouseButton::Left; // Unknown button. Defaulting to left button.
			if (e.xbutton.button == 1) button = MouseButton::Left;
			else if (e.xbutton.button == 2) button = MouseButton::Middle;
			else if (e.xbutton.button == 3) button = MouseButton::Right;
			else if (e.xbutton.button == 8) button = MouseButton::XButton1;
			else if (e.xbutton.button == 9) button = MouseButton::XButton2;
			mouseButtonIsDown.SetFalse((uint64_t)button);
			mousePosition.x = e.xbutton.x;
			mousePosition.y = e.xbutton.y;
			event_out.type = EventType::MouseButtonUp;
			event_out.mouse.button = button;
			event_out.mouse.x = mousePosition.x;
			event_out.mouse.y = mousePosition.y;
			event_out.mouse.scrollWheel = 0;
			eventQueue.push(event_out);
			break;
		case MotionNotify:
			mousePosition.x = e.xmotion.x;
			mousePosition.y = e.xmotion.y;
			event_out.type = EventType::MouseMove;
			event_out.mouse.x = e.xmotion.x;
			event_out.mouse.y = e.xmotion.y;
			event_out.mouse.button = MouseButton::None;
			event_out.mouse.scrollWheel = 0;
			eventQueue.push(event_out);
			break;
		case KeymapNotify:
			XRefreshKeyboardMapping(&e.xmapping);
			break;
		case KeyPress_:
		case KeyRelease:
			len = XLookupString(&e.xkey, tempStr, 25, &keysym, NULL);
			if (len > 0)
			{
				if (e.type == KeyRelease)
				{
					//TODO: remove this debug print
					Print("Key released: " + std::string(tempStr) + " - " + std::to_string(keysym) + " (keycode " + std::to_string(e.xkey.keycode) + ")" + "\n");
				}
				if (e.type == KeyPress_)
				{
					//TODO: remove this debug print
					Print("Key pressed: " + std::string(tempStr) + " - " + std::to_string(keysym) + " (keycode " + std::to_string(e.xkey.keycode) + ")" + "\n");
				}
			}

			event_out.key = Key::Unknown;

			//Print("Keycode " + std::to_string(e.xkey.keycode) + "\n"); //TODO: Remove this debug line
			switch (keysym)
			{
				//TODO: Does capital and shift keys work also?

				///////////////////////////////////////////////////////////////////////////////////////////////////////
			case 'A': case 'a': event_out.key = Key::A; break;
			case 'B': case 'b': event_out.key = Key::B; break;
			case 'C': case 'c': event_out.key = Key::C; break;
			case 'D': case 'd': event_out.key = Key::D; break;
			case 'E': case 'e': event_out.key = Key::E; break;
			case 'F': case 'f': event_out.key = Key::F; break;
			case 'G': case 'g': event_out.key = Key::G; break;
			case 'H': case 'h': event_out.key = Key::H; break;
			case 'I': case 'i': event_out.key = Key::I; break;
			case 'J': case 'j': event_out.key = Key::J; break;
			case 'K': case 'k': event_out.key = Key::K; break;
			case 'L': case 'l': event_out.key = Key::L; break;
			case 'M': case 'm': event_out.key = Key::M; break;
			case 'N': case 'n': event_out.key = Key::N; break;
			case 'O': case 'o': event_out.key = Key::O; break;
			case 'P': case 'p': event_out.key = Key::P; break;
			case 'Q': case 'q': event_out.key = Key::Q; break;
			case 'R': case 'r': event_out.key = Key::R; break;
			case 'S': case 's': event_out.key = Key::S; break;
			case 'T': case 't': event_out.key = Key::T; break;
			case 'U': case 'u': event_out.key = Key::U; break;
			case 'V': case 'v': event_out.key = Key::V; break;
			case 'W': case 'w': event_out.key = Key::W; break;
			case 'X': case 'x': event_out.key = Key::X; break;
			case 'Y': case 'y': event_out.key = Key::Y; break;
			case 'Z': case 'z': event_out.key = Key::Z; break;
			case '0': event_out.key = Key::Num0; break;
			case '1': event_out.key = Key::Num1; break;
			case '2': event_out.key = Key::Num2; break;
			case '3': event_out.key = Key::Num3; break;
			case '4': event_out.key = Key::Num4; break;
			case '5': event_out.key = Key::Num5; break;
			case '6': event_out.key = Key::Num6; break;
			case '7': event_out.key = Key::Num7; break;
			case '8': event_out.key = Key::Num8; break;
			case '9': event_out.key = Key::Num9; break;
			case XK_KP_0: event_out.key = Key::Numpad0; break;
			case XK_KP_1: event_out.key = Key::Numpad1; break;
			case XK_KP_2: event_out.key = Key::Numpad2; break;
			case XK_KP_3: event_out.key = Key::Numpad3; break;
			case XK_KP_4: event_out.key = Key::Numpad4; break;
			case XK_KP_5: event_out.key = Key::Numpad5; break;
			case XK_KP_6: event_out.key = Key::Numpad6; break;
			case XK_KP_7: event_out.key = Key::Numpad7; break;
			case XK_KP_8: event_out.key = Key::Numpad8; break;
			case XK_KP_9: event_out.key = Key::Numpad9; break;
			case XK_F1: event_out.key = Key::F1; break;
			case XK_F2: event_out.key = Key::F2; break;
			case XK_F3: event_out.key = Key::F3; break;
			case XK_F4: event_out.key = Key::F4; break;
			case XK_F5: event_out.key = Key::F5; break;
			case XK_F6: event_out.key = Key::F6; break;
			case XK_F7: event_out.key = Key::F7; break;
			case XK_F8: event_out.key = Key::F8; break;
			case XK_F9: event_out.key = Key::F9; break;
			case XK_F10: event_out.key = Key::F10; break;
			case XK_F11: event_out.key = Key::F11; break;
			case XK_F12: event_out.key = Key::F12; break;
			case XK_F13: event_out.key = Key::F13; break;
			case XK_F14: event_out.key = Key::F14; break;
			case XK_F15: event_out.key = Key::F15; break;
			case XK_F16: event_out.key = Key::F16; break;
			case XK_F17: event_out.key = Key::F17; break;
			case XK_F18: event_out.key = Key::F18; break;
			case XK_F19: event_out.key = Key::F19; break;
			case XK_F20: event_out.key = Key::F20; break;
			case XK_F21: event_out.key = Key::F21; break;
			case XK_F22: event_out.key = Key::F22; break;
			case XK_F23: event_out.key = Key::F23; break;
			case XK_F24: event_out.key = Key::F24; break;
			case XK_Control_L: event_out.key = Key::LeftControl; break;
			case XK_Control_R: event_out.key = Key::RightControl; break;
			case XK_Shift_L: event_out.key = Key::LeftShift; break;
			case XK_Shift_R: event_out.key = Key::RightShift; break;
			case XK_Alt_L: event_out.key = Key::LeftAlt; break;
			case XK_Alt_R: event_out.key = Key::RightAlt; break;
				//case XK_LWIN: event_out.key = Key::LeftSystem; break;
				//case XK_RWIN: event_out.key = Key::RightSystem; break;
			case XK_Page_Up: event_out.key = Key::PageUp; break;
			case XK_Page_Down: event_out.key = Key::PageDown; break;
			case XK_End: event_out.key = Key::End; break;
			case XK_Home: event_out.key = Key::Home; break;
			case XK_Escape: event_out.key = Key::Escape; break;
			case XK_space: event_out.key = Key::Space; break;
			case XK_Caps_Lock: event_out.key = Key::CapsLock; break;
			case XK_Pause: event_out.key = Key::Pause; break;
			case XK_Return: event_out.key = Key::Enter; break;
			case XK_Clear: event_out.key = Key::Clear; break;
			case XK_BackSpace: event_out.key = Key::Backspace; break;
			case XK_Tab: event_out.key = Key::Tab; break;
			case XK_Insert: event_out.key = Key::Insert; break;
			case XK_Delete: event_out.key = Key::Delete; break;
			case XK_Print: event_out.key = Key::PrintScreen; break;
			case XK_Left: event_out.key = Key::Left; break;
			case XK_Up: event_out.key = Key::Up; break;
			case XK_Right: event_out.key = Key::Right; break;
			case XK_Down: event_out.key = Key::Down; break;
				//case XK_Sleep: event_out.key = Key::Sleep; break;
			case XK_KP_Multiply: event_out.key = Key::Multiply; break;
			case XK_KP_Add: event_out.key = Key::Add; break;
			case XK_KP_Separator: event_out.key = Key::Seperator; break;
			case XK_KP_Subtract: event_out.key = Key::Subtract; break;
			case XK_KP_Decimal: event_out.key = Key::Decimal; break;
			case XK_KP_Divide: event_out.key = Key::Divide; break;
			case XK_Num_Lock: event_out.key = Key::NumLock; break;
			case XK_Scroll_Lock: event_out.key = Key::ScrollLock; break;
				//case XK_Apps: event_out.key = Key::Apps; break;
				//case XK_Volume_Mute: event_out.key = Key::VolumeMute; break;
				//case XK_Volume_Down: event_out.key = Key::VolumeDown; break;
				//case XK_Volume_Up: event_out.key = Key::VolumeUp; break;
				//case XK_MEDIA_NEXT_TRACK: event_out.key = Key::MediaNextTrack; break;
				//case XK_MEDIA_PREV_TRACK: event_out.key = Key::MediaPrevTrack; break;
				//case XK_MEDIA_STOP: event_out.key = Key::MediaStop; break;
				//case XK_MEDIA_PLAY_PAUSE: event_out.key = Key::MediaPlayPause; break;
			case XK_plus: event_out.key = Key::Plus; break;
			case XK_comma: event_out.key = Key::Comma; break;
			case XK_minus: event_out.key = Key::Minus; break;
			case XK_period: event_out.key = Key::Period; break;

				// TODO: Are these correct? Try holding Shift and enter these keys. Maybe use keycodes instead? 'xev' command in linux will help.
			case XK_dead_diaeresis: event_out.key = Key::OEM_1; break;
			case XK_apostrophe: event_out.key = Key::OEM_2; break;
			case XK_odiaeresis: event_out.key = Key::OEM_3; break;
			case XK_dead_acute: event_out.key = Key::OEM_4; break;
			case XK_section: event_out.key = Key::OEM_5; break;
			case XK_aring: event_out.key = Key::OEM_6; break;
			case XK_adiaeresis: event_out.key = Key::OEM_7; break;
				//case XK_OEM_8: event_out.key = Key::OEM_8; break;
			case XK_less: event_out.key = Key::OEM_102; break;

			default:
				break;
			}

			if (e.type == KeyPress_)
			{
				bool keyIsDownBefore = keyIsDown.GetAt((uint64_t)event_out.key); //TODO: does this work?
				if (event_out.key != Key::Unknown && (uint64_t)event_out.key < keyIsDown.GetSize())
				{
					keyIsDown.SetTrue((uint64_t)event_out.key);
				}
				event_out.type = EventType::KeyPress;
				if (!keyIsDownBefore) // If this key wasn't down before...
				{
					Event bonusKeyDownEvent = event_out;
					bonusKeyDownEvent.type = EventType::KeyDown;

					eventQueue.push(bonusKeyDownEvent); //...then create a KeyDown event
				}
			}
			else // KeyRelease
			{
				if (event_out.key != Key::Unknown && (uint64_t)event_out.key < keyIsDown.GetSize())
				{
					keyIsDown.SetFalse((uint64_t)event_out.key);
				}
				event_out.type = EventType::KeyUp;
			}
			eventQueue.push(event_out);

			break;
		case Expose:
			if (e.xexpose.count == 0)
			{
				XWindowAttributes attribs;
				XGetWindowAttributes(displayHandle, windowHandle, &attribs);
				ResizeBackBuffer(Extent2D(attribs.width, attribs.height), this->backBufferFormat);
			}

		case ClientMessage:
			if (e.xclient.data.l[0] == atomWmDeleteWindow)
			{
				event_out.type = EventType::CloseRequest;
				eventQueue.push(event_out);
				break;
			}
			break;
		case DestroyNotify:
			break;
		}
	}
#endif

	void Buffer::Unload()
	{
		for (size_t i = 0; i < descriptor.size(); i++)
		{
			if (!descriptor[i].IsNull())
			{
				if (context) context->GetDescriptorHeap().FreePersistentResource(descriptor[i]);
				else Log(LogType::Error, "Buffer descriptor memory leak detected! This should be impossible.");
			}
		}
		descriptor.clear();
		descriptor.shrink_to_fit();

		if (!unorderedAccessDescriptor.IsNull())
		{
			if (context) context->GetDescriptorHeap().FreePersistentResource(unorderedAccessDescriptor);
			else Log(LogType::Error, "Buffer descriptor memory leak detected! This should be impossible.");
		}
		unorderedAccessDescriptor.SetToNull();

#ifdef IGLO_D3D12
		for (size_t i = 0; i < d3d12Resource.size(); i++)
		{
			if (d3d12Resource[i])
			{
				if (usage == BufferUsage::Dynamic)
				{
					d3d12Resource[i]->Unmap(0, nullptr);
				}
				else if (usage == BufferUsage::Readable)
				{
					D3D12_RANGE noWrite = { 0, 0 };
					d3d12Resource[i]->Unmap(0, &noWrite);
				}
			}
		}
		d3d12Resource.clear();
		d3d12Resource.shrink_to_fit();
#endif

		isLoaded = false;
		context = nullptr;
		type = BufferType::VertexBuffer;
		usage = BufferUsage::Default;
		size = 0;
		stride = 0;
		numElements = 0;

		mapped.clear();
		mapped.shrink_to_fit();

		dynamicSetCounter = 0;
	}

	Buffer::Buffer(Buffer&& other) noexcept
	{
		*this = std::move(other);
	}
	Buffer& Buffer::operator=(Buffer&& other) noexcept
	{
		Unload();

#ifdef IGLO_D3D12
		std::swap(this->d3d12Resource, other.d3d12Resource);
#endif

		std::swap(this->isLoaded, other.isLoaded);
		std::swap(this->context, other.context);
		std::swap(this->type, other.type);
		std::swap(this->usage, other.usage);
		std::swap(this->size, other.size);
		std::swap(this->stride, other.stride);
		std::swap(this->numElements, other.numElements);
		std::swap(this->descriptor, other.descriptor);
		std::swap(this->unorderedAccessDescriptor, other.unorderedAccessDescriptor);
		std::swap(this->mapped, other.mapped);
		std::swap(this->dynamicSetCounter, other.dynamicSetCounter);

		return *this;
	}

	const Descriptor* Buffer::GetDescriptor() const
	{
		if (!isLoaded) return nullptr;
		if (usage == BufferUsage::Readable) return nullptr;
		switch (type)
		{
		case BufferType::StructuredBuffer:
		case BufferType::RawBuffer:
		case BufferType::ShaderConstant:
			if (usage == BufferUsage::Dynamic) return &descriptor.at(dynamicSetCounter);
			return &descriptor.at(0);
		default:
			return nullptr;
		}
	}

	const Descriptor* Buffer::GetUnorderedAccessDescriptor() const
	{
		if (!isLoaded) return nullptr;
		if (unorderedAccessDescriptor.IsNull()) return nullptr;
		return &unorderedAccessDescriptor;
	}

#ifdef IGLO_D3D12
	ID3D12Resource* Buffer::GetD3D12Resource() const
	{
		if (!isLoaded) return nullptr;
		switch (usage)
		{
		case BufferUsage::Default:
		case BufferUsage::UnorderedAccess:
			return d3d12Resource.at(0).Get();
		case BufferUsage::Readable:
			return d3d12Resource.at(context->GetFrameIndex()).Get();
		case BufferUsage::Dynamic:
			return d3d12Resource.at(dynamicSetCounter).Get();
		default:
			throw std::runtime_error("This should be impossible.");
		}
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC Buffer::GenerateD3D12UAVDesc() const
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC out = {};
		
		if (type == BufferType::StructuredBuffer)
		{
			out.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			out.Format = DXGI_FORMAT_UNKNOWN;
			out.Buffer.FirstElement = 0;
			out.Buffer.NumElements = numElements;
			out.Buffer.StructureByteStride = stride;
			out.Buffer.CounterOffsetInBytes = 0;
			out.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		}
		else if (type == BufferType::RawBuffer)
		{
			out.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			out.Format = DXGI_FORMAT_R32_TYPELESS;
			out.Buffer.FirstElement = 0;
			out.Buffer.NumElements = uint32_t(size) / sizeof(uint32_t);
			out.Buffer.StructureByteStride = 0;
			out.Buffer.CounterOffsetInBytes = 0;
			out.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
		}
		
		return out;
	}
#endif

	bool Buffer::InternalLoad(const IGLOContext& context, uint64_t size, uint32_t stride, uint32_t numElements, BufferUsage usage, BufferType type)
	{
		Unload();

		const char* errStr = nullptr;
		switch (type)
		{
		case BufferType::VertexBuffer:	   errStr = "Failed to create vertex buffer. Reason: "; break;
		case BufferType::IndexBuffer:	   errStr = "Failed to create index buffer. Reason: "; break;
		case BufferType::StructuredBuffer: errStr = "Failed to create structured buffer. Reason: "; break;
		case BufferType::RawBuffer:        errStr = "Failed to create raw buffer. Reason: "; break;
		case BufferType::ShaderConstant:   errStr = "Failed to create shader constant buffer. Reason: "; break;
		default: throw std::invalid_argument("This should be impossible.");
		}

		if (size == 0)
		{
			Log(LogType::Error, ToString(errStr, "Size of buffer can't be zero."));
			return false;
		}

		if (usage == BufferUsage::UnorderedAccess)
		{
			if (type != BufferType::RawBuffer &&
				type != BufferType::StructuredBuffer)
			{
				Log(LogType::Error, ToString(errStr, "Unordered access buffer usage is only supported for raw and structured buffers."));
				return false;
			}
		}

		if (type == BufferType::VertexBuffer || type == BufferType::RawBuffer)
		{
			if (size % 4 != 0)
			{
				Log(LogType::Error, ToString(errStr, "The size of this type of buffer must be a multiple of 4."));
				Unload();
				return false;
			}
			if (stride != 0)
			{
				if (stride % 4 != 0)
				{
					Log(LogType::Error, ToString(errStr, "The stride of this type of buffer must be a multiple of 4."));
					Unload();
					return false;
				}
			}
		}

#ifdef IGLO_D3D12

		this->mapped.clear();
		this->mapped.shrink_to_fit();
		this->d3d12Resource.clear();
		this->d3d12Resource.shrink_to_fit();
		this->descriptor.clear();
		this->descriptor.shrink_to_fit();
		if (usage == BufferUsage::Readable || usage == BufferUsage::Dynamic)
		{
			this->d3d12Resource.resize(context.GetNumFramesInFlight(), nullptr);
		}
		else
		{
			this->d3d12Resource.resize(1, nullptr);
		}

		for (size_t i = 0; i < this->d3d12Resource.size(); i++)
		{
			D3D12_RESOURCE_DESC1 desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Alignment = 0;
			desc.Height = 1;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.SamplerFeedbackMipRegion.Width = 0;
			desc.SamplerFeedbackMipRegion.Height = 0;
			desc.SamplerFeedbackMipRegion.Depth = 0;

			desc.Flags = D3D12_RESOURCE_FLAG_NONE;
			if (usage == BufferUsage::UnorderedAccess) desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			desc.Width = size;
			if (type == BufferType::ShaderConstant) desc.Width = AlignUp(size, (uint64_t)BufferPlacementAlignment::Constant);

			D3D12_HEAP_PROPERTIES heapProp = {};
			heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProp.CreationNodeMask = 1;
			heapProp.VisibleNodeMask = 1;

			heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
			if (usage == BufferUsage::Dynamic) heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
			else if (usage == BufferUsage::Readable) heapProp.Type = D3D12_HEAP_TYPE_READBACK;

			D3D12_BARRIER_LAYOUT barrierLayout = D3D12_BARRIER_LAYOUT_UNDEFINED;

			HRESULT hr = context.GetD3D12Device()->CreateCommittedResource3(&heapProp, D3D12_HEAP_FLAG_NONE, &desc, barrierLayout,
				nullptr, nullptr, 0, nullptr, IID_PPV_ARGS(&this->d3d12Resource[i]));
			if (FAILED(hr))
			{
				Log(LogType::Error, ToString(errStr, "ID3D12Device10::CreateCommittedResource3 returned error code: ", (uint32_t)hr, "."));
				Unload();
				return false;
			}

			if (usage == BufferUsage::Dynamic || usage == BufferUsage::Readable)
			{
				D3D12_RANGE noRead = { 0, 0 };
				D3D12_RANGE* readRange = &noRead;
				if (usage == BufferUsage::Readable) readRange = nullptr;
				void* mappedPtr = nullptr;
				hr = this->d3d12Resource[i]->Map(0, readRange, &mappedPtr);
				if (FAILED(hr))
				{
					Log(LogType::Error, ToString(errStr, "ID3D12Resource::Map returned error code: ", (uint32_t)hr, "."));
					Unload();
					return false;
				}

				this->mapped.push_back(mappedPtr);
			}
		}

		uint32_t numDescriptors = 0;
		if (usage == BufferUsage::Default) numDescriptors = 1;
		else if (usage == BufferUsage::UnorderedAccess) numDescriptors = 1;
		else if (usage == BufferUsage::Dynamic) numDescriptors = context.GetNumFramesInFlight();

		for (uint32_t i = 0; i < numDescriptors; i++)
		{
			if (type == BufferType::StructuredBuffer ||
				type == BufferType::RawBuffer ||
				type == BufferType::ShaderConstant)
			{
				Descriptor allocatedDescriptor = context.GetDescriptorHeap().AllocatePersistentResource();
				if (allocatedDescriptor.IsNull())
				{
					Log(LogType::Error, ToString(errStr, "Failed to allocate descriptor."));
					Unload();
					return false;
				}

				if (type == BufferType::ShaderConstant)
				{
					D3D12_CONSTANT_BUFFER_VIEW_DESC cbv = {};
					cbv.BufferLocation = this->d3d12Resource.at(i)->GetGPUVirtualAddress();
					cbv.SizeInBytes = (UINT)AlignUp(size, (uint64_t)BufferPlacementAlignment::Constant);
					context.GetD3D12Device()->CreateConstantBufferView(&cbv,
						context.GetDescriptorHeap().GetD3D12CPUDescriptorHandleForResource(allocatedDescriptor));
				}
				else
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};

					if (type == BufferType::StructuredBuffer)
					{
						srv.Format = DXGI_FORMAT_UNKNOWN;
						srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
						srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
						srv.Buffer.FirstElement = 0;
						srv.Buffer.NumElements = numElements;
						srv.Buffer.StructureByteStride = stride;
						srv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
					}
					else if (type == BufferType::RawBuffer)
					{
						srv.Format = DXGI_FORMAT_R32_TYPELESS;
						srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
						srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
						srv.Buffer.FirstElement = 0;
						srv.Buffer.NumElements = uint32_t(size) / sizeof(uint32_t);
						srv.Buffer.StructureByteStride = 0;
						srv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
					}

					context.GetD3D12Device()->CreateShaderResourceView(this->d3d12Resource.at(0).Get(), &srv,
						context.GetDescriptorHeap().GetD3D12CPUDescriptorHandleForResource(allocatedDescriptor));
				}
				this->descriptor.push_back(allocatedDescriptor);
			}
		}

		if (usage == BufferUsage::UnorderedAccess)
		{
			this->unorderedAccessDescriptor = context.GetDescriptorHeap().AllocatePersistentResource();
			if (this->unorderedAccessDescriptor.IsNull())
			{
				Log(LogType::Error, ToString(errStr, "Failed to allocate descriptor."));
				Unload();
				return false;
			}

			D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {};

			if (type == BufferType::StructuredBuffer)
			{
				uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				uav.Format = DXGI_FORMAT_UNKNOWN;
				uav.Buffer.FirstElement = 0;
				uav.Buffer.NumElements = numElements;
				uav.Buffer.StructureByteStride = stride;
				uav.Buffer.CounterOffsetInBytes = 0;
				uav.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
			}
			else if (type == BufferType::RawBuffer)
			{
				uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				uav.Format = DXGI_FORMAT_R32_TYPELESS;
				uav.Buffer.FirstElement = 0;
				uav.Buffer.NumElements = uint32_t(size) / sizeof(uint32_t);
				uav.Buffer.StructureByteStride = 0;
				uav.Buffer.CounterOffsetInBytes = 0;
				uav.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
			}
			else
			{
				throw std::invalid_argument("This should be impossible.");
			}

			context.GetD3D12Device()->CreateUnorderedAccessView(this->d3d12Resource.at(0).Get(), nullptr, &uav,
				context.GetDescriptorHeap().GetD3D12CPUDescriptorHandleForResource(this->unorderedAccessDescriptor));
		}
#endif

		this->isLoaded = true;
		this->context = &context;
		this->type = type;
		this->usage = usage;
		this->size = size;
		this->stride = stride;
		this->numElements = numElements;
		return true;
	}

	bool Buffer::LoadAsVertexBuffer(const IGLOContext& context, uint32_t vertexStride, uint32_t numVertices, BufferUsage usage)
	{
		return InternalLoad(context, uint64_t(vertexStride) * uint64_t(numVertices), vertexStride, numVertices, usage, BufferType::VertexBuffer);
	}
	bool Buffer::LoadAsIndexBuffer(const IGLOContext& context, IndexFormat format, uint32_t numIndices, BufferUsage usage)
	{
		uint32_t indexStride = 0;

		if (format == IndexFormat::UINT16)
		{
			indexStride = 2;
		}
		else if (format == IndexFormat::UINT32)
		{
			indexStride = 4;
		}
		else
		{
			Log(LogType::Error, "Failed to create index buffer. Reason: Invalid format.");
			return false;
		}

		return InternalLoad(context, uint64_t(indexStride) * uint64_t(numIndices), indexStride, numIndices, usage, BufferType::IndexBuffer);
	}
	bool Buffer::LoadAsStructuredBuffer(const IGLOContext& context, uint32_t elementStride, uint32_t numElements, BufferUsage usage)
	{
		return InternalLoad(context, uint64_t(elementStride) * uint64_t(numElements), elementStride, numElements, usage, BufferType::StructuredBuffer);
	}
	bool Buffer::LoadAsRawBuffer(const IGLOContext& context, uint64_t numBytes, BufferUsage usage)
	{
		return InternalLoad(context, numBytes, 0, 0, usage, BufferType::RawBuffer);
	}
	bool Buffer::LoadAsShaderConstant(const IGLOContext& context, uint64_t numBytes, BufferUsage usage)
	{
		return InternalLoad(context, numBytes, 0, 0, usage, BufferType::ShaderConstant);
	}

	void Buffer::SetData(CommandList& cmd, void* srcData)
	{
		bool usageOK = (usage == BufferUsage::Default || usage == BufferUsage::UnorderedAccess);
		if (!isLoaded || !usageOK)
		{
			Log(LogType::Error, "Failed to set buffer data. Reason: Buffer must be created with Default or UnorderedAccess usage.");
			return;
		}

		TempBuffer tempBuffer = context->GetBufferAllocator().AllocateTempBuffer(size, 0);
		if (tempBuffer.IsNull())
		{
			Log(LogType::Error, "Failed to set buffer data. Reason: Failed to allocate temporary buffer.");
			return;
		}

		memcpy(tempBuffer.data, srcData, size);

		cmd.CopyTempBufferToBuffer(tempBuffer, *this);
	}

	void Buffer::SetDynamicData(void* srcData)
	{
		if (!isLoaded || usage != BufferUsage::Dynamic)
		{
			Log(LogType::Error, "Failed to set dynamic buffer data. Reason: Buffer must be created with Dynamic usage.");
			return;
		}

		dynamicSetCounter++;
		if (dynamicSetCounter >= context->GetNumFramesInFlight()) dynamicSetCounter = 0;

		if (!mapped.at(dynamicSetCounter)) throw std::runtime_error("This should be impossible.");

		memcpy(mapped[dynamicSetCounter], srcData, size);
	}

	void Buffer::ReadData(void* destData)
	{
		if (!isLoaded || usage != BufferUsage::Readable)
		{
			Log(LogType::Error, "Failed to read buffer data. Reason: Buffer must be created with Readable usage.");
			return;
		}

		uint32_t frameIndex = context->GetFrameIndex();
		if (!mapped.at(frameIndex)) throw std::runtime_error("This should be impossible.");

		memcpy(destData, mapped[frameIndex], size);
	}

	uint32_t Texture::GetColorChannelCount() const
	{
		return GetFormatInfo(format).elementCount;
	}

	void Texture::Unload()
	{
		// Free descriptors
		if (!descriptor.IsNull())
		{
			if (context) context->GetDescriptorHeap().FreePersistentResource(descriptor);
			else Log(LogType::Error, "Texture descriptor memory leak detected! This should be impossible.");
		}
		descriptor.SetToNull();

		if (!stencilDescriptor.IsNull())
		{
			if (context) context->GetDescriptorHeap().FreePersistentResource(stencilDescriptor);
			else Log(LogType::Error, "Texture descriptor memory leak detected! This should be impossible.");
		}
		stencilDescriptor.SetToNull();

		if (!unorderedAccessDescriptor.IsNull())
		{
			if (context) context->GetDescriptorHeap().FreePersistentResource(unorderedAccessDescriptor);
			else Log(LogType::Error, "Texture descriptor memory leak detected! This should be impossible.");
		}
		unorderedAccessDescriptor.SetToNull();

#ifdef IGLO_D3D12
		for (size_t i = 0; i < d3d12Resource.size(); i++)
		{
			if (usage == TextureUsage::Readable)
			{
				D3D12_RANGE writeRange = { 0,0 };
				d3d12Resource[i]->Unmap(0, &writeRange);
			}
		}
		d3d12Resource.clear();
		d3d12Resource.shrink_to_fit();
#endif

		isLoaded = false;
		context = nullptr;
		usage = TextureUsage::Default;
		width = 0;
		height = 0;
		format = Format::None;
		msaa = MSAA::Disabled;
		isCubemap = false;
		numFaces = 0;
		numMipLevels = 0;

		readMapped.clear();
		readMapped.shrink_to_fit();
	}

	Texture::Texture(Texture&& other) noexcept
	{
		*this = std::move(other);
	}
	Texture& Texture::operator=(Texture&& other) noexcept
	{
		Unload();

		std::swap(this->isLoaded, other.isLoaded);
		std::swap(this->context, other.context);
		std::swap(this->usage, other.usage);
		std::swap(this->width, other.width);
		std::swap(this->height, other.height);
		std::swap(this->format, other.format);
		std::swap(this->msaa, other.msaa);
		std::swap(this->isCubemap, other.isCubemap);
		std::swap(this->numFaces, other.numFaces);
		std::swap(this->numMipLevels, other.numMipLevels);
		std::swap(this->descriptor, other.descriptor);
		std::swap(this->stencilDescriptor, other.stencilDescriptor);
		std::swap(this->unorderedAccessDescriptor, other.unorderedAccessDescriptor);
		std::swap(this->readMapped, other.readMapped);

#ifdef IGLO_D3D12
		std::swap(this->d3d12Resource, other.d3d12Resource);
#endif 

		return *this;
	}

	const Descriptor* Texture::GetDescriptor() const
	{
		if (!isLoaded) return nullptr;
		if (descriptor.IsNull()) return nullptr;
		return &descriptor;
	}

	const Descriptor* Texture::GetStencilDescriptor() const
	{
		if (!isLoaded) return nullptr;
		if (stencilDescriptor.IsNull()) return nullptr;
		return &stencilDescriptor;
	}

	const Descriptor* Texture::GetUnorderedAccessDescriptor() const
	{
		if (!isLoaded) return nullptr;
		if (unorderedAccessDescriptor.IsNull()) return nullptr;
		return &unorderedAccessDescriptor;
	}

#ifdef IGLO_D3D12
	ID3D12Resource* Texture::GetD3D12Resource() const
	{
		if (!isLoaded) return nullptr;
		if (d3d12Resource.size() == 0) throw std::runtime_error("This should be impossible. A loaded texture must always contain atleast one resource.");
		switch (usage)
		{
		case TextureUsage::Default:
		case TextureUsage::UnorderedAccess:
		case TextureUsage::RenderTexture:
		case TextureUsage::DepthBuffer:
			return d3d12Resource.at(0).Get();
		case TextureUsage::Readable:
			return d3d12Resource.at(context->GetFrameIndex()).Get();
		default:
			throw std::runtime_error("This should be impossible. Invalid texture usage.");
		}
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC Texture::GenerateD3D12DSVDesc() const
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
			}
			else
			{
				out.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
				out.Texture2DMSArray.ArraySize = numFaces;
			}
		}
		else
		{
			if (msaa == MSAA::Disabled)
			{
				out.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			}
			else
			{
				out.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
			}
		}

		return out;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC Texture::GenerateD3D12UAVDesc() const
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
			}
			else
			{
				out.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
				out.Texture2DMSArray.ArraySize = numFaces;
			}
		}
		else
		{
			if (msaa == MSAA::Disabled)
			{
				out.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			}
			else
			{
				out.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
			}
		}

		return out;
	}
#endif

	bool Texture::Load(const IGLOContext& context, uint32_t width, uint32_t height, Format format, TextureUsage usage,
		MSAA msaa, uint32_t numFaces, uint32_t numMipLevels, bool isCubemap, ClearValue optimizedClearValue,
		BarrierLayout* optionalInitLayout, bool createDescriptors)
	{
		Unload();

		const char* errStr = "Failed to create texture. Reason: ";

		switch (usage)
		{
		case TextureUsage::Default:
		case TextureUsage::Readable:
		case TextureUsage::UnorderedAccess:
		case TextureUsage::RenderTexture:
		case TextureUsage::DepthBuffer:
			break;
		default:
			Log(LogType::Error, ToString(errStr, "Invalid texture usage."));
			return false;
		}

		if (width == 0 || height == 0)
		{
			Log(LogType::Error, ToString(errStr, "Texture dimensions must be atleast 1x1."));
			return false;
		}
		if (numFaces == 0 || numMipLevels == 0)
		{
			Log(LogType::Error, ToString(errStr, "Texture must have atleast 1 face and 1 mip level."));
			return false;
		}
		if (format == Format::None)
		{
			Log(LogType::Error, ToString(errStr, "Invalid texture format."));
			return false;
		}
		if (isCubemap && numFaces % 6 != 0)
		{
			Log(LogType::Error, ToString(errStr, "For cubemap textures, number of faces must be a multiple of 6."));
			return false;
		}

		this->context = &context;
		this->usage = usage;
		this->width = width;
		this->height = height;
		this->format = format;
		this->msaa = msaa;
		this->isCubemap = isCubemap;
		this->numFaces = numFaces;
		this->numMipLevels = numMipLevels;

		bool isDepthFormat = GetFormatInfo(format).isDepthFormat;

#ifdef IGLO_D3D12
		FormatInfoDXGI formatInfoD3D = GetFormatInfoDXGI(format);
		if (formatInfoD3D.dxgiFormat == DXGI_FORMAT_UNKNOWN)
		{
			Log(LogType::Error, ToString(errStr, "This iglo format is not supported in D3D12: ", (int)format, "."));
			Unload();
			return false;
		}

		this->d3d12Resource.clear();
		this->d3d12Resource.shrink_to_fit();
		if (usage == TextureUsage::Readable)
		{
			this->d3d12Resource.resize(context.GetNumFramesInFlight(), nullptr);
		}
		else
		{
			this->d3d12Resource.resize(1, nullptr);
		}

		this->readMapped.clear();
		this->readMapped.shrink_to_fit();

		for (size_t i = 0; i < this->d3d12Resource.size(); i++)
		{
			D3D12_RESOURCE_DESC1 resDesc = {};

			DXGI_FORMAT resourceFormat = formatInfoD3D.dxgiFormat;
			if (isDepthFormat) resourceFormat = formatInfoD3D.typeless;

			resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			resDesc.Alignment = 0;
			resDesc.Width = width;
			resDesc.Height = height;
			resDesc.DepthOrArraySize = numFaces;
			resDesc.MipLevels = numMipLevels;
			resDesc.Format = resourceFormat;
			resDesc.SampleDesc.Count = (UINT)msaa;
			resDesc.SampleDesc.Quality = 0;  // 0 = MSAA. Driver specific.
			resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

			resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			if (usage == TextureUsage::UnorderedAccess) resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			if (usage == TextureUsage::RenderTexture) resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			if (usage == TextureUsage::DepthBuffer) resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			resDesc.SamplerFeedbackMipRegion.Width = 0;
			resDesc.SamplerFeedbackMipRegion.Height = 0;
			resDesc.SamplerFeedbackMipRegion.Depth = 0;

			if (usage == TextureUsage::Readable)
			{
				UINT64 requiredBufferSize = 0;
				context.GetD3D12Device()->GetCopyableFootprints1(&resDesc, 0, numFaces * numMipLevels, 0, nullptr, nullptr, nullptr, &requiredBufferSize);
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

			D3D12_HEAP_PROPERTIES heapProp = {};
			heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProp.CreationNodeMask = 1;
			heapProp.VisibleNodeMask = 1;

			heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
			if (usage == TextureUsage::Readable) heapProp.Type = D3D12_HEAP_TYPE_READBACK;

			D3D12_BARRIER_LAYOUT initLayout;
			switch (usage)
			{
			default:
				initLayout = (D3D12_BARRIER_LAYOUT)BarrierLayout::_GraphicsQueue_CopyDest;
				break;

			case TextureUsage::Readable:
				initLayout = (D3D12_BARRIER_LAYOUT)BarrierLayout::Undefined;
				break;

			case TextureUsage::UnorderedAccess:
				initLayout = (D3D12_BARRIER_LAYOUT)BarrierLayout::_GraphicsQueue_UnorderedAccess;
				break;

			case TextureUsage::RenderTexture:
				initLayout = (D3D12_BARRIER_LAYOUT)BarrierLayout::RenderTarget;
				break;

			case TextureUsage::DepthBuffer:
				initLayout = (D3D12_BARRIER_LAYOUT)BarrierLayout::DepthStencilWrite;
				break;
			}
			if (optionalInitLayout) initLayout = (D3D12_BARRIER_LAYOUT)(*optionalInitLayout);

			const D3D12_CLEAR_VALUE* ptrClearValue = nullptr;
			D3D12_CLEAR_VALUE clear = {};
			if (usage == TextureUsage::RenderTexture)
			{
				clear.Format = resDesc.Format;
				clear.Color[0] = optimizedClearValue.color.red;
				clear.Color[1] = optimizedClearValue.color.green;
				clear.Color[2] = optimizedClearValue.color.blue;
				clear.Color[3] = optimizedClearValue.color.alpha;
				ptrClearValue = &clear;
			}
			else if (usage == TextureUsage::DepthBuffer)
			{
				clear.Format = formatInfoD3D.dxgiFormat;
				clear.DepthStencil.Depth = optimizedClearValue.depth;
				clear.DepthStencil.Stencil = optimizedClearValue.stencil;
				ptrClearValue = &clear;
			}

			HRESULT hr = context.GetD3D12Device()->CreateCommittedResource3(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
				initLayout, ptrClearValue, nullptr, 0, nullptr, IID_PPV_ARGS(&this->d3d12Resource.at(i)));
			if (FAILED(hr))
			{
				Log(LogType::Error, ToString(errStr, "ID3D12Device10::CreateCommittedResource3 returned error code: ", (uint32_t)hr, "."));
				Unload();
				return false;
			}

			// Map resources
			if (usage == TextureUsage::Readable)
			{
				void* mappedPtr = nullptr;
				hr = this->d3d12Resource.at(i)->Map(0, nullptr, &mappedPtr);
				if (FAILED(hr))
				{
					Log(LogType::Error, ToString(errStr, "ID3D12Resource::Map returned error code: ", (uint32_t)hr, "."));
					Unload();
					return false;
				}
				this->readMapped.push_back(mappedPtr);
			}
		}

		bool isMultisampledCubemap = (isCubemap && msaa != MSAA::Disabled);
		bool hasStencil = (formatInfoD3D.dxgiFormat == DXGI_FORMAT_D24_UNORM_S8_UINT || formatInfoD3D.dxgiFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT);

		// Multisampled cubemaps and readable textures can't have SRVs
		bool createSRV = (!isMultisampledCubemap && usage != TextureUsage::Readable);
		bool createUAV = (usage == TextureUsage::UnorderedAccess);

		if (!createDescriptors)
		{
			createSRV = false;
			createUAV = false;
		}

		// Allocate descriptors
		this->descriptor.SetToNull();
		this->stencilDescriptor.SetToNull();
		this->unorderedAccessDescriptor.SetToNull();
		bool failedToAllocateDescriptors = false;
		if (createSRV)
		{
			this->descriptor = context.GetDescriptorHeap().AllocatePersistentResource();
			if (this->descriptor.IsNull()) failedToAllocateDescriptors = true;
			if (hasStencil)
			{
				this->stencilDescriptor = context.GetDescriptorHeap().AllocatePersistentResource();
				if (this->stencilDescriptor.IsNull()) failedToAllocateDescriptors = true;
			}
		}
		if (createUAV)
		{
			this->unorderedAccessDescriptor = context.GetDescriptorHeap().AllocatePersistentResource();
			if (this->unorderedAccessDescriptor.IsNull()) failedToAllocateDescriptors = true;
		}
		if (failedToAllocateDescriptors)
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate descriptors."));
			Unload();
			return false;
		}

		if (createUAV)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {};

			uav.ViewDimension = D3D12_UAV_DIMENSION_UNKNOWN;
			if (msaa == MSAA::Disabled)
			{
				if (numFaces == 1)
				{
					uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				}
				else
				{
					uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
					uav.Texture2DArray.ArraySize = numFaces;
				}
			}
			else
			{
				if (numFaces == 1)
				{
					uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
				}
				else
				{
					uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
					uav.Texture2DMSArray.ArraySize = numFaces;
				}
			}

			uav.Format = formatInfoD3D.dxgiFormat;

			context.GetD3D12Device()->CreateUnorderedAccessView(this->d3d12Resource.at(0).Get(), nullptr, &uav,
				context.GetDescriptorHeap().GetD3D12CPUDescriptorHandleForResource(this->unorderedAccessDescriptor));
		}

		if (createSRV)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
			srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			srv.ViewDimension = D3D12_SRV_DIMENSION_UNKNOWN;

			UINT* planeSlice = nullptr; // stencil SRV needs to know where the plane slice variable is located
			if (msaa == MSAA::Disabled)
			{
				if (isCubemap)
				{
					if (numFaces == 6)
					{
						srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
						srv.TextureCube.MipLevels = numMipLevels;
					}
					else
					{
						srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
						srv.TextureCubeArray.MipLevels = numMipLevels;
						srv.TextureCubeArray.NumCubes = numFaces / 6;
					}
				}
				else
				{
					if (numFaces == 1)
					{
						srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
						srv.Texture2D.MipLevels = numMipLevels;
						planeSlice = &srv.Texture2D.PlaneSlice;
					}
					else
					{
						srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
						srv.Texture2DArray.MipLevels = numMipLevels;
						srv.Texture2DArray.ArraySize = numFaces;
						planeSlice = &srv.Texture2DArray.PlaneSlice;
					}
				}
			}
			else
			{
				if (numFaces == 1)
				{
					srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
				}
				else
				{
					srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
					srv.Texture2DMSArray.ArraySize = numFaces;
				}
			}

			srv.Format = formatInfoD3D.dxgiFormat;

			// Format for depth component
			if (formatInfoD3D.dxgiFormat == DXGI_FORMAT_D16_UNORM) srv.Format = DXGI_FORMAT_R16_UNORM;
			else if (formatInfoD3D.dxgiFormat == DXGI_FORMAT_D24_UNORM_S8_UINT) srv.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			else if (formatInfoD3D.dxgiFormat == DXGI_FORMAT_D32_FLOAT) srv.Format = DXGI_FORMAT_R32_FLOAT;
			else if (formatInfoD3D.dxgiFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT) srv.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

			// Create SRV
			context.GetD3D12Device()->CreateShaderResourceView(this->d3d12Resource.at(0).Get(), &srv,
				context.GetDescriptorHeap().GetD3D12CPUDescriptorHandleForResource(this->descriptor));

			// Create stencil SRV
			if (hasStencil)
			{
				// Format for stencil component
				if (formatInfoD3D.dxgiFormat == DXGI_FORMAT_D24_UNORM_S8_UINT) srv.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
				else srv.Format = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
				if (planeSlice) *planeSlice = 1;

				context.GetD3D12Device()->CreateShaderResourceView(this->d3d12Resource.at(0).Get(), &srv,
					context.GetDescriptorHeap().GetD3D12CPUDescriptorHandleForResource(this->stencilDescriptor));
			}
		}

#endif
		this->isLoaded = true;
		return true;
	}

	bool Texture::LoadAsWrapped(const IGLOContext& context,
#ifdef IGLO_D3D12
		std::vector<ComPtr<ID3D12Resource>> resources,
#endif
		uint32_t width, uint32_t height, Format format, TextureUsage usage,
		MSAA msaa, bool isCubemap, uint32_t numFaces, uint32_t numMipLevels,
		Descriptor srvDescriptor, Descriptor srvStencilDescriptor, Descriptor uavDescriptor,
		std::vector<void*> mappedPtrForReadableUsage)
	{
		Unload();

		const char* errStr = "Failed to load wrapped texture. Reason: ";

		switch (usage)
		{
		case TextureUsage::Default:
		case TextureUsage::Readable:
		case TextureUsage::UnorderedAccess:
		case TextureUsage::RenderTexture:
		case TextureUsage::DepthBuffer:
			break;
		default:
			Log(LogType::Error, "Invalid texture usage.");
			return false;
		}

		size_t expectedResourceCount = 1;
		if (usage == TextureUsage::Readable) expectedResourceCount = context.GetNumFramesInFlight();

#ifdef IGLO_D3D12
		if (resources.size() != expectedResourceCount)
		{
			Log(LogType::Error, ToString(errStr, "Expected ", expectedResourceCount, " resources, not ", resources.size(), "."));
			return false;
		}
#endif
#ifdef IGLO_VULKAN
		//TODO: this
#endif

		if (usage == TextureUsage::Readable)
		{
			if (mappedPtrForReadableUsage.size() != expectedResourceCount)
			{
				Log(LogType::Error, ToString(errStr, "Expected ", expectedResourceCount,
					" mapped pointers for readable texture usage, not ", mappedPtrForReadableUsage.size(), "."));
				return false;
			}
		}
		else
		{
			if (mappedPtrForReadableUsage.size() > 0)
			{
				Log(LogType::Error, ToString(errStr, "Expected 0 mapped pointers, since texture usage isn't Readable."));
				return false;
			}
		}

		this->isLoaded = true;
		this->context = &context;
		this->usage = usage;
		this->width = width;
		this->height = height;
		this->format = format;
		this->msaa = msaa;
		this->isCubemap = isCubemap;
		this->numFaces = numFaces;
		this->numMipLevels = numMipLevels;
		this->descriptor = descriptor;
		this->stencilDescriptor = stencilDescriptor;
		this->unorderedAccessDescriptor = unorderedAccessDescriptor;
		this->readMapped = mappedPtrForReadableUsage;

#ifdef IGLO_D3D12
		this->d3d12Resource = resources;
#endif

		return true;
	}

	bool Texture::LoadFromFile(const IGLOContext& context, CommandList& cmd, const std::string& filename, bool generateMipmaps, bool sRGB)
	{
		Unload();
		ReadFileResult file = ReadFile(filename);
		if (!file.success)
		{
			Log(LogType::Error, "Failed to load texture from file. Reason: Couldn't open '" + filename + "'.");
			return false;
		}
		if (file.fileContent.size() == 0)
		{
			Log(LogType::Error, "Failed to load texture from file. Reason: File '" + filename + "' is empty.");
			return false;
		}
		return LoadFromMemory(context, cmd, file.fileContent.data(), file.fileContent.size(), generateMipmaps, sRGB);
	}

	bool Texture::LoadFromMemory(const IGLOContext& context, CommandList& cmd, const byte* fileData, size_t numBytes, bool generateMipmaps, bool sRGB)
	{
		Unload();
		Image image;
		if (!image.LoadFromMemory(fileData, numBytes, false)) return false;
		if (sRGB)
		{
			// User has requested to use sRGB format.
			image.SetSRGB(true);
			if (!image.IsSRGB())
			{
				Log(LogType::Warning, "A texture was unable to use an sRGB format as requested,"
					" because no sRGB equivalent was found for this iglo format: " + std::to_string((int)image.GetFormat()) + ".");
			}
		}
		return LoadFromMemory(context, cmd, image, generateMipmaps);
	}

	bool Texture::LoadFromMemory(const IGLOContext& context, CommandList& cmd, const Image& image, bool generateMipmaps)
	{
		Unload();

		const char* errStr = "Failed to create texture from image. Reason: ";

		if (!image.IsLoaded())
		{
			Log(LogType::Error, ToString(errStr, "The provided image is not loaded."));
			return false;
		}

		uint32_t maxMipLevels = Image::CalculateNumMips(image.GetWidth(), image.GetHeight());
		bool fillMissingMips = false;
		if (generateMipmaps) // User requested max mip levels
		{
			if (maxMipLevels > image.GetNumMipLevels()) // Image doesn't have maximum mips
			{
				fillMissingMips = true;
			}
		}

		// Check if mipmap generation is possible
		if (fillMissingMips)
		{
			const char* mipErrStr = "Unable to generate mipmaps for texture. Reason: ";
			if (GetFormatInfo(image.GetFormat()).blockSize != 0)
			{
				fillMissingMips = false;
				Log(LogType::Warning, ToString(mipErrStr, "Mipmap generation is not supported for block compression formats."));
			}
			else if (image.GetNumFaces() > 1)
			{
				fillMissingMips = false;
				Log(LogType::Warning, ToString(mipErrStr, "Mipmap generation is not yet supported for cube maps and texture arrays."));
			}
			else if (!IsPowerOf2(image.GetWidth()) || !IsPowerOf2(image.GetHeight()))
			{
				fillMissingMips = false;
				Log(LogType::Warning, ToString(mipErrStr, "Mipmap generation is not yet supported for non power of 2 textures."));
			}
			else if (cmd.GetCommandListType() == CommandListType::Copy)
			{
				fillMissingMips = false;
				Log(LogType::Warning, ToString(mipErrStr, "Mipmap generation can't be performed on a 'Copy' command list type."));
			}
		}

		if (fillMissingMips && image.GetNumMipLevels() > 1)
		{
			Log(LogType::Warning, "The texture you requested to generate mipmaps for already has some mipmaps, but not a full amount."
				" The existing mipmaps will be replaced with newly generated ones.");
		}

		BarrierLayout initLayout = BarrierLayout::Undefined;
		if (cmd.GetCommandListType() == CommandListType::Graphics)
		{
			initLayout = BarrierLayout::_GraphicsQueue_CopyDest;
		}
		else if (cmd.GetCommandListType() == CommandListType::Compute)
		{
			initLayout = BarrierLayout::_ComputeQueue_CopyDest;
		}
		else
		{
			initLayout = BarrierLayout::Common; // Copy queue requires common layout
		}

		// Create texture with expected number of mipmaps
		if (!Load(context, image.GetWidth(), image.GetHeight(), image.GetFormat(), TextureUsage::Default, MSAA::Disabled,
			image.GetNumFaces(), fillMissingMips ? maxMipLevels : image.GetNumMipLevels(), image.IsCubemap(), ClearValue(),
			&initLayout))
		{
			return false;
		}

		// Generate missing mipmaps using a compute shader
		if (fillMissingMips)
		{
			if (maxMipLevels <= 1) throw std::runtime_error("impossible");

			// Create an unordered access texture with one less miplevel
			Extent2D nextMipDimensions = Image::CalculateMipExtent(1, image.GetWidth(), image.GetHeight());

			BarrierLayout unorderedInitLayout = BarrierLayout::_GraphicsQueue_UnorderedAccess;
			if (cmd.GetCommandListType() == CommandListType::Compute) unorderedInitLayout = BarrierLayout::_ComputeQueue_UnorderedAccess;

			std::shared_ptr<Texture> unordered = std::make_shared<Texture>();
			if (!unordered->Load(context, nextMipDimensions.width, nextMipDimensions.height, image.GetFormat(),
				TextureUsage::UnorderedAccess, MSAA::Disabled, image.GetNumFaces(), maxMipLevels - 1, false, ClearValue(),
				&unorderedInitLayout, false))
			{
				Log(LogType::Error, ToString(errStr, "Failed to create unordered access texture for mipmap generation."));
				Unload();
				return false;
			}

			// Upload the first and largest mipmap to this texture
			SetPixelsAtSubresource(cmd, image, 0, 0);

			struct MipmapGenPushConstants
			{
				uint32_t srcTextureIndex = IGLO_UINT32_MAX;
				uint32_t destTextureIndex = IGLO_UINT32_MAX;
				uint32_t bilinearClampSamplerIndex = IGLO_UINT32_MAX;
				uint32_t padding = 0;
				Vector2 inverseDestTextureSize;
			};
			MipmapGenPushConstants pushConstants;
			pushConstants.bilinearClampSamplerIndex = context.GetBilinearClampSamplerDescriptor()->heapIndex;

			for (uint32_t i = 0; i < maxMipLevels - 1; i++)
			{
				Descriptor srv = context.GetDescriptorHeap().AllocateTemporaryResource();
				Descriptor uav = context.GetDescriptorHeap().AllocateTemporaryResource();
				if (srv.IsNull() || uav.IsNull())
				{
					Log(LogType::Error, ToString(errStr, "Failed to allocate temporary resource descriptors for mipmap generation."));
					Unload();
					return false;
				}
				pushConstants.srcTextureIndex = srv.heapIndex;
				pushConstants.destTextureIndex = uav.heapIndex;

				Extent2D destDimensions = Image::CalculateMipExtent(i + 1, image.GetWidth(), image.GetHeight());
				pushConstants.inverseDestTextureSize = Vector2(1.0f / (float)destDimensions.width, 1.0f / (float)destDimensions.height);

#ifdef IGLO_D3D12
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc.Format = GetFormatInfoDXGI(image.GetFormat()).dxgiFormat;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;
				srvDesc.Texture2D.MostDetailedMip = i;
				context.GetD3D12Device()->CreateShaderResourceView(this->GetD3D12Resource(), &srvDesc,
					context.GetDescriptorHeap().GetD3D12CPUDescriptorHandleForResource(srv));

				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = GetFormatInfoDXGI(image.GetFormat()).dxgiFormat;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				uavDesc.Texture2D.MipSlice = i;
				context.GetD3D12Device()->CreateUnorderedAccessView(unordered->GetD3D12Resource(), nullptr, &uavDesc,
					context.GetDescriptorHeap().GetD3D12CPUDescriptorHandleForResource(uav));
#endif

				cmd.AddTextureBarrierAtSubresource(*this, SimpleBarrier::CopyDest, SimpleBarrier::ComputeShaderResource, 0, i, true);
				cmd.FlushBarriers();

				cmd.SetPipeline(context.GetMipmapGenerationPipeline());
				cmd.SetComputePushConstants(&pushConstants, sizeof(pushConstants));
				cmd.DispatchCompute(
					std::max(destDimensions.width / 8, 1U),
					std::max(destDimensions.height / 8, 1U),
					1);

				cmd.AddTextureBarrierAtSubresource(*this, SimpleBarrier::ComputeShaderResource, SimpleBarrier::PixelShaderResource, 0, i);
				cmd.AddTextureBarrierAtSubresource(*unordered, SimpleBarrier::ComputeShaderUnorderedAccess, SimpleBarrier::CopySource, 0, i);
				cmd.FlushBarriers();

				cmd.CopyTextureSubresource(*unordered, 0, i, *this, 0, i + 1);
			}

			cmd.AddTextureBarrierAtSubresource(*this, SimpleBarrier::CopyDest, SimpleBarrier::PixelShaderResource, 0, maxMipLevels - 1);
			cmd.FlushBarriers();

			// Unload the unordered texture only when it's safe to do so.
			context.DelayedTextureUnload(unordered);
			unordered = nullptr;
		}
		else
		{
			SetPixels(cmd, image);

			if (cmd.GetCommandListType() != CommandListType::Copy)
			{
				cmd.AddTextureBarrier(*this, SimpleBarrier::CopyDest, SimpleBarrier::PixelShaderResource);
				cmd.FlushBarriers();
			}
		}

		return true;
	}

	void Texture::SetPixels(CommandList& cmd, const Image& srcImage)
	{
		const char* errStr = "Failed to set texture pixels. Reason: ";
		if (!isLoaded || usage == TextureUsage::Readable)
		{
			Log(LogType::Error, ToString(errStr, "Texture must be created with non-readable texture usage."));
			return;
		}
		if (msaa != MSAA::Disabled)
		{
			Log(LogType::Error, ToString(errStr, "Texture is multisampled."));
			return;
		}
		if (!srcImage.IsLoaded() ||
			srcImage.GetWidth() != width ||
			srcImage.GetHeight() != height ||
			srcImage.GetFormat() != format ||
			srcImage.GetNumFaces() != numFaces ||
			srcImage.GetNumMipLevels() != numMipLevels)
		{
			Log(LogType::Error, ToString(errStr, "Source image dimensions must match texture."));
			return;
		}

#ifdef IGLO_D3D12
		uint64_t requiredUploadBufferSize = 0;
		{
			D3D12_RESOURCE_DESC resDesc = d3d12Resource.at(0)->GetDesc();
			context->GetD3D12Device()->GetCopyableFootprints(&resDesc, 0, numFaces * numMipLevels, 0, nullptr, nullptr, nullptr, &requiredUploadBufferSize);
		}
#endif
#ifdef IGLO_VULKAN
		uint64_t requiredUploadBufferSize = 0; //TODO: this
#endif

		TempBuffer tempBuffer = context->GetBufferAllocator().AllocateTempBuffer(requiredUploadBufferSize,
			(uint32_t)BufferPlacementAlignment::Texture);
		if (tempBuffer.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate temporary buffer."));
			return;
		}

		byte* destPtr = (byte*)tempBuffer.data;
		byte* srcPtr = (byte*)srcImage.GetPixels();
		for (uint32_t faceIndex = 0; faceIndex < srcImage.GetNumFaces(); faceIndex++)
		{
			for (uint32_t mipIndex = 0; mipIndex < srcImage.GetNumMipLevels(); mipIndex++)
			{
				size_t srcRowPitch = srcImage.GetMipRowPitch(mipIndex);
				uint64_t destRowPitch = AlignUp(srcRowPitch, (uint64_t)BufferPlacementAlignment::TextureRowPitch);
				for (uint64_t srcProgress = 0; srcProgress < srcImage.GetMipSize(mipIndex); srcProgress += srcRowPitch)
				{
					memcpy(destPtr, srcPtr, srcRowPitch);
					srcPtr += srcRowPitch;
					destPtr += destRowPitch;
#ifdef _DEBUG
					if (srcProgress + srcRowPitch > srcImage.GetMipSize(mipIndex)) throw std::runtime_error("Something is wrong here.");
#endif
				}
			}
		}

		cmd.CopyTempBufferToTexture(tempBuffer, *this);
	}

	void Texture::SetPixels(CommandList& cmd, const void* pixelData)
	{
		if (!pixelData)
		{
			Log(LogType::Error, "Failed to set texture pixels. Reason: The provided data is nullptr.");
			return;
		}
		Image image;
		image.LoadAsPointer((byte*)pixelData, width, height, format, numMipLevels, numFaces, isCubemap);
		SetPixels(cmd, image);
	}

	void Texture::SetPixelsAtSubresource(CommandList& cmd, const Image& srcImage, uint32_t destFaceIndex, uint32_t destMipIndex)
	{
		const char* errStr = "Failed to set texture pixels at subresource. Reason: ";
		if (!isLoaded || usage == TextureUsage::Readable)
		{
			Log(LogType::Error, ToString(errStr, "Texture must be created with non-readable texture usage."));
			return;
		}
		if (msaa != MSAA::Disabled)
		{
			Log(LogType::Error, ToString(errStr, "Texture is multisampled."));
			return;
		}
		Extent2D subResDimensions = Image::CalculateMipExtent(destMipIndex, width, height);
		if (!srcImage.IsLoaded() ||
			srcImage.GetWidth() != subResDimensions.width ||
			srcImage.GetHeight() != subResDimensions.height ||
			srcImage.GetFormat() != format ||
			srcImage.GetNumFaces() != 1 ||
			srcImage.GetNumMipLevels() != 1)
		{
			Log(LogType::Error, ToString(errStr, "Source image is expected to have 1 face, 1 miplevel,"
				" and the dimensions ", subResDimensions.ToString(), "."));
			return;
		}

#ifdef IGLO_D3D12
		uint32_t subResourceIndex = (destFaceIndex * numMipLevels) + destMipIndex;
		uint64_t requiredUploadBufferSize = 0;
		{
			D3D12_RESOURCE_DESC resDesc = d3d12Resource.at(0)->GetDesc();
			context->GetD3D12Device()->GetCopyableFootprints(&resDesc, subResourceIndex, 1, 0, nullptr, nullptr, nullptr, &requiredUploadBufferSize);
		}
#endif
#ifdef IGLO_VULKAN
		uint64_t requiredUploadBufferSize = 0; //TODO: this
#endif

		TempBuffer tempBuffer = context->GetBufferAllocator().AllocateTempBuffer(requiredUploadBufferSize,
			(uint32_t)BufferPlacementAlignment::Texture);
		if (tempBuffer.IsNull())
		{
			Log(LogType::Error, "Failed to set texture pixels. Reason: Failed to allocate temporary buffer.");
			return;
		}

		byte* destPtr = (byte*)tempBuffer.data;
		byte* srcPtr = (byte*)srcImage.GetPixels();
		size_t srcRowPitch = srcImage.GetMipRowPitch(0);
		uint64_t destRowPitch = AlignUp(srcRowPitch, (uint64_t)BufferPlacementAlignment::TextureRowPitch);
		for (uint64_t srcProgress = 0; srcProgress < srcImage.GetMipSize(0); srcProgress += srcRowPitch)
		{
			memcpy(destPtr, srcPtr, srcRowPitch);
			srcPtr += srcRowPitch;
			destPtr += destRowPitch;
#ifdef _DEBUG
			if (srcProgress + srcRowPitch > srcImage.GetMipSize(0)) throw std::runtime_error("Something is wrong here.");
#endif
		}

		cmd.CopyTempBufferToTextureSubresource(tempBuffer, *this, destFaceIndex, destMipIndex);
	}

	void Texture::SetPixelsAtSubresource(CommandList& cmd, const void* pixelData, uint32_t destFaceIndex, uint32_t destMipIndex)
	{
		if (!pixelData)
		{
			Log(LogType::Error, "Failed to set texture pixels at subresource. Reason: The provided data is nullptr.");
			return;
		}
		Extent2D subResDimensions = Image::CalculateMipExtent(destMipIndex, width, height);
		Image image;
		image.LoadAsPointer((byte*)pixelData, subResDimensions.width, subResDimensions.height, format, 1, 1, false);
		SetPixelsAtSubresource(cmd, image, destFaceIndex, destMipIndex);
	}

	void Texture::ReadPixels(Image& destImage)
	{
		if (!isLoaded || usage != TextureUsage::Readable)
		{
			Log(LogType::Error, "Failed to read texture pixels. Reason: Texture must be created with Readable usage.");
			return;
		}
		if (msaa != MSAA::Disabled)
		{
			Log(LogType::Error, "Failed to read texture pixels. Reason: Texture is multisampled.");
			return;
		}
		if (!destImage.IsLoaded() ||
			destImage.GetWidth() != width ||
			destImage.GetHeight() != height ||
			destImage.GetFormat() != format ||
			destImage.GetNumFaces() != numFaces ||
			destImage.GetNumMipLevels() != numMipLevels)
		{
			Log(LogType::Error, "Failed to read texture pixels. Reason: Image dimension must match texture.");
			return;
		}

		byte* destPtr = (byte*)destImage.GetPixels();
		byte* srcPtr = (byte*)readMapped.at(context->GetFrameIndex());
		for (uint32_t faceIndex = 0; faceIndex < numFaces; faceIndex++)
		{
			for (uint32_t mipIndex = 0; mipIndex < numMipLevels; mipIndex++)
			{
				size_t srcRowPitch = AlignUp(Image::CalculateMipRowPitch(mipIndex, width, height, format),
					(uint64_t)BufferPlacementAlignment::TextureRowPitch);
				size_t destRowPitch = destImage.GetMipRowPitch(mipIndex);
				for (uint64_t destProgress = 0; destProgress < destImage.GetMipSize(mipIndex); destProgress += destRowPitch)
				{
					memcpy(destPtr, srcPtr, destRowPitch);
					srcPtr += srcRowPitch;
					destPtr += destRowPitch;
#ifdef _DEBUG
					if (destProgress + destRowPitch > destImage.GetMipSize(mipIndex)) throw std::runtime_error("Something is wrong here.");
#endif
				}
			}
		}

	}

	Image Texture::ReadPixels()
	{
		Image image;
		if (!image.Load(width, height, format, numMipLevels, numFaces, isCubemap))
		{
			Log(LogType::Error, "Failed to read texture pixels to image. Reason: Failed to create image.");
			return image;
		}
		ReadPixels(image);
		return image;
	}

	void Sampler::Unload()
	{
		if (!descriptor.IsNull())
		{
			context->GetDescriptorHeap().FreeSampler(descriptor);
		}
		descriptor.SetToNull();

		isLoaded = false;
		context = nullptr;
	}

	const Descriptor* Sampler::GetDescriptor() const
	{
		if (!isLoaded) return nullptr;
		if (descriptor.IsNull()) throw std::runtime_error("This should be impossible.");
		return &descriptor;
	}

	bool Sampler::Load(const IGLOContext& context, const SamplerDesc& desc)
	{
		Unload();

#ifdef IGLO_D3D12
		this->descriptor = context.GetDescriptorHeap().AllocateSampler();
		if (this->descriptor.IsNull())
		{
			Log(LogType::Error, "Failed to create sampler state. Reason: Failed to allocate sampler descriptor.");
			Unload();
			return false;
		}

		D3D12_SAMPLER_DESC samplerDesc = {};
		TextureFilterD3D12 filterD3D = ConvertTextureFilterToD3D12(desc.filter);
		samplerDesc.Filter = filterD3D.filter;
		samplerDesc.MaxAnisotropy = filterD3D.maxAnisotropy;
		samplerDesc.AddressU = (D3D12_TEXTURE_ADDRESS_MODE)desc.wrapU;
		samplerDesc.AddressV = (D3D12_TEXTURE_ADDRESS_MODE)desc.wrapV;
		samplerDesc.AddressW = (D3D12_TEXTURE_ADDRESS_MODE)desc.wrapW;
		samplerDesc.ComparisonFunc = (D3D12_COMPARISON_FUNC)desc.comparisonFunc;
		samplerDesc.MinLOD = desc.minLOD;
		samplerDesc.MaxLOD = desc.maxLOD;
		samplerDesc.MipLODBias = desc.mipMapLODBias;
		samplerDesc.BorderColor[0] = desc.borderColor.red;
		samplerDesc.BorderColor[1] = desc.borderColor.green;
		samplerDesc.BorderColor[2] = desc.borderColor.blue;
		samplerDesc.BorderColor[3] = desc.borderColor.alpha;
		context.GetD3D12Device()->CreateSampler(&samplerDesc, context.GetDescriptorHeap().GetD3D12CPUDescriptorHandleForSampler(this->descriptor));

		this->isLoaded = true;
		this->context = &context;
		return true;
#endif
#ifdef IGLO_VULKAN
		return false; //TODO: this
#endif
	}


} //end of namespace ig
