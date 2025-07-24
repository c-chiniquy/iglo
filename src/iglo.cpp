
#include "iglo.h"
#include "shaders/CS_GenerateMipmaps.h"

#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#ifdef _WIN32
// Quick workaround to stop stb_image_write.h from causing visual studio compile error: "'sprintf': This function or variable may be unsafe."
#define __STDC_LIB_EXT1__
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_WINDOWS_UTF8
#include "stb/stb_image_write.h"

namespace ig
{
	const BlendDesc BlendDesc::BlendDisabled =
	{
		false, // enabled
		BlendData::SourceAlpha, BlendData::InverseSourceAlpha, BlendOperation::Add,
		BlendData::One, BlendData::InverseSourceAlpha, BlendOperation::Add,
		(byte)ColorWriteMask::All
	};
	const BlendDesc BlendDesc::StraightAlpha =
	{
		true, // enabled
		BlendData::SourceAlpha, BlendData::InverseSourceAlpha, BlendOperation::Add,
		// Dest alpha is affected by src alpha, which is useful for when drawing to transparent rendertargets.
		BlendData::One, BlendData::InverseSourceAlpha, BlendOperation::Add,
		(byte)ColorWriteMask::All
	};
	const BlendDesc BlendDesc::PremultipliedAlpha =
	{
		true, // enabled
		BlendData::One, BlendData::InverseSourceAlpha, BlendOperation::Add,
		BlendData::One, BlendData::InverseSourceAlpha, BlendOperation::Add,
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

	std::string GetGpuVendorNameFromID(uint32_t vendorID)
	{
		switch (vendorID)
		{
		case 0x10DE: return "NVIDIA";
		case 0x1002: return "AMD";
		case 0x8086: return "Intel";
		case 0x5143: return "Qualcomm";
		case 0x13B5: return "ARM";
		case 0x106B: return "Apple";
		default:     return "Unknown Vendor";
		}
	}

	FormatInfo GetFormatInfo(Format format)
	{
		uint32_t size = 0; // pixel size in bytes.
		uint32_t block = 0; // For block-compressed formats.
		uint32_t elem = 0; // Amount of elements. (Color channels).
		bool isDepth = false;
		bool hasStencil = false;
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

		case Format::UFLOAT11_UFLOAT11_UFLOAT10:			elem = 3; size = 4;  break;
		case Format::UINT32_UINT32_UINT32_NotNormalized:	elem = 3; size = 12; break;
		case Format::INT32_INT32_INT32_NotNormalized:		elem = 3; size = 12; break;
		case Format::FLOAT_FLOAT_FLOAT:						elem = 3; size = 12; break;

		case Format::BYTE_BYTE_BYTE_BYTE_NotNormalized:			elem = 4; size = 4;  break;
		case Format::INT8_INT8_INT8_INT8:						elem = 4; size = 4;  break;
		case Format::INT8_INT8_INT8_INT8_NotNormalized:			elem = 4; size = 4;  break;
		case Format::UINT10_UINT10_UINT10_UINT2:				elem = 4; size = 4;	 break;
		case Format::UINT10_UINT10_UINT10_UINT2_NotNormalized:	elem = 4; size = 4;	 break;
		case Format::UFLOAT9_UFLOAT9_UFLOAT9_UFLOAT5:			elem = 4; size = 4;	 break;
		case Format::UINT16_UINT16_UINT16_UINT16:				elem = 4; size = 8;  break;
		case Format::UINT16_UINT16_UINT16_UINT16_NotNormalized:	elem = 4; size = 8;  break;
		case Format::INT16_INT16_INT16_INT16:					elem = 4; size = 8;  break;
		case Format::INT16_INT16_INT16_INT16_NotNormalized:		elem = 4; size = 8;  break;
		case Format::FLOAT16_FLOAT16_FLOAT16_FLOAT16:			elem = 4; size = 8;  break;
		case Format::UINT32_UINT32_UINT32_UINT32_NotNormalized:	elem = 4; size = 16; break;
		case Format::INT32_INT32_INT32_INT32_NotNormalized:		elem = 4; size = 16; break;
		case Format::FLOAT_FLOAT_FLOAT_FLOAT:					elem = 4; size = 16; break;

		case Format::BC1:			block = 8;	elem = 4; sRGB_anti = Format::BC1_sRGB;				 break;
		case Format::BC1_sRGB:		block = 8;	elem = 4; sRGB_anti = Format::BC1;		sRGB = true; break;
		case Format::BC2:			block = 16;	elem = 4; sRGB_anti = Format::BC2_sRGB;				 break;
		case Format::BC2_sRGB:		block = 16;	elem = 4; sRGB_anti = Format::BC2;		sRGB = true; break;
		case Format::BC3:			block = 16;	elem = 4; sRGB_anti = Format::BC3_sRGB;				 break;
		case Format::BC3_sRGB:		block = 16;	elem = 4; sRGB_anti = Format::BC3;		sRGB = true; break;
		case Format::BC4_UNSIGNED:	block = 8;	elem = 1; break;
		case Format::BC4_SIGNED:	block = 8;	elem = 1; break;
		case Format::BC5_UNSIGNED:	block = 16; elem = 2; break;
		case Format::BC5_SIGNED:	block = 16; elem = 2; break;
		case Format::BC6H_UFLOAT16: block = 16; elem = 4; break;
		case Format::BC6H_SFLOAT16: block = 16; elem = 4; break;
		case Format::BC7:			block = 16;	elem = 4; sRGB_anti = Format::BC7_sRGB;				 break;
		case Format::BC7_sRGB:		block = 16;	elem = 4; sRGB_anti = Format::BC7;		sRGB = true; break;

		case Format::DEPTHFORMAT_FLOAT:			elem = 1; size = 4; isDepth = true; break;
		case Format::DEPTHFORMAT_UINT16:		elem = 1; size = 2; isDepth = true; break;
		case Format::DEPTHFORMAT_UINT24_BYTE:	elem = 2; size = 4; isDepth = true; hasStencil = true; break;

			// NOTE: D3D12's GetCopyableFootprints() seems to think that this format is 4 bytes per pixel, not 5 or 8.
			// For code simplicity, size is set to 4 to match the size returned from GetCopyableFootprints().
		case Format::DEPTHFORMAT_FLOAT_BYTE:	elem = 2; size = 4; isDepth = true; hasStencil = true; break;

		default:
			break;
		}

		FormatInfo out = FormatInfo();
		out.blockSize = block;
		out.bytesPerPixel = size;
		out.elementCount = elem;
		out.isDepthFormat = isDepth;
		out.hasStencilComponent = hasStencil;
		out.is_sRGB = sRGB;
		out.sRGB_opposite = sRGB_anti;
		return out;
	}

	std::string ConvertKeyToString(Key key)
	{
		switch (key)
		{
		case Key::Unknown: return "Unknown";

		case Key::A: return "A"; case Key::B: return "B"; case Key::C: return "C"; case Key::D: return "D"; case Key::E: return "E";
		case Key::F: return "F"; case Key::G: return "G"; case Key::H: return "H"; case Key::I: return "I"; case Key::J: return "J";
		case Key::K: return "K"; case Key::L: return "L"; case Key::M: return "M"; case Key::N: return "N"; case Key::O: return "O";
		case Key::P: return "P"; case Key::Q: return "Q"; case Key::R: return "R"; case Key::S: return "S"; case Key::T: return "T";
		case Key::U: return "U"; case Key::V: return "V"; case Key::W: return "W"; case Key::X: return "X"; case Key::Y: return "Y";
		case Key::Z: return "Z";

		case Key::Num0: return "Num0"; case Key::Num1: return "Num1"; case Key::Num2: return "Num2"; case Key::Num3: return "Num3";
		case Key::Num4: return "Num4"; case Key::Num5: return "Num5"; case Key::Num6: return "Num6"; case Key::Num7: return "Num7";
		case Key::Num8: return "Num8"; case Key::Num9: return "Num9";

		case Key::Numpad0: return "Numpad0"; case Key::Numpad1: return "Numpad1"; case Key::Numpad2: return "Numpad2";
		case Key::Numpad3: return "Numpad3"; case Key::Numpad4: return "Numpad4"; case Key::Numpad5: return "Numpad5";
		case Key::Numpad6: return "Numpad6"; case Key::Numpad7: return "Numpad7"; case Key::Numpad8: return "Numpad8";
		case Key::Numpad9: return "Numpad9";

		case Key::F1: return "F1"; case Key::F2: return "F2"; case Key::F3: return "F3"; case Key::F4: return "F4";
		case Key::F5: return "F5"; case Key::F6: return "F6"; case Key::F7: return "F7"; case Key::F8: return "F8";
		case Key::F9: return "F9"; case Key::F10: return "F10"; case Key::F11: return "F11"; case Key::F12: return "F12";
		case Key::F13: return "F13"; case Key::F14: return "F14"; case Key::F15: return "F15"; case Key::F16: return "F16";
		case Key::F17: return "F17"; case Key::F18: return "F18"; case Key::F19: return "F19"; case Key::F20: return "F20";
		case Key::F21: return "F21"; case Key::F22: return "F22"; case Key::F23: return "F23"; case Key::F24: return "F24";

		case Key::LeftShift: return "LeftShift"; case Key::RightShift: return "RightShift";
		case Key::LeftControl: return "LeftControl"; case Key::RightControl: return "RightControl";
		case Key::LeftAlt: return "LeftAlt"; case Key::RightAlt: return "RightAlt";
		case Key::LeftSystem: return "LeftSystem"; case Key::RightSystem: return "RightSystem";

		case Key::Backspace: return "Backspace"; case Key::Tab: return "Tab"; case Key::Clear: return "Clear";
		case Key::Enter: return "Enter"; case Key::Pause: return "Pause"; case Key::CapsLock: return "CapsLock";
		case Key::Escape: return "Escape"; case Key::Space: return "Space"; case Key::PageUp: return "PageUp";
		case Key::PageDown: return "PageDown"; case Key::End: return "End"; case Key::Home: return "Home";

		case Key::Left: return "Left"; case Key::Up: return "Up"; case Key::Right: return "Right"; case Key::Down: return "Down";
		case Key::PrintScreen: return "PrintScreen"; case Key::Insert: return "Insert"; case Key::Delete: return "Delete";

		case Key::Sleep: return "Sleep"; case Key::Multiply: return "Multiply"; case Key::Add: return "Add";
		case Key::Seperator: return "Seperator"; case Key::Subtract: return "Subtract";
		case Key::Decimal: return "Decimal"; case Key::Divide: return "Divide";

		case Key::NumLock: return "NumLock"; case Key::ScrollLock: return "ScrollLock";

		case Key::Apps: return "Apps";

		case Key::VolumeMute: return "VolumeMute"; case Key::VolumeDown: return "VolumeDown"; case Key::VolumeUp: return "VolumeUp";
		case Key::MediaNextTrack: return "MediaNextTrack"; case Key::MediaPrevTrack: return "MediaPrevTrack";
		case Key::MediaStop: return "MediaStop"; case Key::MediaPlayPause: return "MediaPlayPause";

		case Key::Plus: return "Plus"; case Key::Comma: return "Comma"; case Key::Minus: return "Minus"; case Key::Period: return "Period";

		case Key::OEM_1: return "OEM_1"; case Key::OEM_2: return "OEM_2"; case Key::OEM_3: return "OEM_3"; case Key::OEM_4: return "OEM_4";
		case Key::OEM_5: return "OEM_5"; case Key::OEM_6: return "OEM_6"; case Key::OEM_7: return "OEM_7"; case Key::OEM_8: return "OEM_8";
		case Key::OEM_102: return "OEM_102";

		default: return "<Unsupported key>";
		}
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

	void IGLOContext::SetWindowSize(uint32_t width, uint32_t height)
	{
		uint32_t cappedWidth = width;
		uint32_t cappedHeight = height;

		Extent2D minSize = windowResizeConstraints.minSize;
		Extent2D maxSize = windowResizeConstraints.maxSize;
		if (cappedWidth < minSize.width && minSize.width != 0) cappedWidth = minSize.width;
		if (cappedHeight < minSize.height && minSize.height != 0) cappedHeight = minSize.height;
		if (cappedWidth > maxSize.width && maxSize.width != 0) cappedWidth = maxSize.width;
		if (cappedHeight > maxSize.height && maxSize.height != 0) cappedHeight = maxSize.height;

		if (windowedMode.size == Extent2D(cappedWidth, cappedHeight)) return;
		windowedMode.size = Extent2D(cappedWidth, cappedHeight);

		if (displayMode == DisplayMode::Windowed)
		{
			Impl_SetWindowSize(width, height);
		}
	}

	void IGLOContext::SetWindowPosition(int32_t x, int32_t y)
	{
		if (windowedMode.pos == IntPoint(x, y)) return;
		windowedMode.pos = IntPoint(x, y);

		if (displayMode == DisplayMode::Windowed)
		{
			Impl_SetWindowPosition(x, y);
		}
	}

	void IGLOContext::SetWindowResizable(bool resizable)
	{
		if (windowedMode.resizable == resizable) return;
		windowedMode.resizable = resizable;

		if (displayMode == DisplayMode::Windowed)
		{
			Impl_SetWindowResizable(resizable);
		}
	}

	void IGLOContext::SetWindowBordersVisible(bool visible)
	{
		if (windowedMode.bordersVisible == visible) return;
		windowedMode.bordersVisible = visible;

		if (displayMode == DisplayMode::Windowed)
		{
			Impl_SetWindowBordersVisible(visible);
		}
	}

	void IGLOContext::SetWindowVisible(bool visible)
	{
		if (windowedMode.visible == visible) return;
		windowedMode.visible = visible;

		if (displayMode == DisplayMode::Windowed)
		{
			Impl_SetWindowVisible(visible);
		}
	}

	void IGLOContext::SetWindowIconFromImage(const Image& icon)
	{
		FormatInfo info = GetFormatInfo(icon.GetFormat());
		uint32_t bitsPerPixel = info.bytesPerPixel * 8;
		uint32_t numChannels = info.elementCount;
		if (bitsPerPixel != 32 || numChannels != 4 || !icon.IsLoaded())
		{
			Log(LogType::Error, "Failed to set window icon. Reason: Icon must have 4 color channels and be 32 bits per pixel.");
			return;
		}

		std::vector<Color32> pixels;
		uint32_t* pixelPtr = nullptr;
		if (icon.GetFormat() == Format::BYTE_BYTE_BYTE_BYTE_BGRA ||
			icon.GetFormat() == Format::BYTE_BYTE_BYTE_BYTE_BGRA_sRGB)
		{
			// We can use the image pixel data directly if the format is already BGRA.
			pixelPtr = (uint32_t*)icon.GetPixels();
		}
		else
		{
			// We must have BGRA format. Swap blue and red channels to convert RGBA to BGRA.
			pixels.resize(icon.GetSize() / 4);
			for (size_t i = 0; i < icon.GetSize() / 4; i++)
			{
				Color32* src = (Color32*)icon.GetPixels();
				pixels[i].red = src[i].blue;
				pixels[i].green = src[i].green;
				pixels[i].blue = src[i].red;
				pixels[i].alpha = src[i].alpha;
			}
			pixelPtr = (uint32_t*)pixels.data();
		}

		Impl_SetWindowIconFromImage_BGRA(icon.GetWidth(), icon.GetHeight(), pixelPtr);
	}

	void IGLOContext::ToggleFullscreen()
	{
		if (displayMode == DisplayMode::Windowed)
		{
			EnterFullscreenMode();
		}
		else
		{
			EnterWindowedMode(windowedMode.size.width, windowedMode.size.height);
		}
	}

	void Pipeline::Unload()
	{
		Impl_Unload();

		isLoaded = false;
		context = nullptr;
		isComputePipeline = false;
		primitiveTopology = PrimitiveTopology::Undefined;
	}

	bool Pipeline::Load(const IGLOContext& context, const PipelineDesc& desc)
	{
		Unload();

		const char* errStr = "Failed to create graphics pipeline state. Reason: ";

		if (desc.blendStates.size() > MAX_SIMULTANEOUS_RENDER_TARGETS)
		{
			Log(LogType::Error, ToString(errStr, "Too many blend states provided."));
			return false;
		}

		if (desc.renderTargetDesc.colorFormats.size() > MAX_SIMULTANEOUS_RENDER_TARGETS)
		{
			Log(LogType::Error, ToString(errStr, "Too many color formats provided in the render target description."));
			return false;
		}

		this->context = &context;
		this->isComputePipeline = false;
		this->primitiveTopology = desc.primitiveTopology;

		DetailedResult result = Impl_Load(context, desc);
		if (!result)
		{
			Log(LogType::Error, errStr + result.errorMessage);
			Unload();
			return false;
		}

		this->isLoaded = true;
		return true;
	}

	bool Pipeline::LoadAsCompute(const IGLOContext& context, const Shader& CS)
	{
		Unload();

		this->context = &context;
		this->isComputePipeline = true;
		this->primitiveTopology = PrimitiveTopology::Undefined;

		DetailedResult result = Impl_LoadAsCompute(context, CS);
		if (!result)
		{
			Log(LogType::Error, "Failed to create compute pipeline state. Reason: " + result.errorMessage);
			Unload();
			return false;
		}

		this->isLoaded = true;
		return true;
	}

	bool Pipeline::LoadFromFile(const IGLOContext& context, const std::string& filepathVS, const std::string& entryPointNameVS,
		const std::string filepathPS, const std::string& entryPointNamePS, const RenderTargetDesc& renderTargetDesc,
		const std::vector<VertexElement>& vertexLayout, PrimitiveTopology primitiveTopology, DepthDesc depth,
		RasterizerDesc rasterizer, const std::vector<BlendDesc>& blend)
	{
		Unload();

		const char* errStr = "Failed to create graphics pipeline state. Reason: ";

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
			renderTargetDesc, vertexLayout, primitiveTopology, depth, rasterizer, blend);
	}

	bool Pipeline::Load(const IGLOContext& context, const Shader& VS, const Shader& PS,
		const RenderTargetDesc& renderTargetDesc, const std::vector<VertexElement>& vertexLayout,
		PrimitiveTopology primitiveTopology, DepthDesc depth, RasterizerDesc rasterizer, const std::vector<BlendDesc>& blend)
	{
		PipelineDesc desc;
		desc.VS = VS;
		desc.PS = PS;
		desc.vertexLayout = vertexLayout;
		desc.primitiveTopology = primitiveTopology;
		desc.depthState = depth;
		desc.rasterizerState = rasterizer;
		desc.blendStates = blend;
		desc.renderTargetDesc = renderTargetDesc;
		return Load(context, desc);
	}

	void DescriptorHeap::PersistentIndexAllocator::Reset(uint32_t maxIndices)
	{
		this->maxIndices = maxIndices;
		this->allocationCount = 0;

		this->freed.clear();
		this->freed.shrink_to_fit();
		this->freed.reserve(maxIndices);

#ifdef _DEBUG
		this->isAllocated.Clear();
		this->isAllocated.Resize(maxIndices, false);
#endif
	}

	void DescriptorHeap::PersistentIndexAllocator::Clear()
	{
		maxIndices = 0;
		allocationCount = 0;

		freed.clear();
		freed.shrink_to_fit();

#ifdef _DEBUG
		isAllocated.Clear();
#endif
	}

	std::optional<uint32_t> DescriptorHeap::PersistentIndexAllocator::Allocate()
	{
		if (allocationCount >= maxIndices) return std::nullopt;

		uint32_t index;
		if (!freed.empty())
		{
			index = freed.back();
			freed.pop_back();
		}
		else
		{
			index = allocationCount;
		}
		allocationCount++;

#ifdef _DEBUG
		if (isAllocated.GetAt(index)) throw std::runtime_error("The newly allocated index was already allocated!");
		isAllocated.SetTrue(index);
#endif

		return index;
	}

	void DescriptorHeap::PersistentIndexAllocator::Free(uint32_t index)
	{
		if (index >= maxIndices) throw std::out_of_range("Index out of range.");

#ifdef _DEBUG
		if (!isAllocated.GetAt(index)) throw std::runtime_error("Double free or invalid free detected.");
		isAllocated.SetFalse(index);
		if (allocationCount == 0) throw std::runtime_error("Something is wrong here.");
#endif

		freed.push_back(index);
		allocationCount--;
	}

	void DescriptorHeap::TempIndexAllocator::Reset(uint32_t maxIndices, uint32_t offset)
	{
		this->maxIndices = maxIndices;
		this->allocationCount = 0;
		this->offset = offset;
	}

	void DescriptorHeap::TempIndexAllocator::Clear()
	{
		maxIndices = 0;
		allocationCount = 0;
	}

	std::optional<uint32_t> DescriptorHeap::TempIndexAllocator::Allocate()
	{
		if (allocationCount >= maxIndices) return std::nullopt;

		uint32_t index = allocationCount;
		allocationCount++;
		return index + offset;
	}

	void DescriptorHeap::TempIndexAllocator::FreeAllIndices()
	{
		allocationCount = 0;
	}

	void DescriptorHeap::Unload()
	{
		if (isLoaded)
		{
			if (persResourceIndices.GetAllocationCount() > 0)
			{
				Log(LogType::Warning, ToString(persResourceIndices.GetAllocationCount(),
					" unfreed persistent resource descriptor(s) detected."));
			}
			if (persSamplerIndices.GetAllocationCount() > 0)
			{
				Log(LogType::Warning, ToString(persSamplerIndices.GetAllocationCount(),
					" unfreed sampler descriptor(s) detected."));
			}
		}

		Impl_Unload();

		isLoaded = false;
		context = nullptr;
		frameIndex = 0;
		numFrames = 0;

		persResourceIndices.Clear();
		persSamplerIndices.Clear();
		tempResourceIndices.clear();
		tempResourceIndices.shrink_to_fit();
	}

	bool DescriptorHeap::LogErrorIfNotLoaded() const
	{
		if (!isLoaded)
		{
			Log(LogType::Error, "You can't use a descriptor heap that isn't loaded!");
			return true;
		}
		return false;
	}

	uint32_t DescriptorHeap::CalcTotalResDescriptors(uint32_t maxPersistent, uint32_t maxTempPerFrame, uint32_t numFramesInFlight)
	{
		return maxPersistent + (maxTempPerFrame * numFramesInFlight);
	}

	DetailedResult DescriptorHeap::Load(const IGLOContext& context, uint32_t maxPersistentResources,
		uint32_t maxTempResourcesPerFrame, uint32_t maxSamplers, uint32_t numFramesInFlight)
	{
		Unload();

		this->context = &context;
		this->numFrames = numFramesInFlight;

		// Resource descriptor indices are partitioned like this:
		// [maxPersistent][maxTemporaryPerFrame frame 0][maxTemporaryPerFrame frame 1][maxTemporaryPerFrame frame N...]
		this->persResourceIndices.Reset(maxPersistentResources);
		this->persSamplerIndices.Reset(maxSamplers);
		this->tempResourceIndices.resize(numFrames);
		for (uint32_t i = 0; i < numFrames; i++)
		{
			uint32_t offset = maxPersistentResources + (i * maxTempResourcesPerFrame);
			this->tempResourceIndices[i].Reset(maxTempResourcesPerFrame, offset);
		}

		DetailedResult result = Impl_Load(context, maxPersistentResources, maxTempResourcesPerFrame, maxSamplers, numFramesInFlight);
		if (!result)
		{
			Unload();
			return result;
		}

		this->isLoaded = true;
		return DetailedResult::MakeSuccess();
	}

	void DescriptorHeap::NextFrame()
	{
		if (LogErrorIfNotLoaded()) return;

		frameIndex = (frameIndex + 1) % numFrames;

		assert(frameIndex < tempResourceIndices.size());
		tempResourceIndices[frameIndex].FreeAllIndices();

		Impl_NextFrame();
	}

	void DescriptorHeap::FreeAllTempResources()
	{
		if (LogErrorIfNotLoaded()) return;

		for (size_t i = 0; i < tempResourceIndices.size(); i++)
		{
			tempResourceIndices[i].FreeAllIndices();
		}

		Impl_FreeAllTempResources();
	}

	DescriptorHeap::Stats DescriptorHeap::GetStats() const
	{
		if (LogErrorIfNotLoaded()) return Stats();

		assert(frameIndex < tempResourceIndices.size());

		Stats out;
		out.maxPersistentResources = persResourceIndices.GetMaxIndices();
		out.maxTempResources = tempResourceIndices[frameIndex].GetMaxIndices();
		out.maxSamplers = persSamplerIndices.GetMaxIndices();
		out.livePersistentResources = persResourceIndices.GetAllocationCount();
		out.liveTempResources = tempResourceIndices[frameIndex].GetAllocationCount();
		out.liveSamplers = persSamplerIndices.GetAllocationCount();
		return out;
	}

	std::string DescriptorHeap::GetStatsString() const
	{
		return GetStats().ToString();
	}

	Descriptor DescriptorHeap::AllocatePersistent(DescriptorType descriptorType)
	{
		if (LogErrorIfNotLoaded()) return Descriptor();

		PersistentIndexAllocator& allocator = (descriptorType == DescriptorType::Sampler) ? persSamplerIndices : persResourceIndices;

		std::optional<uint32_t> out = allocator.Allocate();
		if (!out)
		{
			Log(LogType::Error, ToString("Failed to allocate persistent resource descriptor (",
				allocator.GetAllocationCount(), " out of ",
				allocator.GetMaxIndices(), " descriptors in use)."));
			return Descriptor();
		}

		return Descriptor(out.value(), descriptorType);
	}

	Descriptor DescriptorHeap::AllocateTemp(DescriptorType descriptorType)
	{
		if (LogErrorIfNotLoaded()) return Descriptor();
		if (descriptorType == DescriptorType::Sampler)
		{
			Log(LogType::Error, ToString("Failed to allocate temporary descriptor. Reason: Temporary samplers are not supported."));
			return Descriptor();
		}

		assert(frameIndex < tempResourceIndices.size());
		TempIndexAllocator& allocator = tempResourceIndices[frameIndex];

		std::optional<uint32_t> out = allocator.Allocate();
		if (!out)
		{
			Log(LogType::Error, ToString("Failed to allocate temporary resource descriptor (",
				allocator.GetAllocationCount(), " out of ",
				allocator.GetMaxIndices(), " descriptors in use)."));
			return Descriptor();
		}

		return Descriptor(out.value(), descriptorType);
	}

	void DescriptorHeap::FreePersistent(Descriptor descriptor)
	{
		if (LogErrorIfNotLoaded()) return;

		if (descriptor.IsNull())
		{
			Log(LogType::Error, "Unable to free descriptor. Reason: Descriptor is already null.");
			return;
		}

		if (descriptor.type == DescriptorType::Sampler)
		{
			persSamplerIndices.Free(descriptor.heapIndex);
		}
		else
		{
			persResourceIndices.Free(descriptor.heapIndex);
		}
	}

	void CommandList::Unload()
	{
		Impl_Unload();

		isLoaded = false;
		context = nullptr;
		numFrames = 0;
		frameIndex = 0;
		commandListType = CommandListType::Graphics;
	}

	bool CommandList::Load(const IGLOContext& context, CommandListType commandListType)
	{
		Unload();

		uint32_t numFrames = context.GetMaxFramesInFlight();

		this->context = &context;
		this->numFrames = numFrames;
		this->commandListType = commandListType;
		this->frameIndex = 0;

		DetailedResult result = Impl_Load(context, commandListType);
		if (!result)
		{
			Log(LogType::Error, "Failed to create command list. Reason: " + result.errorMessage);
			Unload();
			return false;
		}

		this->isLoaded = true;
		return true;
	}

	void CommandList::Begin()
	{
		if (!isLoaded)
		{
			Log(LogType::Error, "Failed recording commands. Reason: Command list isn't loaded.");
			return;
		}

		// Advance to next frame
		frameIndex = (frameIndex + 1) % numFrames;

		Impl_Begin();
	}

	void CommandList::End()
	{
		if (!isLoaded)
		{
			Log(LogType::Error, "Failed to stop recording commands. Reason: Command list isn't loaded.");
			return;
		}

		Impl_End();
	}

	SimpleBarrierInfo GetSimpleBarrierInfo(SimpleBarrier simpleBarrier, CommandListType queueType)
	{
		SimpleBarrierInfo out;
		out.discard = (simpleBarrier == SimpleBarrier::Discard);

		bool isGraphicsQueue = (queueType == CommandListType::Graphics);
		bool isComputeQueue = (queueType == CommandListType::Compute);
#ifdef IGLO_D3D12
		bool isCopyQueue = (queueType == CommandListType::Copy);
#endif

		switch (simpleBarrier)
		{
		default:
			throw std::invalid_argument("Invalid SimpleBarrier.");

			// In D3D12, 'Present' and 'Common' layouts share the same enum value (0).
			// We distinguish them here as separate to provide the option of not using queue-specific layouts.
			// Queue-specific layouts (e.g., _GraphicsQueue_Common) are incompatible with backbuffers.
		case SimpleBarrier::Common:
			out.sync = BarrierSync::All;
			out.access = BarrierAccess::Common;
			out.layout = BarrierLayout::Common;
			if (isGraphicsQueue) out.layout = BarrierLayout::_GraphicsQueue_Common;
			if (isComputeQueue) out.layout = BarrierLayout::_ComputeQueue_Common;
			break;

		case SimpleBarrier::Present:
			out.sync = BarrierSync::All;
			out.access = BarrierAccess::Common;
			out.layout = BarrierLayout::Present;
			break;

		case SimpleBarrier::Discard:
			out.sync = BarrierSync::None;
			out.access = BarrierAccess::NoAccess;
			out.layout = BarrierLayout::Undefined;
			break;

		case SimpleBarrier::PixelShaderResource:
			out.sync = BarrierSync::PixelShading;
			out.access = BarrierAccess::ShaderResource;
			out.layout = BarrierLayout::ShaderResource;
			if (queueType == CommandListType::Compute)
			{
				out.sync = BarrierSync::None;
				out.access = BarrierAccess::NoAccess;
			}
			if (isGraphicsQueue) out.layout = BarrierLayout::_GraphicsQueue_ShaderResource;
			if (isComputeQueue) out.layout = BarrierLayout::_ComputeQueue_ShaderResource;
			break;

		case SimpleBarrier::ComputeShaderResource:
			out.sync = BarrierSync::ComputeShading;
			out.access = BarrierAccess::ShaderResource;
			out.layout = BarrierLayout::ShaderResource;
			if (isGraphicsQueue) out.layout = BarrierLayout::_GraphicsQueue_ShaderResource;
			if (isComputeQueue) out.layout = BarrierLayout::_ComputeQueue_ShaderResource;
			break;

		case SimpleBarrier::ComputeShaderUnorderedAccess:
			out.sync = BarrierSync::ComputeShading;
			out.access = BarrierAccess::UnorderedAccess;
			out.layout = BarrierLayout::UnorderedAccess;
			if (isGraphicsQueue) out.layout = BarrierLayout::_GraphicsQueue_UnorderedAccess;
			if (isComputeQueue) out.layout = BarrierLayout::_ComputeQueue_UnorderedAccess;
			break;

		case SimpleBarrier::RenderTarget:
			out.sync = BarrierSync::RenderTarget;
			out.access = BarrierAccess::RenderTarget;
			out.layout = BarrierLayout::RenderTarget;
			break;

		case SimpleBarrier::DepthWrite:
			out.sync = BarrierSync::DepthStencil;
			out.access = BarrierAccess::DepthStencilWrite;
			out.layout = BarrierLayout::DepthStencilWrite;
			break;

		case SimpleBarrier::DepthRead:
			out.sync = BarrierSync::DepthStencil;
			out.access = BarrierAccess::DepthStencilRead;
			out.layout = BarrierLayout::DepthStencilRead;
			break;

		case SimpleBarrier::CopySource:
			out.sync = BarrierSync::Copy;
			out.access = BarrierAccess::CopySource;
			out.layout = BarrierLayout::CopySource;
			if (isGraphicsQueue) out.layout = BarrierLayout::_GraphicsQueue_CopySource;
			if (isComputeQueue) out.layout = BarrierLayout::_ComputeQueue_CopySource;
			break;

		case SimpleBarrier::CopyDest:
			out.sync = BarrierSync::Copy;
			out.access = BarrierAccess::CopyDest;
			out.layout = BarrierLayout::CopyDest;
			if (isGraphicsQueue) out.layout = BarrierLayout::_GraphicsQueue_CopyDest;
			if (isComputeQueue) out.layout = BarrierLayout::_ComputeQueue_CopyDest;
#ifdef IGLO_D3D12
			if (isCopyQueue) out.layout = BarrierLayout::Common;
#endif
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

		case SimpleBarrier::ClearUnorderedAccess:
			out.sync = BarrierSync::ClearUnorderedAccessView;
			out.access = BarrierAccess::UnorderedAccess;
			out.layout = BarrierLayout::UnorderedAccess;
			if (isGraphicsQueue) out.layout = BarrierLayout::_GraphicsQueue_UnorderedAccess;
			if (isComputeQueue) out.layout = BarrierLayout::_ComputeQueue_UnorderedAccess;
			break;
		}

		return out;
	}

	void CommandList::AddTextureBarrier(const Texture& texture, SimpleBarrier before, SimpleBarrier after)
	{
		SimpleBarrierInfo infoBefore = GetSimpleBarrierInfo(before, commandListType);
		SimpleBarrierInfo infoAfter = GetSimpleBarrierInfo(after, commandListType);
		AddTextureBarrier(texture, infoBefore.sync, infoAfter.sync, infoBefore.access, infoAfter.access, infoBefore.layout, infoAfter.layout);
	}

	void CommandList::AddTextureBarrierAtSubresource(const Texture& texture, SimpleBarrier before, SimpleBarrier after,
		uint32_t faceIndex, uint32_t mipIndex)
	{
		SimpleBarrierInfo infoBefore = GetSimpleBarrierInfo(before, commandListType);
		SimpleBarrierInfo infoAfter = GetSimpleBarrierInfo(after, commandListType);
		AddTextureBarrierAtSubresource(texture, infoBefore.sync, infoAfter.sync, infoBefore.access, infoAfter.access,
			infoBefore.layout, infoAfter.layout, faceIndex, mipIndex);
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
		if (numRenderTextures > 0 && !renderTextures)
		{
			Log(LogType::Error, "Failed to set render targets. Reason: Bad arguments.");
			return;
		}
		if (numRenderTextures > MAX_SIMULTANEOUS_RENDER_TARGETS)
		{
			Log(LogType::Error, "Failed to set render targets. Reason: Too many render textures provided.");
			return;
		}

		Impl_SetRenderTargets(renderTextures, numRenderTextures, depthBuffer, optimizedClear);
	}

	void CommandList::ClearColor(const Texture& renderTexture, Color color, uint32_t numRects, const IntRect* rects)
	{
		if (numRects > 0 && !rects)
		{
			Log(LogType::Error, "Failed to clear render texture. Reason: Bad arguments.");
			return;
		}

		Impl_ClearColor(renderTexture, color, numRects, rects);
	}

	void CommandList::ClearDepth(const Texture& depthBuffer, float depth, byte stencil, bool clearDepth, bool clearStencil,
		uint32_t numRects, const IntRect* rects)
	{
		if (numRects > 0 && !rects)
		{
			Log(LogType::Error, "Failed to clear depth buffer. Reason: Bad arguments.");
			return;
		}

		Impl_ClearDepth(depthBuffer, depth, stencil, clearDepth, clearStencil, numRects, rects);
	}

	void CommandList::ClearUnorderedAccessBufferUInt32(const Buffer& buffer, const uint32_t value)
	{
		if (!buffer.IsLoaded() || !buffer.GetUnorderedAccessDescriptor())
		{
			Log(LogType::Error, "Failed to clear unordered access buffer. Reason: Buffer has no unordered access descriptor.");
			return;
		}

		Impl_ClearUnorderedAccessBufferUInt32(buffer, value);
	}

	void CommandList::ClearUnorderedAccessTextureFloat(const Texture& texture, const float values[4])
	{
		if (!texture.IsLoaded() || !texture.GetUnorderedAccessDescriptor())
		{
			Log(LogType::Error, "Failed to clear unordered access texture. Reason: Texture has no unordered access descriptor.");
			return;
		}

		Impl_ClearUnorderedAccessTextureFloat(texture, values);
	}

	void CommandList::ClearUnorderedAccessTextureUInt32(const Texture& texture, const uint32_t values[4])
	{
		if (!texture.IsLoaded() || !texture.GetUnorderedAccessDescriptor())
		{
			Log(LogType::Error, "Failed to clear unordered access texture. Reason: Texture has no unordered access descriptor.");
			return;
		}

		Impl_ClearUnorderedAccessTextureUInt32(texture, values);
	}

	DetailedResult CommandList::ValidatePushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes)
	{
		if (sizeInBytes > MAX_PUSH_CONSTANTS_BYTE_SIZE)
		{
			return DetailedResult::MakeFail(ToString("Exceeded maximum push constants byte size (", MAX_PUSH_CONSTANTS_BYTE_SIZE, ")."));
		}
		if (sizeInBytes % 4 != 0 || destOffsetInBytes % 4 != 0)
		{
			return DetailedResult::MakeFail("Push constants byte size and offset must be multiples of 4.");
		}
		if (!data || sizeInBytes == 0)
		{
			return DetailedResult::MakeFail("No data provided.");
		}
		return DetailedResult::MakeSuccess();
	}

	void CommandList::SetPushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes)
	{
		DetailedResult result = ValidatePushConstants(data, sizeInBytes, destOffsetInBytes);
		if (!result)
		{
			Log(LogType::Error, ToString("Failed to set push constants of size ", sizeInBytes, ". Reason: ", result.errorMessage));
			return;
		}

		Impl_SetPushConstants(data, sizeInBytes, destOffsetInBytes);
	}

	void CommandList::SetComputePushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes)
	{
		DetailedResult result = ValidatePushConstants(data, sizeInBytes, destOffsetInBytes);
		if (!result)
		{
			Log(LogType::Error, ToString("Failed to set compute push constants of size ", sizeInBytes, ". Reason: ", result.errorMessage));
			return;
		}

		Impl_SetComputePushConstants(data, sizeInBytes, destOffsetInBytes);
	}

	void CommandList::SetVertexBuffer(const Buffer& vertexBuffer, uint32_t slot)
	{
		const Buffer* buffers[] = { &vertexBuffer };
		SetVertexBuffers(buffers, 1, slot);
	}

	void CommandList::SetVertexBuffers(const Buffer* const* vertexBuffers, uint32_t count, uint32_t startSlot)
	{
		if (count == 0 || !vertexBuffers)
		{
			Log(LogType::Error, "Failed setting vertex buffer(s). Reason: No vertex buffers provided.");
			return;
		}
		if (count + startSlot >= MAX_VERTEX_BUFFER_BIND_SLOTS)
		{
			Log(LogType::Error, "Failed setting vertex buffer(s). Reason: Vertex buffer slot out of bounds!");
			return;
		}

		Impl_SetVertexBuffers(vertexBuffers, count, startSlot);
	}

	void CommandList::SetTempVertexBuffer(const void* data, uint64_t sizeInBytes, uint32_t vertexStride, uint32_t slot)
	{
		if (slot >= MAX_VERTEX_BUFFER_BIND_SLOTS)
		{
			Log(LogType::Error, "Failed setting temporary vertex buffer. Reason: Vertex buffer slot out of bounds!");
			return;
		}

		TempBuffer vb = context->GetTempBufferAllocator().AllocateTempBuffer(sizeInBytes,
			context->GetGraphicsSpecs().bufferPlacementAlignments.vertexOrIndexBuffer);
		if (vb.IsNull())
		{
			Log(LogType::Error, "Failed setting temporary vertex buffer. Reason: Failed to allocate temporary buffer.");
			return;
		}

		memcpy(vb.data, data, sizeInBytes);

#ifdef IGLO_D3D12
		D3D12_VERTEX_BUFFER_VIEW view = {};
		view.BufferLocation = vb.impl.resource->GetGPUVirtualAddress() + vb.offset;
		view.SizeInBytes = (UINT)sizeInBytes;
		view.StrideInBytes = (UINT)vertexStride;
		impl.graphicsCommandList->IASetVertexBuffers(slot, 1, &view);
#endif
#ifdef IGLO_VULKAN
		VkBuffer buffer = vb.impl.buffer;
		VkDeviceSize offset = vb.offset;
		vkCmdBindVertexBuffers(impl.currentCommandBuffer, slot, 1, &buffer, &offset);
#endif
	}

	void CommandList::SetViewport(float width, float height)
	{
		Viewport viewport(0, 0, width, height);
		SetViewport(viewport);
	}
	void CommandList::SetViewport(Viewport viewPort)
	{
		SetViewports(&viewPort, 1);
	}

	void CommandList::SetScissorRectangle(int32_t width, int32_t height)
	{
		SetScissorRectangle(IntRect(0, 0, width, height));
	}
	void CommandList::SetScissorRectangle(IntRect scissorRect)
	{
		SetScissorRectangles(&scissorRect, 1);
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

		Impl_CopyTexture(source, destination);
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

		Impl_CopyTextureSubresource(source, sourceFaceIndex, sourceMipIndex, destination, destFaceIndex, destMipIndex);
	}

	void CommandList::CopyTextureToReadableTexture(const Texture& source, const Texture& destination)
	{
		if (!source.IsLoaded() ||
			!destination.IsLoaded() ||
			source.GetUsage() == TextureUsage::Readable ||
			destination.GetUsage() != TextureUsage::Readable)
		{
			Log(LogType::Error, "Unable to copy texture to readable texture."
				" Reason: Destination texture usage must be Readable, and source texture usage must be non-readable.");
			return;
		}

		Impl_CopyTextureToReadableTexture(source, destination);
	}

	void TempBufferAllocator::Page::Free(const IGLOContext& context)
	{
		Impl_Free(context);

		mapped = nullptr;
	}

	void TempBufferAllocator::Unload()
	{
		if (perFrame.size() > 0 && !context) throw std::runtime_error("This should be impossible.");

		for (size_t i = 0; i < perFrame.size(); i++)
		{
			// Free large pages
			for (Page& page : perFrame[i].largePages)
			{
				if (!page.IsNull()) page.Free(*context);
			}

			// Free linear pages
			for (Page& page : perFrame[i].linearPages)
			{
				if (!page.IsNull()) page.Free(*context);
			}
		}

		perFrame.clear();
		perFrame.shrink_to_fit();

		isLoaded = false;
		context = nullptr;
		frameIndex = 0;
		numFrames = 0;
		linearPageSize = 0;
	}

	DetailedResult TempBufferAllocator::Load(const IGLOContext& context, uint64_t linearPageSize, uint32_t numFramesInFlight)
	{
		Unload();

		this->perFrame.clear();
		this->perFrame.shrink_to_fit();
		this->context = &context;

		// Each frame has a set number of persistent pages, and a changing number of temporary pages placed ontop.
		// The temporary pages are created as needed when the persistent pages run out of space.
		// The temporary pages are deleted on a per-frame basis.
		// The persistent pages are reused frame by frame.

		for (size_t i = 0; i < numFramesInFlight; i++)
		{
			PerFrame frame;
			frame.linearNextByte = 0;

			for (size_t p = 0; p < numPersistentPages; p++)
			{
				Page page = Page::Create(context, linearPageSize);
				if (page.IsNull())
				{
					Unload();
					return DetailedResult::MakeFail("The buffer allocator failed to allocate memory.");
				}
				frame.linearPages.push_back(page);
			}

			this->perFrame.push_back(frame);
		}

		this->isLoaded = true;
		this->frameIndex = 0;
		this->numFrames = numFramesInFlight;
		this->linearPageSize = linearPageSize;
		return DetailedResult::MakeSuccess();
	}

	bool TempBufferAllocator::LogErrorIfNotLoaded()
	{
		if (!isLoaded)
		{
			Log(LogType::Error, "You can't use a buffer allocator that isn't loaded!");
			return true;
		}
		return false;
	}

	void TempBufferAllocator::NextFrame()
	{
		if (LogErrorIfNotLoaded()) return;

		frameIndex = (frameIndex + 1) % numFrames;

		assert(frameIndex < perFrame.size());
		FreeTempPagesAtFrame(*context, perFrame[frameIndex]);
	}

	void TempBufferAllocator::FreeAllTempPages()
	{
		if (LogErrorIfNotLoaded()) return;

		for (size_t i = 0; i < perFrame.size(); i++)
		{
			FreeTempPagesAtFrame(*context, perFrame[i]);
		}
	}

	void TempBufferAllocator::FreeTempPagesAtFrame(const IGLOContext& context, PerFrame& perFrame)
	{
		// Delete extra linear pages
		while (perFrame.linearPages.size() > numPersistentPages)
		{
			perFrame.linearPages.back().Free(context);
			perFrame.linearPages.pop_back();
		}

		// Delete large pages
		for (size_t i = 0; i < perFrame.largePages.size(); i++)
		{
			perFrame.largePages[i].Free(context);
		}
		perFrame.largePages.clear();


		perFrame.linearNextByte = 0;
	}

	TempBuffer TempBufferAllocator::AllocateTempBuffer(uint64_t sizeInBytes, uint32_t alignment)
	{
		if (LogErrorIfNotLoaded()) return TempBuffer();

		assert(frameIndex < perFrame.size());
		PerFrame& current = perFrame[frameIndex];

		uint64_t alignedStart = AlignUp(current.linearNextByte, (uint64_t)alignment);
		uint64_t alignedSize = AlignUp(sizeInBytes, (uint64_t)alignment);

		// New large page
		if (alignedSize > linearPageSize)
		{
			Page large = Page::Create(*context, alignedSize);
			if (large.IsNull())
			{
				Log(LogType::Error, ToString("Failed to allocate a temporary buffer of size ", sizeInBytes, "."));
				return TempBuffer();
			}
			current.largePages.push_back(large);

			TempBuffer out;
			out.offset = 0;
			out.data = large.mapped;
			out.impl.Set(large.impl);
			return out;
		}

		// New linear page
		if (alignedStart + alignedSize > linearPageSize)
		{
			Page page = Page::Create(*context, linearPageSize);
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
			out.impl.Set(page.impl);
			return out;
		}

		// Use existing linear page
		{
			if (current.linearPages.size() == 0) throw std::runtime_error("This should be impossible.");

			// Move position forward by actual size, not aligned size.
			// Aligning the size is only useful for preventing temporary buffer from extending outside bounds.
			// If code reaches this point, we already know it won't extend too far.
			current.linearNextByte = alignedStart + sizeInBytes;

			Page& page = current.linearPages.back();

			TempBuffer out;
			out.offset = alignedStart;
			out.data = (void*)((size_t)page.mapped + (size_t)alignedStart);
			out.impl.Set(page.impl);
			return out;
		}
	}

	size_t TempBufferAllocator::GetNumLivePages() const
	{
		if (!isLoaded) return 0;
		assert(frameIndex < perFrame.size());
		const PerFrame& current = perFrame[frameIndex];
		return current.largePages.size() + current.linearPages.size();
	}

	void IGLOContext::SetFrameBuffering(uint32_t numFramesInFlight, uint32_t numBackBuffers)
	{
		if (this->numFramesInFlight == numFramesInFlight &&
			this->swapChain.numBackBuffers == numBackBuffers) return;

		if (numFramesInFlight > numBackBuffers)
		{
			Log(LogType::Error, ToString(
				"Invalid framebuffer configuration: frames in flight (", numFramesInFlight,
				") cannot exceed number of backbuffers (", numBackBuffers, ")."));
			return;
		}
		if (numFramesInFlight > this->maxFramesInFlight)
		{
			Log(LogType::Error, ToString(
				"Invalid framebuffer configuration: frames in flight (", numFramesInFlight,
				") exceeds max allowed (", this->maxFramesInFlight, ") set at context creation."));
			return;
		}

		WaitForIdleDevice();
		DetailedResult result = CreateSwapChain(this->swapChain.extent, this->swapChain.format,
			numBackBuffers, numFramesInFlight, this->swapChain.presentMode);
		if (!result)
		{
			Log(LogType::Error, ToString(
				"Failed to recreate swapchain with ", numBackBuffers, " backbuffers and ",
				numFramesInFlight, " frames in flight. Reason: ", result.errorMessage));
		}
		this->frameIndex = 0;
		this->numFramesInFlight = numFramesInFlight;
		this->endOfFrame.clear();
		this->endOfFrame.resize(numFramesInFlight);

		// Recreate the temp buffer allocator with new number of frames in flight
		this->tempBufferAllocator.Load(*this, this->tempBufferAllocator.GetLinearPageSize(), numFramesInFlight);

		// The descriptor heap manager can't be recreated here with a new number of frames in flight,
		// because it contains persistent descriptors which we don't want to destroy.
		// A few unused per-frame temporary descriptors (and some per-frame temporary VkImageView) is not a big deal though.

	}

	void IGLOContext::SetBackBufferFormat(Format format)
	{
		if (swapChain.format == format) return;

		WaitForIdleDevice();
		DetailedResult result = CreateSwapChain(swapChain.extent, format, swapChain.numBackBuffers,
			numFramesInFlight, swapChain.presentMode);
		if (!result)
		{
			Log(LogType::Error, ToString("Failed to recreate swapchain with iglo format: ",
				(int)format, " Reason: ", result.errorMessage));
		}
	}

	MSAA IGLOContext::GetMaxMultiSampleCount(Format textureFormat) const
	{
		if (!isLoaded)
		{
			Log(LogType::Error, "Failed to get max multisample count. Reason: IGLOContext must be loaded first.");
			return MSAA::Disabled;
		}

		constexpr uint32_t MaxIgloFormats = 256; // An arbitrary safe upper bound.

		uint32_t formatIndex = (uint32_t)textureFormat;
		if (formatIndex >= MaxIgloFormats) throw std::invalid_argument("Invalid format.");

		if (maxMSAAPerFormat.size() == 0) maxMSAAPerFormat.resize(MaxIgloFormats, 0);

		// If the max MSAA has already been retreived for this format, then return it.
		if (maxMSAAPerFormat[formatIndex] != 0) return (MSAA)maxMSAAPerFormat[formatIndex];

		uint32_t maxMSAA = Impl_GetMaxMultiSampleCount(textureFormat);
		maxMSAAPerFormat[formatIndex] = maxMSAA;
		return (MSAA)maxMSAA;
	}

	const RenderTargetDesc& IGLOContext::GetBackBufferRenderTargetDesc(bool get_opposite_sRGB_view) const
	{
		if (get_opposite_sRGB_view)
		{
			if (swapChain.wrapped_sRGB_opposite.size() == 0)
			{
				Log(LogType::Warning, "Failed to get back buffer render target desc with an sRGB opposite format as requested."
					" Reason: No back buffer views exist with an sRGB opposite format.");
				return swapChain.renderTargetDesc;
			}
			else
			{
				return swapChain.renderTargetDesc_sRGB_opposite;
			}
		}
		else
		{
			return swapChain.renderTargetDesc;
		}
	}

	const Texture& IGLOContext::GetBackBuffer(bool get_opposite_sRGB_view) const
	{
		uint32_t backBufferIndex = GetCurrentBackBufferIndex();
		if (get_opposite_sRGB_view)
		{
			if (swapChain.wrapped_sRGB_opposite.size() == 0)
			{
				Log(LogType::Warning, "Failed to get back buffer view with an sRGB opposite format as requested."
					" Reason: No back buffer views exist with an sRGB opposite format.");
				return *swapChain.wrapped[backBufferIndex];
			}
			else
			{
				return *swapChain.wrapped_sRGB_opposite[backBufferIndex];
			}
		}
		else
		{
			return *swapChain.wrapped[backBufferIndex];
		}
	}

	void IGLOContext::OnFatalError(const std::string& message, bool popupMessage)
	{
		Log(LogType::FatalError, message);
		if (popupMessage) PopupMessage(message, window.title, this);
	}

	bool IGLOContext::Load(WindowSettings windowSettings, RenderSettings renderSettings, bool showPopupIfFailed)
	{
		if (isLoaded)
		{
			Log(LogType::Warning, "You are attempting to load an already loaded IGLOContext."
				" The existing context will be replaced with a new one.");
		}

		// Unload previous context if there is one
		Unload();

		if (renderSettings.maxFramesInFlight > renderSettings.numBackBuffers)
		{
			Log(LogType::Error, "Failed to load IGLOContext. Reason: You can't have more frames in flight than back buffers!");
			return false;
		}

		DetailedResult windowResult = LoadWindow(windowSettings);
		if (!windowResult)
		{
			OnFatalError("Failed to initialize window. Reason: " + windowResult.errorMessage, showPopupIfFailed);
			Unload();
			return false;
		}

		DetailedResult graphicsResult = LoadGraphicsDevice(renderSettings, windowedMode.size);
		if (!graphicsResult)
		{
			OnFatalError("Failed to initialize " IGLO_GRAPHICS_API_STRING ". Reason: " + graphicsResult.errorMessage, showPopupIfFailed);
			Unload();
			return false;
		}

		PrepareWindowPostGraphics(windowSettings);

		// Window and graphics device has loaded successfully
		isLoaded = true;
		return true;
	}

	void IGLOContext::Unload()
	{
		if (commandQueue.IsLoaded()) commandQueue.WaitForIdle();
#ifdef IGLO_VULKAN
		if (isLoaded) vkDeviceWaitIdle(graphics.device);
#endif

		isLoaded = false;

#ifdef __linux__
		// You are required to unload the X11 display before unloading the vulkan device.
		// Otherwise you may get a segfault.
		UnloadWindow();
		UnloadGraphicsDevice();
#else
		UnloadGraphicsDevice();
		UnloadWindow();
#endif
	}

	void IGLOContext::UnloadWindow()
	{
		eventQueue = {};

		windowedMode = WindowState();
		windowResizeConstraints = WindowResizeConstraints();

		callbackModalLoop = nullptr;
		insideModalLoopCallback = false;

		mousePosition = IntPoint();
		mouseButtonIsDown.Clear();
		keyIsDown.Clear();

		Impl_UnloadWindow();
	}

	void IGLOContext::UnloadGraphicsDevice()
	{
		DestroySwapChainResources();

		endOfFrame.clear();
		endOfFrame.shrink_to_fit();

		graphicsSpecs = GraphicsSpecs();

		displayMode = DisplayMode::Windowed;

		maxFramesInFlight = 0;
		numFramesInFlight = 0;
		frameIndex = 0;

		generateMipmapsPipeline.Unload();
		bilinearClampSampler.Unload();

		descriptorHeap.Unload();
		tempBufferAllocator.Unload();
		commandQueue.Unload();

		callbackOnDeviceRemoved = nullptr;

		Impl_UnloadGraphicsDevice();

		maxMSAAPerFormat.clear();
		maxMSAAPerFormat.shrink_to_fit();
	}

	DetailedResult IGLOContext::LoadWindow(const WindowSettings& windowSettings)
	{
		mousePosition = IntPoint(0, 0);
		mouseButtonIsDown.Clear();
		mouseButtonIsDown.Resize(6, false); // 6 mouse buttons
		keyIsDown.Clear();
		keyIsDown.Resize(256, false); // There are max 256 keys

		windowedMode.pos = IntPoint(0, 0);
		windowedMode.size = Extent2D(windowSettings.width, windowSettings.height);
		windowedMode.visible = windowSettings.visible;
		windowedMode.bordersVisible = windowSettings.bordersVisible;
		windowedMode.resizable = windowSettings.resizable;

		window.title = windowSettings.title;

		return Impl_LoadWindow(windowSettings);
	}

	DetailedResult IGLOContext::LoadGraphicsDevice(const RenderSettings& renderSettings, Extent2D backBufferSize)
	{
		maxFramesInFlight = renderSettings.maxFramesInFlight;
		numFramesInFlight = renderSettings.maxFramesInFlight;
		frameIndex = 0;

		endOfFrame.clear();
		endOfFrame.shrink_to_fit();
		endOfFrame.resize(renderSettings.maxFramesInFlight);

		// Load graphics device
		{
			DetailedResult result = Impl_LoadGraphicsDevice();
			if (!result) return result;
		}

		// Load command queue manager
		{
			DetailedResult result = commandQueue.Load(*this,
				renderSettings.maxFramesInFlight,
				renderSettings.numBackBuffers);
			if (!result) return result;
		}

		// Load buffer allocation manager
		{
			DetailedResult result = tempBufferAllocator.Load(*this,
				renderSettings.tempBufferAllocatorLinearPageSize,
				renderSettings.maxFramesInFlight);
			if (!result) return result;
		}

		// Load descriptor heap manager
		{
			DetailedResult result = descriptorHeap.Load(*this,
				renderSettings.maxPersistentResourceDescriptors,
				renderSettings.maxTemporaryResourceDescriptorsPerFrame,
				renderSettings.maxSamplerDescriptors,
				renderSettings.maxFramesInFlight);
			if (!result) return result;
		}

		// Create Swap Chain
		{
			DetailedResult result = CreateSwapChain(backBufferSize,
				renderSettings.backBufferFormat,
				renderSettings.numBackBuffers,
				renderSettings.maxFramesInFlight,
				renderSettings.presentMode);
			if (!result) return result;
		}

		// Load mipmap generation compute pipeline
		{
			if (!generateMipmapsPipeline.LoadAsCompute(*this, Shader(g_CS_GenerateMipmaps, sizeof(g_CS_GenerateMipmaps), "CSMain")))
			{
				Log(LogType::Warning, "Failed to load mipmap generation compute pipeline.");
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

		return DetailedResult::MakeSuccess();
	}

	void CommandQueue::Unload()
	{
		Impl_Unload();

		isLoaded = false;
		context = nullptr;
	}

	DetailedResult CommandQueue::Load(const IGLOContext& context, uint32_t numFramesInFlight, uint32_t numBackBuffers)
	{
		Unload();

		this->context = &context;

		DetailedResult result = Impl_Load(context, numFramesInFlight, numBackBuffers);
		if (!result)
		{
			Unload();
			return result;
		}

		this->isLoaded = true;
		return DetailedResult::MakeSuccess();
	}

	Receipt CommandQueue::SubmitCommands(const CommandList& commandList)
	{
		const CommandList* cmd[] = { &commandList };
		return SubmitCommands(cmd, 1);
	}

	Receipt CommandQueue::SubmitCommands(const CommandList* const* commandLists, uint32_t numCommandLists)
	{
		const char* errStr = "Failed to submit commands. Reason: ";

		if (!commandLists || numCommandLists == 0)
		{
			Log(LogType::Error, ToString(errStr, "No command lists specified."));
			return Receipt();
		}
		if (numCommandLists > MAX_COMMAND_LISTS_PER_SUBMIT)
		{
			Log(LogType::Error, ToString(errStr, "Exceeded max number of command lists per submit."));
			return Receipt();
		}

		CommandListType cmdType = commandLists[0]->GetCommandListType();

		for (uint32_t i = 0; i < numCommandLists; i++)
		{
			if (!commandLists[i]->IsLoaded())
			{
				Log(LogType::Error, ToString(errStr, "Atleast one specified command list isn't loaded."));
				return Receipt();
			}
			if (commandLists[i]->GetCommandListType() != cmdType)
			{
				Log(LogType::Error, ToString(errStr, "The provided command lists must share the same command list type."));
				return Receipt();
			}
		}

		return Impl_SubmitCommands(commandLists, numCommandLists, cmdType);
	}

	bool CommandQueue::IsComplete(Receipt receipt) const
	{
		if (receipt.IsNull()) return true;

		return Impl_IsComplete(receipt);
	}

	void CommandQueue::WaitForCompletion(Receipt receipt)
	{
		if (receipt.IsNull()) return;
		if (IsComplete(receipt)) return;

		Impl_WaitForCompletion(receipt);
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

		desc = ImageDesc();
	}

	Image::Image(Image&& other) noexcept
	{
		*this = std::move(other);
	}
	Image& Image::operator=(Image&& other) noexcept
	{
		Unload();

		std::swap(this->desc, other.desc);
		std::swap(this->size, other.size);
		std::swap(this->pixelsPtr, other.pixelsPtr);
		std::swap(this->mustFreeSTBI, other.mustFreeSTBI);

		this->ownedBuffer.swap(other.ownedBuffer);

		return *this;
	}

	DetailedResult ImageDesc::Validate() const
	{
		if (extent.width == 0 || extent.height == 0)
		{
			return DetailedResult::MakeFail("Width and height must be nonzero.");
		}
		if (format == Format::None)
		{
			return DetailedResult::MakeFail("Bad format.");
		}
		if (mipLevels < 1 || numFaces < 1)
		{
			return DetailedResult::MakeFail("mipLevels and numFaces must be 1 or higher.");
		}
		if (isCubemap && (numFaces % 6 != 0))
		{
			return DetailedResult::MakeFail("The number of faces in a cubemap image must be a multiple of 6.");
		}
		FormatInfo info = GetFormatInfo(format);
		if (info.blockSize > 0)
		{
			bool widthIsMultipleOf4 = (extent.width % 4 == 0);
			bool heightIsMultipleOf4 = (extent.height % 4 == 0);
			if (!widthIsMultipleOf4 || !heightIsMultipleOf4)
			{
				return DetailedResult::MakeFail("Width and height must be multiples of 4 when using a block compression format.");
			}
		}
		return DetailedResult::MakeSuccess();
	}

	bool Image::Load(uint32_t width, uint32_t height, Format format)
	{
		ImageDesc imageDesc;
		imageDesc.extent = Extent2D(width, height);
		imageDesc.format = format;
		return Load(imageDesc);
	}

	bool Image::Load(const ImageDesc& desc)
	{
		Unload();

		DetailedResult result = desc.Validate();
		if (!result)
		{
			Log(LogType::Error, "Failed to create image. Reason: " + result.errorMessage);
			return false;
		}

		this->desc = desc;
		this->mustFreeSTBI = false;
		this->size = CalculateTotalSize(desc.extent, desc.format, desc.mipLevels, desc.numFaces);
		this->ownedBuffer.resize(this->size, 0);
		this->pixelsPtr = this->ownedBuffer.data();
		return true;
	}

	bool Image::LoadAsPointer(const void* pixels, const ImageDesc& desc)
	{
		Unload();

		DetailedResult result = desc.Validate();
		if (!result)
		{
			Log(LogType::Error, "Failed to load image as pointer. Reason: " + result.errorMessage);
			return false;
		}

		this->desc = desc;
		this->mustFreeSTBI = false;
		this->size = CalculateTotalSize(desc.extent, desc.format, desc.mipLevels, desc.numFaces);
		this->ownedBuffer.clear();
		this->ownedBuffer.shrink_to_fit();
		this->pixelsPtr = (byte*)pixels; // Store pointer
		return true;
	}

	size_t Image::GetMipSize(uint32_t mipIndex) const
	{
		return CalculateMipSize(desc.extent, desc.format, mipIndex);
	}

	uint32_t Image::GetMipRowPitch(uint32_t mipIndex) const
	{
		return CalculateMipRowPitch(desc.extent, desc.format, mipIndex);
	}

	Extent2D Image::GetMipExtent(uint32_t mipIndex) const
	{
		return CalculateMipExtent(desc.extent, mipIndex);
	}

	size_t Image::CalculateTotalSize(Extent2D extent, Format format, uint32_t mipLevels, uint32_t numFaces)
	{
		size_t totalSize = 0;
		for (uint32_t i = 0; i < mipLevels; i++)
		{
			totalSize += CalculateMipSize(extent, format, i);
		}
		return totalSize * numFaces;
	}

	size_t Image::CalculateMipSize(Extent2D extent, Format format, uint32_t mipIndex)
	{
		FormatInfo info = GetFormatInfo(format);
		uint32_t mipWidth = std::max(uint32_t(1), extent.width >> mipIndex);
		uint32_t mipHeight = std::max(uint32_t(1), extent.height >> mipIndex);
		if (info.blockSize > 0) // Block-compressed format
		{
			return (size_t)std::max(uint32_t(1), ((mipWidth + 3) / 4)) * (size_t)std::max(uint32_t(1), ((mipHeight + 3) / 4)) * info.blockSize;
		}
		else
		{
			return std::max(uint32_t(1), mipWidth * mipHeight * info.bytesPerPixel);
		}
	}

	uint32_t Image::CalculateMipRowPitch(Extent2D extent, Format format, uint32_t mipIndex)
	{
		FormatInfo info = GetFormatInfo(format);
		uint32_t mipWidth = std::max(uint32_t(1), extent.width >> mipIndex);
		if (info.blockSize > 0) // Block-compressed format
		{
			return std::max(uint32_t(1), ((mipWidth + 3) / 4)) * info.blockSize;
		}
		else
		{
			return std::max(uint32_t(1), mipWidth * info.bytesPerPixel);
		}
	}

	Extent2D Image::CalculateMipExtent(Extent2D extent, uint32_t mipIndex)
	{
		Extent2D mipExtent;
		mipExtent.width = std::max(uint32_t(1), extent.width >> mipIndex);
		mipExtent.height = std::max(uint32_t(1), extent.height >> mipIndex);
		return mipExtent;
	}

	uint32_t Image::CalculateNumMips(Extent2D extent)
	{
		uint32_t numLevels = 1;
		while (extent.width > 1 && extent.height > 1)
		{
			extent.width = std::max(extent.width / 2, (uint32_t)1);
			extent.height = std::max(extent.height / 2, (uint32_t)1);
			numLevels++;
		}
		return numLevels;
	}

	void* Image::GetMipPixels(uint32_t faceIndex, uint32_t mipIndex) const
	{
		if (!IsLoaded()) return nullptr;

		byte* out = pixelsPtr;
		for (uint32_t f = 0; f < desc.numFaces; f++)
		{
			for (uint32_t m = 0; m < desc.mipLevels; m++)
			{
				if (f == faceIndex && m == mipIndex) return out;
				out += GetMipSize(m);
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
	bool Image::LoadFromDDS(const byte* fileData, size_t numBytes, bool guaranteeOwnership)
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
		ImageDesc tempDesc =
		{
			.extent = Extent2D(header->dwWidth, header->dwHeight),
			.format = Format::None,
			.mipLevels = 1,
			.numFaces = 1,
			.isCubemap = false,
		};
		bool mustConvertBGRToBGRA = false;

		int32_t cubemap = (header->sCaps.dwCaps2 & DDSCAPS2_CUBEMAP) / DDSCAPS2_CUBEMAP;
		if (cubemap)
		{
			tempDesc.isCubemap = true;
			tempDesc.numFaces = 6; // A cubemap has 6 faces
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
				tempDesc.format = Format::BYTE_BYTE_BYTE_BYTE_BGRA;
				if (header->sPixelFormat.dwRBitMask == 0x000000ff)
				{
					tempDesc.format = Format::BYTE_BYTE_BYTE_BYTE; // Order is RGBA, not BGRA
				}
			}
			else if (header->sPixelFormat.dwRGBBitCount == 24)
			{
				// 24 bits per pixel BGR. Must be converted to BGRA.
				mustConvertBGRToBGRA = true;
				tempDesc.format = Format::BYTE_BYTE_BYTE_BYTE_BGRA;
			}
			else if (header->sPixelFormat.dwRGBBitCount == 16)
			{
				Log(LogType::Error, ToString(errStr, "The DDS file contains an unsupported format (16-bits per pixel uncompressed)."));
				return false;
			}
			else if (header->sPixelFormat.dwRGBBitCount == 8)
			{
				tempDesc.format = Format::BYTE;
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

			case 1: tempDesc.format = Format::BC1; break;
			case 3: tempDesc.format = Format::BC2; break;
			case 5: tempDesc.format = Format::BC3; break;
			}
		}

		// If mipmaps exist
		if ((header->sCaps.dwCaps1 & DDSCAPS_MIPMAP) && (header->dwMipMapCount > 1))
		{
			tempDesc.mipLevels = header->dwMipMapCount;
		}

		byte* ddsPixels = (byte*)&fileData[buffer_index];
		failed = false;
		if (mustConvertBGRToBGRA || guaranteeOwnership)
		{
			// If we must convert to different format, then this image must create its own pixel buffer first
			if (!Load(tempDesc))
			{
				failed = true;
			}
		}
		else
		{
			// Store pointer to existing pixel data
			if (!LoadAsPointer(ddsPixels, tempDesc))
			{
				failed = true;
			}
		}
		if (failed)
		{
			Log(LogType::Error, ToString(errStr, "Unable to create the image. This error should be impossible."));
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
			if (guaranteeOwnership)
			{
				// Copy pixel data from DDS file to this image
				memcpy(this->pixelsPtr, ddsPixels, this->size);
			}
		}
		return true;
	}

	bool Image::LoadFromMemory(const byte* fileData, size_t numBytes, bool guaranteeOwnership)
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
			return LoadFromDDS(fileData, numBytes, guaranteeOwnership);
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

		// Graphics API's don't like triple channel texture formats. Force 4 channels if 3 channels detected.
		int forceChannels = (channels == 3) ? 4 : 0;

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

		if (!stbiPixels)
		{
			const char* stbErrorStr = stbi_failure_reason();
			Log(LogType::Error, ToString(errStr, "stb_image returned error: ", stbErrorStr, "."));
			return false;
		}

		if (forceChannels != 0) channels = forceChannels;
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
				" This error should be impossible."));
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
			Log(LogType::Error, ToString(errStr, "Unable to figure out which iglo format to use. This error should be impossible."));
			stbi_image_free(stbiPixels);
			return false;
		}

		// Store a pointer to the pixel data stb_image created
		ImageDesc pointerImageDesc;
		pointerImageDesc.extent = Extent2D(w, h);
		pointerImageDesc.format = tempFormat;
		if (!LoadAsPointer(stbiPixels, pointerImageDesc))
		{
			Log(LogType::Error, ToString(errStr, "Unable to load as pointer. This error should be impossible."));
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
		FormatInfo info = GetFormatInfo(desc.format);
		if (info.blockSize > 0)
		{
			Log(LogType::Error, "Failed to save image to file '" + filename +
				"'. Reason: Image uses block-compression format, which is not supported being saved to file.");
			return false;
		}

		std::string fileExt = utf8_to_lower(GetFileExtension(filename));
		bool success = false;
		bool validExtension = false;
		byte* tempPixels = pixelsPtr;

		std::vector<byte> tempPixelBuffer;
		if (desc.format == Format::BYTE_BYTE_BYTE_BYTE_BGRA ||
			desc.format == Format::BYTE_BYTE_BYTE_BYTE_BGRA_sRGB) // BGRA must be converted to RGBA
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

		switch (desc.format)
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
				success = (bool)stbi_write_png(filename.c_str(), GetWidth(), GetHeight(), info.elementCount, (byte*)tempPixels, 0);
				validExtension = true;
			}
			else if (fileExt == ".bmp")
			{
				success = (bool)stbi_write_bmp(filename.c_str(), GetWidth(), GetHeight(), info.elementCount, (byte*)tempPixels);
				validExtension = true;
			}
			else if (fileExt == ".tga")
			{
				success = (bool)stbi_write_tga(filename.c_str(), GetWidth(), GetHeight(), info.elementCount, (byte*)tempPixels);
				validExtension = true;
			}
			else if (fileExt == ".jpg" || fileExt == ".jpeg")
			{
				success = (bool)stbi_write_jpg(filename.c_str(), GetWidth(), GetHeight(), info.elementCount, (byte*)tempPixels, 90);
				validExtension = true;
			}
			break;
		case Format::FLOAT_FLOAT:
		case Format::FLOAT_FLOAT_FLOAT:
		case Format::FLOAT_FLOAT_FLOAT_FLOAT:
			if (fileExt == ".hdr")
			{
				success = (bool)stbi_write_hdr(filename.c_str(), GetWidth(), GetHeight(), info.elementCount, (float*)tempPixels);
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

		FormatInfo info = GetFormatInfo(desc.format);
		// If sRGB opposite format is found, use it
		if (info.sRGB_opposite != Format::None)
		{
			desc.format = info.sRGB_opposite;
		}
	}
	bool Image::IsSRGB() const
	{
		return GetFormatInfo(desc.format).is_sRGB;
	}

	void Image::ReplaceColors(Color32 colorA, Color32 colorB)
	{
		if (!IsLoaded()) return;

		if (desc.format == ig::Format::BYTE_BYTE_BYTE_BYTE ||
			desc.format == Format::BYTE_BYTE_BYTE_BYTE_NotNormalized ||
			desc.format == Format::BYTE_BYTE_BYTE_BYTE_sRGB ||
			desc.format == ig::Format::BYTE_BYTE_BYTE_BYTE_BGRA ||
			desc.format == Format::BYTE_BYTE_BYTE_BYTE_BGRA_sRGB)
		{
			if (colorA == colorB)
			{
				return; // nothing needs to be done
			}

			uint32_t findValue = colorA.rgba;
			uint32_t replaceValue = colorB.rgba;

			// Convert colorA and colorB from RGBA to BGRA
			if (desc.format == ig::Format::BYTE_BYTE_BYTE_BYTE_BGRA ||
				desc.format == Format::BYTE_BYTE_BYTE_BYTE_BGRA_sRGB)
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

	void IGLOContext::SetModalLoopCallback(CallbackModalLoop callbackModalLoop)
	{
		if (!isLoaded)
		{
			Log(LogType::Error, "Failed to set modal loop callback. Reason: IGLOContext must be loaded!");
			return;
		}

		this->callbackModalLoop = callbackModalLoop;
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

	void IGLOContext::Present()
	{
#ifdef IGLO_D3D12
		HRESULT hr = 0;
		switch (swapChain.presentMode)
		{
		case PresentMode::ImmediateWithTearing:
			hr = graphics.swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
			break;
		case PresentMode::Immediate:
			hr = graphics.swapChain->Present(0, 0);
			break;
		case PresentMode::Vsync:
			hr = graphics.swapChain->Present(1, 0);
			break;
		case PresentMode::VsyncHalf:
			hr = graphics.swapChain->Present(2, 0);
			break;
		default:
			Log(LogType::Error, "Presentation failed. Reason: Invalid present mode.");
			break;
		}
		if (FAILED(hr))
		{
			Log(LogType::Error, ToString("Presentation failed. Reason: IDXGISwapChain::Present returned error code: ", (uint32_t)hr, "."));

			hr = graphics.device->GetDeviceRemovedReason();
			if (FAILED(hr))
			{
				std::string reasonStr;
				switch (hr)
				{
				case DXGI_ERROR_DEVICE_HUNG: reasonStr = "The device has hung."; break;
				case DXGI_ERROR_DEVICE_REMOVED: reasonStr = "The device was removed."; break;
				case DXGI_ERROR_DEVICE_RESET: reasonStr = "The device was reset."; break;
				default:
					reasonStr = "Unknown device removal reason.";
					break;
				}
				Log(LogType::Error, "Device removal detected! Reason: " + reasonStr);
				if (callbackOnDeviceRemoved) callbackOnDeviceRemoved(reasonStr);
			}
		}

		// Wait until the swap chain is ready to present the next frame.
		// This ensures that the value passed to SetMaximumFrameLatency() is respected.
		// Waiting for this at the start of the next frame reduces input latency.
		// Info:
		// https://learn.microsoft.com/en-us/windows/uwp/gaming/reduce-latency-with-dxgi-1-3-swap-chains
		// https://www.intel.com/content/www/us/en/developer/articles/code-sample/sample-application-for-direct3d-12-flip-model-swap-chains.html
		HANDLE swapchainWaitableObject = graphics.swapChain->GetFrameLatencyWaitableObject();
		WaitForSingleObjectEx(swapchainWaitableObject, 1000, true);
#endif
#ifdef IGLO_VULKAN
		if (graphics.validSwapChain)
		{
			VkResult result = commandQueue.Present(graphics.swapChain);
			HandleVulkanSwapChainResult(result, "presentation");
		}
#endif

		endOfFrame[frameIndex].graphicsReceipt = commandQueue.SubmitSignal(CommandListType::Graphics);

		MoveToNextFrame();
	}

	Receipt IGLOContext::Submit(const CommandList& commandList)
	{
		const CommandList* cmd[] = { &commandList };
		return Submit(cmd, 1);
	}

	Receipt IGLOContext::Submit(const CommandList* const* commandLists, uint32_t numCommandLists)
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
				throw std::runtime_error("This should be impossible.");
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
			// We can safely free all temp resources when GPU is idle.
			FreeAllTempResources();
		}
	}

	void IGLOContext::WaitForIdleDevice()
	{
		commandQueue.WaitForIdle();

		// We can safely free all temp resources when GPU is idle.
		FreeAllTempResources();
	}

	void IGLOContext::FreeAllTempResources()
	{
		descriptorHeap.FreeAllTempResources();
		tempBufferAllocator.FreeAllTempPages();
		for (size_t i = 0; i < endOfFrame.size(); i++)
		{
			endOfFrame[i].delayedUnloadTextures.clear();
		}
	}

	void IGLOContext::MoveToNextFrame()
	{
		frameIndex = (frameIndex + 1) % numFramesInFlight;

		if (numFramesInFlight > maxFramesInFlight) throw std::runtime_error("This should be impossible.");
		if (numFramesInFlight > swapChain.numBackBuffers) throw std::runtime_error("This should be impossible.");
		if (frameIndex >= endOfFrame.size()) throw std::runtime_error("This should be impossible.");

		EndOfFrame& currentFrame = endOfFrame[frameIndex];
		commandQueue.WaitForCompletion(currentFrame.graphicsReceipt);
		commandQueue.WaitForCompletion(currentFrame.computeReceipt);
		commandQueue.WaitForCompletion(currentFrame.copyReceipt);

		// The delayed unload textures are held for max 1 full frame cycle
		currentFrame.delayedUnloadTextures.clear();

		descriptorHeap.NextFrame();
		tempBufferAllocator.NextFrame();

#ifdef IGLO_VULKAN
		if (graphics.validSwapChain)
		{
			VkResult result = commandQueue.AcquireNextVulkanSwapChainImage(graphics.device, graphics.swapChain,
				graphics.swapChainUsesMinImageCount ? 0 : UINT64_MAX);
			HandleVulkanSwapChainResult(result, "image acquisition");
		}
		if (!graphics.validSwapChain)
		{
			WaitForIdleDevice();
			DetailedResult dr = CreateSwapChain(swapChain.extent, swapChain.format, swapChain.numBackBuffers,
				numFramesInFlight, swapChain.presentMode);
			if (!dr)
			{
				Log(LogType::Error, "Failed to replace swapchain. Reason: " + dr.errorMessage);
			}
			WaitForIdleDevice();
		}
#endif
	}

	void IGLOContext::DelayedTextureUnload(std::shared_ptr<Texture> texture) const
	{
		endOfFrame[frameIndex].delayedUnloadTextures.push_back(texture);
	}

	Descriptor IGLOContext::CreateTempConstant(const void* data, uint64_t numBytes) const
	{
		const char* errStr = "Failed to create temporary shader constant. Reason: ";

		Descriptor outDescriptor = descriptorHeap.AllocateTemp(DescriptorType::ConstantBuffer_CBV);
		if (outDescriptor.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate temporary resource descriptor."));
			return Descriptor();
		}

		TempBuffer tempBuffer = tempBufferAllocator.AllocateTempBuffer(numBytes, GetGraphicsSpecs().bufferPlacementAlignments.constant);
		if (tempBuffer.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate temporary buffer."));
			return Descriptor();
		}

		memcpy(tempBuffer.data, data, numBytes);

#ifdef IGLO_D3D12
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
		desc.BufferLocation = tempBuffer.impl.resource->GetGPUVirtualAddress() + tempBuffer.offset;
		desc.SizeInBytes = (UINT)AlignUp(numBytes, GetGraphicsSpecs().bufferPlacementAlignments.constant);
		graphics.device->CreateConstantBufferView(&desc, descriptorHeap.GetD3D12CPUHandle(outDescriptor));
#endif
#ifdef IGLO_VULKAN
		descriptorHeap.WriteBufferDescriptor(outDescriptor, tempBuffer.impl.buffer, tempBuffer.offset, numBytes);
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

		Descriptor outDescriptor = descriptorHeap.AllocateTemp(DescriptorType::RawOrStructuredBuffer_SRV_UAV);
		if (outDescriptor.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate temporary resource descriptor."));
			return Descriptor();
		}

#ifdef IGLO_D3D12
		// In D3D12, structured buffers need a special non-power of 2 element stride alignment,
		// which is why we allocate 1 extra element here.
		TempBuffer tempBuffer = tempBufferAllocator.AllocateTempBuffer(numBytes + elementStride,
			GetGraphicsSpecs().bufferPlacementAlignments.rawOrStructuredBuffer);
		if (tempBuffer.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate temporary buffer."));
			return Descriptor();
		}

		// We pretend the entire page is an array of elements.
		// We must figure out the index of our first element, aligned upwards to 'elementStride'.
		byte* beginningOfPage = (byte*)tempBuffer.data - tempBuffer.offset;
		uint64_t firstElementIndex = (tempBuffer.offset / elementStride) + 1; // +1 to align upwards
		uint64_t firstElementOffset = firstElementIndex * elementStride; // Byte offset of our first element
		memcpy(beginningOfPage + firstElementOffset, data, numBytes);

		D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
		srv.Format = DXGI_FORMAT_UNKNOWN;
		srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv.Buffer.FirstElement = firstElementIndex;
		srv.Buffer.NumElements = numElements;
		srv.Buffer.StructureByteStride = elementStride;
		srv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		graphics.device->CreateShaderResourceView(tempBuffer.impl.resource, &srv, descriptorHeap.GetD3D12CPUHandle(outDescriptor));
#endif
#ifdef IGLO_VULKAN
		TempBuffer tempBuffer = tempBufferAllocator.AllocateTempBuffer(numBytes,
			GetGraphicsSpecs().bufferPlacementAlignments.rawOrStructuredBuffer);
		if (tempBuffer.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate temporary buffer."));
			return Descriptor();
		}

		memcpy(tempBuffer.data, data, numBytes);
		descriptorHeap.WriteBufferDescriptor(outDescriptor, tempBuffer.impl.buffer, tempBuffer.offset, numBytes);
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

		Descriptor outDescriptor = descriptorHeap.AllocateTemp(DescriptorType::RawOrStructuredBuffer_SRV_UAV);
		if (outDescriptor.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate temporary resource descriptor."));
			return Descriptor();
		}

		TempBuffer tempBuffer = tempBufferAllocator.AllocateTempBuffer(numBytes,
			GetGraphicsSpecs().bufferPlacementAlignments.rawOrStructuredBuffer);
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

		graphics.device->CreateShaderResourceView(tempBuffer.impl.resource, &srv, descriptorHeap.GetD3D12CPUHandle(outDescriptor));
#endif
#ifdef IGLO_VULKAN
		descriptorHeap.WriteBufferDescriptor(outDescriptor, tempBuffer.impl.buffer, tempBuffer.offset, numBytes);
#endif

		return outDescriptor;
	}

	void Buffer::Unload()
	{
		Impl_Unload();

		for (Descriptor d : descriptor_SRV_or_CBV)
		{
			if (!d.IsNull()) context->GetDescriptorHeap().FreePersistent(d);
		}
		descriptor_SRV_or_CBV.clear();
		descriptor_SRV_or_CBV.shrink_to_fit();

		if (!descriptor_UAV.IsNull()) context->GetDescriptorHeap().FreePersistent(descriptor_UAV);
		descriptor_UAV.SetToNull();

		isLoaded = false;
		context = nullptr;
		type = BufferType::VertexBuffer;
		usage = BufferUsage::Default;
		size = 0;
		stride = 0;
		numElements = 0;
		dynamicSetCounter = 0;

		mapped.clear();
		mapped.shrink_to_fit();

	}

	Buffer::Buffer(Buffer&& other) noexcept
	{
		*this = std::move(other);
	}
	Buffer& Buffer::operator=(Buffer&& other) noexcept
	{
		Unload();

		std::swap(this->isLoaded, other.isLoaded);
		std::swap(this->context, other.context);
		std::swap(this->type, other.type);
		std::swap(this->usage, other.usage);
		std::swap(this->size, other.size);
		std::swap(this->stride, other.stride);
		std::swap(this->numElements, other.numElements);
		std::swap(this->descriptor_SRV_or_CBV, other.descriptor_SRV_or_CBV);
		std::swap(this->descriptor_UAV, other.descriptor_UAV);
		std::swap(this->mapped, other.mapped);
		std::swap(this->dynamicSetCounter, other.dynamicSetCounter);

		std::swap(this->impl, other.impl);

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
			if (usage == BufferUsage::Dynamic) return &descriptor_SRV_or_CBV[dynamicSetCounter];
			return &descriptor_SRV_or_CBV[0];
		default:
			return nullptr;
		}
	}

	const Descriptor* Buffer::GetUnorderedAccessDescriptor() const
	{
		if (!isLoaded) return nullptr;
		if (descriptor_UAV.IsNull()) return nullptr;
		return &descriptor_UAV;
	}

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
		default:
			throw std::invalid_argument("This should be impossible.");
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
				return false;
			}
			if (stride != 0)
			{
				if (stride % 4 != 0)
				{
					Log(LogType::Error, ToString(errStr, "The stride of this type of buffer must be a multiple of 4."));
					return false;
				}
			}
		}

		this->context = &context;
		this->type = type;
		this->usage = usage;
		this->size = size;
		this->stride = stride;
		this->numElements = numElements;

		DetailedResult result = Impl_InternalLoad(context, size, stride, numElements, usage, type);
		if (!result)
		{
			Log(LogType::Error, ToString(errStr, result.errorMessage));
			Unload();
			return false;
		}

		this->isLoaded = true;
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

		TempBuffer tempBuffer = context->GetTempBufferAllocator().AllocateTempBuffer(size, 1);
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

		dynamicSetCounter = (dynamicSetCounter + 1) % context->GetMaxFramesInFlight();

		assert(dynamicSetCounter < mapped.size());
		if (!mapped[dynamicSetCounter]) throw std::runtime_error("This should be impossible.");

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
		assert(frameIndex < mapped.size());
		if (!mapped[frameIndex]) throw std::runtime_error("This should be impossible.");

		memcpy(destData, mapped[frameIndex], size);
	}

	void Texture::Unload()
	{
		Impl_Unload();

		if (!isWrapped)
		{
			if (!srvDescriptor.IsNull()) context->GetDescriptorHeap().FreePersistent(srvDescriptor);
			if (!uavDescriptor.IsNull()) context->GetDescriptorHeap().FreePersistent(uavDescriptor);
		}
		srvDescriptor.SetToNull();
		uavDescriptor.SetToNull();

		isLoaded = false;
		context = nullptr;
		desc = TextureDesc();

		readMapped.clear();
		readMapped.shrink_to_fit();

		isWrapped = false;
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
		std::swap(this->desc, other.desc);
		std::swap(this->srvDescriptor, other.srvDescriptor);
		std::swap(this->uavDescriptor, other.uavDescriptor);
		std::swap(this->readMapped, other.readMapped);
		std::swap(this->isWrapped, other.isWrapped);

		std::swap(this->impl, other.impl);

		return *this;
	}

	const Descriptor* Texture::GetDescriptor() const
	{
		if (!isLoaded) return nullptr;
		if (srvDescriptor.IsNull()) return nullptr;
		return &srvDescriptor;
	}

	const Descriptor* Texture::GetUnorderedAccessDescriptor() const
	{
		if (!isLoaded) return nullptr;
		if (uavDescriptor.IsNull()) return nullptr;
		return &uavDescriptor;
	}

	bool Texture::Load(const IGLOContext& context, uint32_t width, uint32_t height, Format format,
		TextureUsage usage, MSAA msaa, ClearValue optimizedClearValue)
	{
		TextureDesc desc;
		desc.extent = Extent2D(width, height);
		desc.format = format;
		desc.usage = usage;
		desc.msaa = msaa;
		desc.optimizedClearValue = optimizedClearValue;
		return Load(context, desc);
	}

	bool Texture::Load(const IGLOContext& context, const TextureDesc& desc)
	{
		Unload();

		const char* errStr = "Failed to create texture. Reason: ";

		switch (desc.usage)
		{
		case TextureUsage::Default:
		case TextureUsage::Readable:
		case TextureUsage::UnorderedAccess:
		case TextureUsage::RenderTexture:
		case TextureUsage::DepthBuffer:
			break;
		default:
			throw std::invalid_argument("Invalid texture usage.");
		}

		if (desc.extent.width == 0 || desc.extent.height == 0)
		{
			Log(LogType::Error, ToString(errStr, "Texture dimensions must be atleast 1x1."));
			return false;
		}
		if (desc.numFaces == 0 || desc.mipLevels == 0)
		{
			Log(LogType::Error, ToString(errStr, "Texture must have atleast 1 face and 1 mip level."));
			return false;
		}
		if (desc.format == Format::None)
		{
			Log(LogType::Error, ToString(errStr, "Invalid texture format."));
			return false;
		}
		if (desc.isCubemap && desc.numFaces % 6 != 0)
		{
			Log(LogType::Error, ToString(errStr, "For cubemap textures, number of faces must be a multiple of 6."));
			return false;
		}

		this->context = &context;
		this->desc = desc;
		this->isWrapped = false;

		DetailedResult result = Impl_Load(context, desc);
		if (!result)
		{
			Log(LogType::Error, ToString(errStr, result.errorMessage));
			Unload();
			return false;
		}

		this->isLoaded = true;
		return true;
	}

	bool Texture::LoadAsWrapped(const IGLOContext& context, const WrappedTextureDesc& desc)
	{
		Unload();

		const char* errStr = "Failed to load wrapped texture. Reason: ";

		size_t expectedResources = 1;
		size_t expectedMemories = 1;
		size_t expectedMappedPtrs = 0;
		if (desc.textureDesc.usage == TextureUsage::Readable)
		{
			expectedResources = context.GetMaxFramesInFlight();
			expectedMemories = context.GetMaxFramesInFlight();
			expectedMappedPtrs = context.GetMaxFramesInFlight();
		}

#ifdef IGLO_D3D12
		size_t providedResources = desc.impl.resource.size();
		size_t providedMemories = desc.impl.resource.size();
#endif
#ifdef IGLO_VULKAN
		size_t providedResources = desc.impl.image.size();
		size_t providedMemories = desc.impl.memory.size();
#endif
		size_t providedMappedPtrs = desc.readMapped.size();

		if (expectedResources != providedResources ||
			expectedMemories != providedMemories ||
			expectedMappedPtrs != providedMappedPtrs)
		{
			Log(LogType::Error, ToString(errStr, "Unexpected vector size for impl.resource, impl.image, impl.memory or readMapped."));
			return false;
		}

		this->isLoaded = true;
		this->context = &context;
		this->desc = desc.textureDesc;
		this->srvDescriptor = desc.srvDescriptor;
		this->uavDescriptor = desc.uavDescriptor;
		this->readMapped = desc.readMapped;
		this->isWrapped = true;
		this->impl = desc.impl;
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
		if (sRGB) // User requests sRGB format
		{
			image.SetSRGB(true);
			if (!image.IsSRGB())
			{
				Log(LogType::Warning, ToString("A texture was unable to use an sRGB format as requested,"
					" because no sRGB equivalent was found for this iglo format: ", (uint32_t)image.GetFormat(), "."));
			}
		}
		return LoadFromMemory(context, cmd, image, generateMipmaps);
	}

	bool Texture::LoadFromMemory(const IGLOContext& context, CommandList& cmd, const Image& image, bool generateMipmaps)
	{
		Unload();

		const char* errStr = "Failed to load texture from image. Reason: ";

		if (!image.IsLoaded())
		{
			Log(LogType::Error, ToString(errStr, "The provided image is not loaded."));
			return false;
		}

		// Should we generate mips?
		uint32_t fullMipLevels = Image::CalculateNumMips(image.GetExtent());
		bool proceedWithMipGen = generateMipmaps && (fullMipLevels > 1) && (image.GetMipLevels() == 1);

		// Can we generate mips?
		if (proceedWithMipGen)
		{
			DetailedResult result = ValidateMipGeneration(cmd.GetCommandListType(), image);
			if (!result)
			{
				Log(LogType::Warning, "Unable to generate mipmaps for texture. Reason: " + result.errorMessage);
				proceedWithMipGen = false;
			}
		}

		// Load with appropriate mip level count
		TextureDesc selfDesc;
		selfDesc.extent = Extent2D(image.GetWidth(), image.GetHeight());
		selfDesc.format = image.GetFormat();
		selfDesc.usage = TextureUsage::Default;
		selfDesc.msaa = MSAA::Disabled;
		selfDesc.isCubemap = image.IsCubemap();
		selfDesc.numFaces = image.GetNumFaces();
		selfDesc.mipLevels = proceedWithMipGen ? fullMipLevels : image.GetMipLevels();
		if (!Load(context, selfDesc))
		{
			// Load() will have already unloaded the texture and logged an error message if it failed.
			return false;
		}

		if (proceedWithMipGen)
		{
			DetailedResult result = GenerateMips(cmd, image);
			if (!result)
			{
				Log(LogType::Error, ToString(errStr, result.errorMessage));
				Unload();
				return false;
			}
		}
		else
		{
			cmd.AddTextureBarrier(*this, SimpleBarrier::Discard, SimpleBarrier::CopyDest);
			cmd.FlushBarriers();

			SetPixels(cmd, image);

			if (cmd.GetCommandListType() != CommandListType::Copy)
			{
				cmd.AddTextureBarrier(*this, SimpleBarrier::CopyDest, SimpleBarrier::PixelShaderResource);
				cmd.FlushBarriers();
			}
		}

		return true;
	}

	DetailedResult Texture::ValidateMipGeneration(CommandListType cmdListType, const Image& image)
	{
		if (!image.IsLoaded())
		{
			return DetailedResult::MakeFail("The provided image isn't loaded.");
		}
		if (GetFormatInfo(image.GetFormat()).blockSize != 0)
		{
			return DetailedResult::MakeFail("Mipmap generation is not supported for block compression formats.");
		}
		else if (image.GetNumFaces() > 1)
		{
			return DetailedResult::MakeFail("Mipmap generation is not yet supported for cube maps and texture arrays.");
		}
		else if (!IsPowerOf2(image.GetWidth()) || !IsPowerOf2(image.GetHeight()))
		{
			return DetailedResult::MakeFail("Mipmap generation is not yet supported for non power of 2 textures.");
		}
		else if (cmdListType == CommandListType::Copy)
		{
			return DetailedResult::MakeFail("Mipmap generation can't be performed with a 'Copy' command list type.");
		}

		return DetailedResult::MakeSuccess();
	}

	DetailedResult Texture::GenerateMips(CommandList& cmd, const Image& image)
	{
		if (!isLoaded || desc.mipLevels <= 1 || !image.IsLoaded() || image.GetExtent() != desc.extent)
		{
			throw std::runtime_error("This should be impossible.");
		}

		Extent2D nextMipExtent = Image::CalculateMipExtent(desc.extent, 1);
		FormatInfo formatInfo = GetFormatInfo(desc.format);
		Format format_non_sRGB = formatInfo.is_sRGB ? formatInfo.sRGB_opposite : desc.format;

		// Create an unordered access texture with one less miplevel
		std::shared_ptr<Texture> unordered = std::make_shared<Texture>();
		TextureDesc unorderedDesc;
		unorderedDesc.extent = nextMipExtent;
		unorderedDesc.format = format_non_sRGB;
		unorderedDesc.usage = TextureUsage::UnorderedAccess;
		unorderedDesc.msaa = MSAA::Disabled;
		unorderedDesc.isCubemap = false;
		unorderedDesc.numFaces = desc.numFaces;
		unorderedDesc.mipLevels = desc.mipLevels - 1;
		unorderedDesc.createDescriptors = false;
		if (!unordered->Load(*context, unorderedDesc))
		{
			return DetailedResult::MakeFail("Failed to create unordered access texture for mipmap generation.");
		}

		// To give the GPU enough time to use this texture, we will keep it alive past this function.
		context->DelayedTextureUnload(unordered);

		cmd.AddTextureBarrierAtSubresource(*this, SimpleBarrier::Discard, SimpleBarrier::CopyDest, 0, 0);
		cmd.FlushBarriers();

		// Upload the first and largest mipmap to this texture
		SetPixelsAtSubresource(cmd, image, 0, 0);

		struct MipmapGenPushConstants
		{
			uint32_t srcTextureIndex = IGLO_UINT32_MAX;
			uint32_t destTextureIndex = IGLO_UINT32_MAX;
			uint32_t bilinearClampSamplerIndex = IGLO_UINT32_MAX;
			uint32_t is_sRGB = 0;
			Vector2 inverseDestTextureSize;
		};
		MipmapGenPushConstants pushConstants;
		pushConstants.bilinearClampSamplerIndex = context->GetBilinearClampSamplerDescriptor()->heapIndex;
		pushConstants.is_sRGB = formatInfo.is_sRGB;

		for (uint32_t i = 0; i < desc.mipLevels - 1; i++)
		{
			DescriptorHeap& heap = context->GetDescriptorHeap();

			Descriptor srv = heap.AllocateTemp(DescriptorType::Texture_SRV);
			Descriptor uav = heap.AllocateTemp(DescriptorType::Texture_UAV);
			if (srv.IsNull() || uav.IsNull())
			{
				return DetailedResult::MakeFail("Failed to allocate temporary resource descriptors for mipmap generation.");
			}
			pushConstants.srcTextureIndex = srv.heapIndex;
			pushConstants.destTextureIndex = uav.heapIndex;

			Extent2D destDimensions = Image::CalculateMipExtent(image.GetExtent(), i + 1);
			pushConstants.inverseDestTextureSize = Vector2(1.0f / (float)destDimensions.width, 1.0f / (float)destDimensions.height);

#ifdef IGLO_D3D12
			auto device = context->GetD3D12Device();

			// SRV
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = GetFormatInfoDXGI(image.GetFormat()).dxgiFormat;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = i;
			device->CreateShaderResourceView(this->GetD3D12Resource(), &srvDesc, heap.GetD3D12CPUHandle(srv));

			// UAV
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = GetFormatInfoDXGI(format_non_sRGB).dxgiFormat;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = i;
			device->CreateUnorderedAccessView(unordered->GetD3D12Resource(), nullptr, &uavDesc, heap.GetD3D12CPUHandle(uav));
#endif
#ifdef IGLO_VULKAN
			VkDevice device = context->GetVulkanDevice();

			// SRV
			VkImageViewCreateInfo srcViewInfo = {};
			srcViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			srcViewInfo.image = GetVulkanImage();
			srcViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			srcViewInfo.format = ConvertToVulkanFormat(image.GetFormat());
			srcViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			srcViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			srcViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			srcViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			srcViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			srcViewInfo.subresourceRange.baseMipLevel = i;
			srcViewInfo.subresourceRange.levelCount = 1;
			srcViewInfo.subresourceRange.baseArrayLayer = 0;
			srcViewInfo.subresourceRange.layerCount = 1;
			VkImageView srcImageView = VK_NULL_HANDLE;
			if (heap.CreateTempVulkanImageView(device, &srcViewInfo, nullptr, &srcImageView) != VK_SUCCESS)
			{
				return DetailedResult::MakeFail("Failed to create source image view for mipmap generation.");
			}
			heap.WriteImageDescriptor(srv, srcImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			// UAV
			VkImageViewCreateInfo destViewInfo = srcViewInfo;
			destViewInfo.image = unordered->GetVulkanImage();
			destViewInfo.format = ConvertToVulkanFormat(format_non_sRGB);
			VkImageView destImageView = VK_NULL_HANDLE;
			if (heap.CreateTempVulkanImageView(device, &destViewInfo, nullptr, &destImageView) != VK_SUCCESS)
			{
				return DetailedResult::MakeFail("Failed to create destination image view for mipmap generation.");
			}
			heap.WriteImageDescriptor(uav, destImageView, VK_IMAGE_LAYOUT_GENERAL);

#endif

			cmd.AddTextureBarrierAtSubresource(*this, SimpleBarrier::CopyDest, SimpleBarrier::ComputeShaderResource, 0, i);
			cmd.AddTextureBarrierAtSubresource(*unordered, SimpleBarrier::Discard, SimpleBarrier::ComputeShaderUnorderedAccess, 0, i);
			cmd.FlushBarriers();

			cmd.SetPipeline(context->GetMipmapGenerationPipeline());
			cmd.SetComputePushConstants(&pushConstants, sizeof(pushConstants));
			cmd.DispatchCompute(
				std::max(destDimensions.width / 8, 1U),
				std::max(destDimensions.height / 8, 1U),
				1);

			cmd.AddTextureBarrierAtSubresource(*this, SimpleBarrier::ComputeShaderResource, SimpleBarrier::PixelShaderResource, 0, i);
			cmd.AddTextureBarrierAtSubresource(*this, SimpleBarrier::Discard, SimpleBarrier::CopyDest, 0, i + 1);
			cmd.AddTextureBarrierAtSubresource(*unordered, SimpleBarrier::ComputeShaderUnorderedAccess, SimpleBarrier::CopySource, 0, i);
			cmd.FlushBarriers();

			cmd.CopyTextureSubresource(*unordered, 0, i, *this, 0, i + 1);
		}

		cmd.AddTextureBarrierAtSubresource(*this, SimpleBarrier::CopyDest, SimpleBarrier::PixelShaderResource, 0, desc.mipLevels - 1);
		cmd.FlushBarriers();

		return DetailedResult::MakeSuccess();
	}

	void Texture::SetPixels(CommandList& cmd, const Image& srcImage)
	{
		const char* errStr = "Failed to set texture pixels. Reason: ";
		if (!isLoaded || desc.usage == TextureUsage::Readable)
		{
			Log(LogType::Error, ToString(errStr, "Texture must be created with non-readable texture usage."));
			return;
		}
		if (desc.msaa != MSAA::Disabled)
		{
			Log(LogType::Error, ToString(errStr, "Texture is multisampled."));
			return;
		}
		if (!srcImage.IsLoaded() ||
			srcImage.GetExtent() != desc.extent ||
			srcImage.GetFormat() != desc.format ||
			srcImage.GetNumFaces() != desc.numFaces ||
			srcImage.GetMipLevels() != desc.mipLevels)
		{
			Log(LogType::Error, ToString(errStr, "Source image dimensions must match texture."));
			return;
		}

#ifdef IGLO_D3D12
		uint64_t requiredUploadBufferSize = 0;
		{
			D3D12_RESOURCE_DESC resDesc = impl.resource[0]->GetDesc();
			context->GetD3D12Device()->GetCopyableFootprints(&resDesc, 0, desc.numFaces * desc.mipLevels, 0,
				nullptr, nullptr, nullptr, &requiredUploadBufferSize);
		}
#endif
#ifdef IGLO_VULKAN
		uint64_t requiredUploadBufferSize = GetRequiredUploadBufferSize(srcImage, context->GetGraphicsSpecs().bufferPlacementAlignments);
#endif

		TempBuffer tempBuffer = context->GetTempBufferAllocator().AllocateTempBuffer(requiredUploadBufferSize,
			context->GetGraphicsSpecs().bufferPlacementAlignments.texture);
		if (tempBuffer.IsNull())
		{
			Log(LogType::Error, ToString(errStr, "Failed to allocate temporary buffer."));
			return;
		}

		byte* destPtr = (byte*)tempBuffer.data;
		byte* srcPtr = (byte*)srcImage.GetPixels();
		for (uint32_t faceIndex = 0; faceIndex < srcImage.GetNumFaces(); faceIndex++)
		{
			for (uint32_t mipIndex = 0; mipIndex < srcImage.GetMipLevels(); mipIndex++)
			{
				size_t srcRowPitch = srcImage.GetMipRowPitch(mipIndex);
				uint64_t destRowPitch = AlignUp(srcRowPitch, context->GetGraphicsSpecs().bufferPlacementAlignments.textureRowPitch);
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

		ImageDesc imageDesc;
		imageDesc.extent = desc.extent;
		imageDesc.format = desc.format;
		imageDesc.mipLevels = desc.mipLevels;
		imageDesc.numFaces = desc.numFaces;
		imageDesc.isCubemap = desc.isCubemap;

		Image image;
		image.LoadAsPointer((byte*)pixelData, imageDesc);

		SetPixels(cmd, image);
	}

	void Texture::SetPixelsAtSubresource(CommandList& cmd, const Image& srcImage, uint32_t destFaceIndex, uint32_t destMipIndex)
	{
		const char* errStr = "Failed to set texture pixels at subresource. Reason: ";
		if (!isLoaded || desc.usage == TextureUsage::Readable)
		{
			Log(LogType::Error, ToString(errStr, "Texture must be created with non-readable texture usage."));
			return;
		}
		if (desc.msaa != MSAA::Disabled)
		{
			Log(LogType::Error, ToString(errStr, "Texture is multisampled."));
			return;
		}
		Extent2D subResDimensions = Image::CalculateMipExtent(desc.extent, destMipIndex);
		if (!srcImage.IsLoaded() ||
			srcImage.GetExtent() != subResDimensions ||
			srcImage.GetFormat() != desc.format ||
			srcImage.GetNumFaces() != 1 ||
			srcImage.GetMipLevels() != 1)
		{
			Log(LogType::Error, ToString(errStr, "Source image is expected to have 1 face, 1 miplevel,"
				" and the dimensions ", subResDimensions.ToString(), "."));
			return;
		}

#ifdef IGLO_D3D12
		uint32_t subResourceIndex = (destFaceIndex * desc.mipLevels) + destMipIndex;
		uint64_t requiredUploadBufferSize = 0;
		{
			D3D12_RESOURCE_DESC resDesc = impl.resource[0]->GetDesc();
			context->GetD3D12Device()->GetCopyableFootprints(&resDesc, subResourceIndex, 1, 0, nullptr, nullptr, nullptr, &requiredUploadBufferSize);
		}
#endif
#ifdef IGLO_VULKAN
		uint64_t requiredUploadBufferSize = GetRequiredUploadBufferSize(srcImage, context->GetGraphicsSpecs().bufferPlacementAlignments);
#endif

		TempBuffer tempBuffer = context->GetTempBufferAllocator().AllocateTempBuffer(requiredUploadBufferSize,
			context->GetGraphicsSpecs().bufferPlacementAlignments.texture);
		if (tempBuffer.IsNull())
		{
			Log(LogType::Error, "Failed to set texture pixels. Reason: Failed to allocate temporary buffer.");
			return;
		}

		byte* destPtr = (byte*)tempBuffer.data;
		byte* srcPtr = (byte*)srcImage.GetPixels();
		size_t srcRowPitch = srcImage.GetMipRowPitch(0);
		uint64_t destRowPitch = AlignUp(srcRowPitch, context->GetGraphicsSpecs().bufferPlacementAlignments.textureRowPitch);
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

		ImageDesc imageDesc;
		imageDesc.extent = Image::CalculateMipExtent(desc.extent, destMipIndex);
		imageDesc.format = desc.format;
		
		Image image;
		image.LoadAsPointer((byte*)pixelData, imageDesc);

		SetPixelsAtSubresource(cmd, image, destFaceIndex, destMipIndex);
	}

	void Texture::ReadPixels(Image& destImage)
	{
		if (!isLoaded || desc.usage != TextureUsage::Readable)
		{
			Log(LogType::Error, "Failed to read texture pixels. Reason: Texture must be created with Readable usage.");
			return;
		}
		if (desc.msaa != MSAA::Disabled)
		{
			Log(LogType::Error, "Failed to read texture pixels. Reason: Texture is multisampled.");
			return;
		}
		if (!destImage.IsLoaded() ||
			destImage.GetExtent() != desc.extent ||
			destImage.GetFormat() != desc.format ||
			destImage.GetNumFaces() != desc.numFaces ||
			destImage.GetMipLevels() != desc.mipLevels)
		{
			Log(LogType::Error, "Failed to read texture pixels. Reason: Image dimension must match texture.");
			return;
		}

		if (readMapped.size() != context->GetMaxFramesInFlight()) throw std::runtime_error("This should be impossible.");

		byte* destPtr = (byte*)destImage.GetPixels();
		byte* srcPtr = (byte*)readMapped[context->GetFrameIndex()];
		for (uint32_t faceIndex = 0; faceIndex < desc.numFaces; faceIndex++)
		{
			for (uint32_t mipIndex = 0; mipIndex < desc.mipLevels; mipIndex++)
			{
				size_t srcRowPitch = AlignUp(Image::CalculateMipRowPitch(desc.extent, desc.format, mipIndex),
					context->GetGraphicsSpecs().bufferPlacementAlignments.textureRowPitch);
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
		ImageDesc imageDesc;
		imageDesc.extent = desc.extent;
		imageDesc.format = desc.format;
		imageDesc.mipLevels = desc.mipLevels;
		imageDesc.numFaces = desc.numFaces;
		imageDesc.isCubemap = desc.isCubemap;
		
		Image image;
		if (!image.Load(imageDesc))
		{
			Log(LogType::Error, "Failed to read texture pixels to image. Reason: Failed to create image.");
			return image;
		}

		ReadPixels(image);
		return image;
	}

	void Sampler::Unload()
	{
#ifdef IGLO_VULKAN
		if (vkSampler) vkDestroySampler(context->GetVulkanDevice(), vkSampler, nullptr);
		vkSampler = VK_NULL_HANDLE;
#endif

		if (!descriptor.IsNull()) context->GetDescriptorHeap().FreePersistent(descriptor);
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

		this->context = &context;

		DetailedResult result = Impl_Load(context, desc);
		if (!result)
		{
			Log(LogType::Error, "Failed to create sampler state. Reason: " + result.errorMessage);
			Unload();
			return false;
		}

		this->isLoaded = true;
		return true;
	}


} //end of namespace ig
