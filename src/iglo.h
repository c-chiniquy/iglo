/*
	iglo is a low-level, public domain C++ rendering abstraction layer for D3D12 and Vulkan.
 	https://github.com/c-chiniquy/iglo
	See the bottom of this document for licensing details.
*/

#pragma once
namespace ig
{
	static const char* igloVersion = "iglo v0.1.1";
}

//////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// OS Includes ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
// Use unicode
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

// Windows
#ifdef _WIN32 
#define NOMINMAX // Conflicts with std::min std::max
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifdef _WIN64
#ifndef WIN64_LEAN_AND_MEAN
#define WIN64_LEAN_AND_MEAN
#endif
#endif
#include <windows.h>
#include <shellapi.h>
// Conflicts with ig::CreateDirectory
#ifdef CreateDirectory
#undef CreateDirectory
#endif
#endif

// Linux
#ifdef __linux__
#define Font Font_ // Conflicts with ig::Font
#define Cursor Cursor_
#include <unistd.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#ifdef Always
#undef Always // Conflicts with iglo.h enum elements
#define Always_ 2
#endif
#ifdef None
#undef None // Conflicts with iglo.h enum elements
#define None_ 0L
#endif
#ifdef KeyPress
#undef KeyPress // Conflicts with iglo.h ig::EventType::KeyPress
#define KeyPress_ 2
#endif
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// IGLO Includes ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
#include "stb/stb_truetype.h"
#define STBI_WINDOWS_UTF8
#include "stb/stb_image.h"
#define STBIW_WINDOWS_UTF8
#include "stb/stb_image_write.h"
#include "igloConfig.h"
#include "igloUtility.h"
#include <queue>
#include <mutex>

constexpr uint32_t IGLO_MAX_SIMULTANEOUS_RENDER_TARGETS = 8;
constexpr uint64_t IGLO_PUSH_CONSTANT_TOTAL_BYTE_SIZE = 64 * 4;
constexpr uint32_t IGLO_MAX_BATCHED_BARRIERS_PER_TYPE = 16;

// Use D3D12 by default
#if !defined(IGLO_D3D12) && !defined(IGLO_VULKAN)
#define IGLO_D3D12
#endif


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// D3D12 Includes //////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
#ifdef IGLO_D3D12
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#include <d3d12.h>
#include <dxgi1_6.h> 
#include <wrl.h> // For ComPtr

using Microsoft::WRL::ComPtr;

#endif

//////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// Vulkan Includes ////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
#ifdef IGLO_VULKAN
#include <vulkan/vulkan.h>
#endif

//////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// IGLO Forward declarations ////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

namespace ig
{
	enum class LogType;
	enum class MSAA;
	enum class PresentMode;
	enum class DisplayMode;
	enum class ShaderType;
	enum class TextureWrapMode;
	enum class Primitive;
	enum class BlendOperation;
	enum class BlendData;
	enum class LogicOp;
	enum class ComparisonFunc;
	enum class StencilOp;
	enum class ColorWriteMask;
	enum class DepthWriteMask;
	enum class LineRasterizationMode;
	enum class Cull;
	enum class FrontFace;
	enum class TextureFilter;
	struct BlendDesc;
	struct DepthDesc;
	struct RasterizerDesc;
	struct SamplerDesc;
	struct Descriptor;
	class Sampler;
	enum class Format;
	enum class IndexFormat;
	class Image;
	enum class TextureUsage;
	struct ClearValue;
	class Texture;
	enum class BufferUsage;
	enum class BufferType;
	class Buffer;
	enum class InputClass;
	struct VertexElement;
	struct Shader;
	struct RenderTargetDesc;
	struct PipelineDesc;
	class Pipeline;
	class DescriptorHeap;
	enum class CommandListType;
	struct Receipt;
	class CommandQueue;
	enum class BarrierSync : uint64_t;
	enum class BarrierAccess;
	enum class BarrierLayout;
	enum class SimpleBarrier;
	struct SimpleBarrierInfo;
	class CommandList;
	enum class BufferPlacementAlignment : uint32_t;
	struct TempBuffer;
	class BufferAllocator;
	struct Viewport;
	enum class MouseButton;
	enum class Key;
	enum class EventType;
	class Event;
	struct WindowSettings;
	struct RenderSettings;
	class IGLOContext;

	//////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////// IGLO Declarations //////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////

	// Callbacks
	typedef void(*CallbackLog)(LogType messageType, const std::string& message);
	typedef void(*CallbackModalLoop)(void* userData);
	typedef void(*CallbackOnDeviceRemoved)();

	enum class LogType
	{
		Info = 0, // Example: Prints the name of the graphics API version being used.
		Warning,
		Error, // Example: Failed to load texture, copy texture etc...
		FatalError, // Example: Failed to load IGLOContext, an error occured that forced IGLOContext to unload.
	};

	// Logs a debug message.
	// By default, the message is printed to the console window or debug output window.
	// You can decide how a debug message is handled with SetLogCallback(myFunc).
	void Log(LogType type, const std::string& message);

	// If nullptr is specified then Log() will revert to default behavior.
	void SetLogCallback(CallbackLog logFunc);

	void PopupMessage(const std::string& message, const std::string& caption = "", const IGLOContext* parent = nullptr);

	// Multisampled anti aliasing
	enum class MSAA
	{
		Disabled = 1,
		X2 = 2,
		X4 = 4,
		X8 = 8,
		X16 = 16
	};

	enum class PresentMode
	{
		// Lowest latency of all presentation modes.
		// Tearing will only appear while in fullscreen (atleast in D3D12).
		// This is also known as "Vsync Off".
		ImmediateWithTearing = 0,

		// No tearing, lower latency than Vsync, but can drop frames.
		// While in fullscreen (atleast in D3D12), the more back buffers you use the lower the latency you get, at the cost of higher GPU usage.
		// This is also known as "Mailbox" and "Fast Vsync".
		Immediate,

		// No dropped frames, no tearing, high latency.
		// The more back buffers and frames in flight you use, the higher the latency.
		Vsync,
		VsyncHalf,
	};

	enum class DisplayMode
	{
		Windowed = 0, // Windowed mode.
		BorderlessFullscreen = 1, // Borderless windowed fullscreen.
	};

	enum class ShaderType
	{
		Vertex = 0,
		Pixel,
		Hull,
		Domain,
		Geometry,
		Compute,
	};

	enum class TextureWrapMode
	{
#ifdef IGLO_D3D12
		Repeat = D3D12_TEXTURE_ADDRESS_MODE_WRAP, // Coords outside 0-1 tiles. The texture repeats.
		Mirror = D3D12_TEXTURE_ADDRESS_MODE_MIRROR, // Texture is mirrored.
		MirrorOnce = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE, // Texture is mirrored once, then clamps to border.
		Clamp = D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // Coords outside 0-1 returns outmost border pixel.
		BorderColor = D3D12_TEXTURE_ADDRESS_MODE_BORDER, // Coords outside 0-1 returns given border color.
#endif
#ifdef IGLO_VULKAN
		Repeat = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		Mirror = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		Clamp = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
#endif
	};

	// Primitive topology
	enum class Primitive
	{
#ifdef IGLO_D3D12
		Undefined = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,
		PointList = D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
		LineList = D3D_PRIMITIVE_TOPOLOGY_LINELIST,
		LineStrip = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
		TriangleList = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		TriangleStrip = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,

		// Primitive topologies for geometry shaders
		_LineList_Adj = D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ,
		_LineStrip_Adj = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ,
		_TriangleList_Adj = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ,
		_TriangleStrip_Adj = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ,

		// Primitive topologies for hull/domain shaders
		__1_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST,
		__2_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST,
		__3_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST,
		__4_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST,
		__5_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST,
		__6_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST,
		__7_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST,
		__8_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST,
		__9_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST,
		__10_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST,
		__11_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST,
		__12_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST,
		__13_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST,
		__14_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST,
		__15_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST,
		__16_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST,
		__17_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST,
		__18_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST,
		__19_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST,
		__20_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST,
		__21_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST,
		__22_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST,
		__23_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST,
		__24_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST,
		__25_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST,
		__26_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST,
		__27_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST,
		__28_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST,
		__29_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST,
		__30_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST,
		__31_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST,
		__32_ControlPointPatchList = D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST,
#endif
#ifdef IGLO_VULKAN
		Undefined = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM,
		PointList = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
		LineList = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		LineStrip = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
		TriangleList = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		TriangleStrip = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,

		_LineList_Adj = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
		_LineStrip_Adj = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
		_TriangleList_Adj = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
		_TriangleStrip_Adj = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
#endif
	};

	enum class BlendOperation
	{
#ifdef IGLO_D3D12
		Add = D3D12_BLEND_OP_ADD,
		Subtract = D3D12_BLEND_OP_SUBTRACT,
		ReverseSubtract = D3D12_BLEND_OP_REV_SUBTRACT,
		Min = D3D12_BLEND_OP_MIN,
		Max = D3D12_BLEND_OP_MAX,
#endif
#ifdef IGLO_VULKAN
		Add = VK_BLEND_OP_ADD,
		Subtract = VK_BLEND_OP_SUBTRACT,
		ReverseSubtract = VK_BLEND_OP_REVERSE_SUBTRACT,
		Min = VK_BLEND_OP_MIN,
		Max = VK_BLEND_OP_MAX,
#endif
	};

	enum class BlendData
	{
#ifdef IGLO_D3D12
		Zero = D3D12_BLEND_ZERO,
		One = D3D12_BLEND_ONE,
		SourceColor = D3D12_BLEND_SRC_COLOR,
		SourceAlpha = D3D12_BLEND_SRC_ALPHA,
		Source1Color = D3D12_BLEND_SRC1_COLOR,
		Source1Alpha = D3D12_BLEND_SRC1_ALPHA,
		InverseSourceColor = D3D12_BLEND_INV_SRC_COLOR,
		InverseSourceAlpha = D3D12_BLEND_INV_SRC_ALPHA,
		InverseDestAlpha = D3D12_BLEND_INV_DEST_ALPHA,
		InverseDestColor = D3D12_BLEND_INV_DEST_COLOR,
		InverseSource1Color = D3D12_BLEND_INV_SRC1_COLOR,
		InverseSource1Alpha = D3D12_BLEND_INV_SRC1_ALPHA,
		DestAlpha = D3D12_BLEND_DEST_ALPHA,
		DestColor = D3D12_BLEND_DEST_COLOR,

		SourceAlphaSat = D3D12_BLEND_SRC_ALPHA_SAT,
		BlendFactor = D3D12_BLEND_BLEND_FACTOR,
		InverseBlendFactor = D3D12_BLEND_INV_BLEND_FACTOR,
#endif
#ifdef IGLO_VULKAN
		Zero = VK_BLEND_FACTOR_ZERO,
		One = VK_BLEND_FACTOR_ONE,
		SourceColor = VK_BLEND_FACTOR_SRC_COLOR,
		SourceAlpha = VK_BLEND_FACTOR_SRC_ALPHA,
		Source1Color = VK_BLEND_FACTOR_SRC1_COLOR,
		Source1Alpha = VK_BLEND_FACTOR_SRC1_ALPHA,
		InverseSourceColor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		InverseSourceAlpha = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		InverseDestAlpha = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
		InverseDestColor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
		InverseSource1Color = VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
		InverseSource1Alpha = VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
		DestAlpha = VK_BLEND_FACTOR_DST_ALPHA,
		DestColor = VK_BLEND_FACTOR_DST_COLOR,
#endif
	};

	enum class LogicOp
	{
#ifdef IGLO_D3D12
		Clear = D3D12_LOGIC_OP_CLEAR,
		Set = D3D12_LOGIC_OP_SET,
		Copy = D3D12_LOGIC_OP_COPY,
		CopyInverted = D3D12_LOGIC_OP_COPY_INVERTED,
		NoOperation = D3D12_LOGIC_OP_NOOP,
		Invert = D3D12_LOGIC_OP_INVERT,
		And = D3D12_LOGIC_OP_AND,
		Nand = D3D12_LOGIC_OP_NAND,
		Or = D3D12_LOGIC_OP_OR,
		Nor = D3D12_LOGIC_OP_NOR,
		Xor = D3D12_LOGIC_OP_XOR,
		Equivalent = D3D12_LOGIC_OP_EQUIV,
		AndReverse = D3D12_LOGIC_OP_AND_REVERSE,
		AndInverted = D3D12_LOGIC_OP_AND_INVERTED,
		OrReverse = D3D12_LOGIC_OP_OR_REVERSE,
		OrInverted = D3D12_LOGIC_OP_OR_INVERTED
#endif
#ifdef IGLO_VULKAN
		Clear = VK_LOGIC_OP_CLEAR,
		Set = VK_LOGIC_OP_SET,
		Copy = VK_LOGIC_OP_COPY,
		CopyInverted = VK_LOGIC_OP_COPY_INVERTED,
		NoOperation = VK_LOGIC_OP_NO_OP,
		Invert = VK_LOGIC_OP_INVERT,
		And = VK_LOGIC_OP_AND,
		Nand = VK_LOGIC_OP_NAND,
		Or = VK_LOGIC_OP_OR,
		Nor = VK_LOGIC_OP_NOR,
		Xor = VK_LOGIC_OP_XOR,
		Equivalent = VK_LOGIC_OP_EQUIVALENT,
		AndReverse = VK_LOGIC_OP_AND_REVERSE,
		AndInverted = VK_LOGIC_OP_AND_INVERTED,
		OrReverse = VK_LOGIC_OP_OR_REVERSE,
		OrInverted = VK_LOGIC_OP_OR_INVERTED
#endif
	};

	enum class ComparisonFunc
	{
#ifdef IGLO_D3D12
		None = D3D12_COMPARISON_FUNC_NONE,
		Never = D3D12_COMPARISON_FUNC_NEVER,
		Less = D3D12_COMPARISON_FUNC_LESS,
		Equal = D3D12_COMPARISON_FUNC_EQUAL,
		LessOrEqual = D3D12_COMPARISON_FUNC_LESS_EQUAL,
		Greater = D3D12_COMPARISON_FUNC_GREATER,
		NotEqual = D3D12_COMPARISON_FUNC_NOT_EQUAL,
		GreaterOrEqual = D3D12_COMPARISON_FUNC_GREATER_EQUAL,
		Always = D3D12_COMPARISON_FUNC_ALWAYS,
#endif
#ifdef IGLO_VULKAN
		None = VK_COMPARE_OP_NEVER,
		Never = VK_COMPARE_OP_NEVER,
		Less = VK_COMPARE_OP_LESS,
		Equal = VK_COMPARE_OP_EQUAL,
		LessOrEqual = VK_COMPARE_OP_LESS_OR_EQUAL,
		Greater = VK_COMPARE_OP_GREATER,
		NotEqual = VK_COMPARE_OP_NOT_EQUAL,
		GreaterOrEqual = VK_COMPARE_OP_GREATER_OR_EQUAL,
		Always = VK_COMPARE_OP_ALWAYS,
#endif
	};

	enum class StencilOp
	{
#ifdef IGLO_D3D12
		Keep = D3D12_STENCIL_OP_KEEP,
		Zero = D3D12_STENCIL_OP_ZERO,
		Replace = D3D12_STENCIL_OP_REPLACE,
		IncreaseSat = D3D12_STENCIL_OP_INCR_SAT,
		DecreaseSat = D3D12_STENCIL_OP_DECR_SAT,
		Invert = D3D12_STENCIL_OP_INVERT,
		Increase = D3D12_STENCIL_OP_INCR,
		Decrease = D3D12_STENCIL_OP_DECR,
#endif
#ifdef IGLO_VULKAN
		Keep = VK_STENCIL_OP_KEEP,
		Zero = VK_STENCIL_OP_ZERO,
		Replace = VK_STENCIL_OP_REPLACE,
		IncreaseSat = VK_STENCIL_OP_INCREMENT_AND_CLAMP,
		DecreaseSat = VK_STENCIL_OP_DECREMENT_AND_CLAMP,
		Invert = VK_STENCIL_OP_INVERT,
		Increase = VK_STENCIL_OP_INCREMENT_AND_WRAP,
		Decrease = VK_STENCIL_OP_DECREMENT_AND_WRAP,
#endif
	};

	enum class ColorWriteMask
	{
#ifdef IGLO_D3D12
		None = 0,
		Red = D3D12_COLOR_WRITE_ENABLE_RED,
		Green = D3D12_COLOR_WRITE_ENABLE_GREEN,
		Blue = D3D12_COLOR_WRITE_ENABLE_BLUE,
		Alpha = D3D12_COLOR_WRITE_ENABLE_ALPHA,
		All = D3D12_COLOR_WRITE_ENABLE_ALL
#endif
#ifdef IGLO_VULKAN
		None = 0,
		Red = VK_COLOR_COMPONENT_R_BIT,
		Green = VK_COLOR_COMPONENT_G_BIT,
		Blue = VK_COLOR_COMPONENT_B_BIT,
		Alpha = VK_COLOR_COMPONENT_A_BIT,
		All = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
#endif
	};

	enum class DepthWriteMask
	{
#ifdef IGLO_D3D12
		Zero = D3D12_DEPTH_WRITE_MASK_ZERO,
		All = D3D12_DEPTH_WRITE_MASK_ALL
#endif
#ifdef IGLO_VULKAN
		All
		//TODO: does vulkan have a depth write mask equivalent?
#endif
	};

	enum class LineRasterizationMode
	{
		Pixelated = 0,
		Smooth = 1
	};

	enum class Cull
	{
		Disabled = 0,
		Front = 1,
		Back = 2, // The back faces (that face away from you) are by default culled (not drawn)
	};

	enum class FrontFace
	{
		CW = 0, // Clockwise is the default front face.
		CCW = 1,
	};

	enum class TextureFilter
	{
		Point = 0, // Point sampling makes textures look pixelated when stretched
		Bilinear,
		Trilinear,
		AnisotropicX2,
		AnisotropicX4,
		AnisotropicX8,
		AnisotropicX16, // Anisotropic filtering makes textures look smooth when stretched

		_Comparison_Point,
		_Comparison_Bilinear,
		_Comparison_Trilinear,
		_Comparison_AnisotropicX2,
		_Comparison_AnisotropicX4,
		_Comparison_AnisotropicX8,
		_Comparison_AnisotropicX16,

		_Minimum_Point,
		_Minimum_Bilinear,
		_Minimum_Trilinear,
		_Minimum_AnisotropicX2,
		_Minimum_AnisotropicX4,
		_Minimum_AnisotropicX8,
		_Minimum_AnisotropicX16,

		_Maximum_Point,
		_Maximum_Bilinear,
		_Maximum_Trilinear,
		_Maximum_AnisotropicX2,
		_Maximum_AnisotropicX4,
		_Maximum_AnisotropicX8,
		_Maximum_AnisotropicX16,
	};

#ifdef IGLO_D3D12
	struct TextureFilterD3D12
	{
		D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		unsigned int maxAnisotropy = 0;
	};
	static TextureFilterD3D12 ConvertTextureFilterToD3D12(TextureFilter filter);
#endif
#ifdef IGLO_VULKAN
	//TODO: Function to convert TextureFilter to Vulkan equivalent.
#endif

	struct BlendDesc
	{
		bool enabled = false;
		bool logicOpEnabled = false;
		BlendData srcBlend = BlendData::One;
		BlendData destBlend = BlendData::Zero;
		BlendOperation blendOp = BlendOperation::Add;
		BlendData srcBlendAlpha = BlendData::One;
		BlendData destBlendAlpha = BlendData::Zero;
		BlendOperation blendOpAlpha = BlendOperation::Add;
		LogicOp logicOp = LogicOp::NoOperation;
		byte colorWriteMask = (byte)ColorWriteMask::All;

		// Alpha blending is disabled
		static const BlendDesc BlendDisabled;

		// This type of blending is quite common.
		static const BlendDesc StraightAlpha;

		// Premultiplied alpha blending is needed when you want to draw a transparent render texture
		// and don't want the alpha values to be multiplied twice, and thus look more transparent than it should.
		static const BlendDesc PremultipliedAlpha;
	};

	struct DepthDesc
	{
		bool enableDepth = true;
		DepthWriteMask depthWriteMask = DepthWriteMask::All;
		ComparisonFunc depthFunc = ComparisonFunc::Less;
		bool enableStencil = false;
		byte stencilReadMask = 0xff;
		byte stencilWriteMask = 0xff;
		StencilOp	   frontFaceStencilFailOp = StencilOp::Keep;
		StencilOp	   frontFaceStencilDepthFailOp = StencilOp::Keep;
		StencilOp	   frontFaceStencilPassOp = StencilOp::Keep;
		ComparisonFunc frontFaceStencilFunc = ComparisonFunc::Always;
		StencilOp	   backFaceStencilFailOp = StencilOp::Keep;
		StencilOp      backFaceStencilDepthFailOp = StencilOp::Keep;
		StencilOp      backFaceStencilPassOp = StencilOp::Keep;
		ComparisonFunc backFaceStencilFunc = ComparisonFunc::Always;

		// Depth testing is disabled.
		static const DepthDesc DepthDisabled;

		// Only depth testing is enabled.
		static const DepthDesc DepthEnabled;

		// Both depth and stencil testing are enabled.
		static const DepthDesc DepthAndStencilEnabled;
	};

	struct RasterizerDesc
	{
		bool enableWireframe = false; // If false, fill mode is solid
		Cull cull = Cull::Back;
		FrontFace frontFace = FrontFace::CW;
		int depthBias = 0;
		float depthBiasClamp = 0.0f;
		float slopeScaledDepthBias = 0.0f;
		bool enableDepthClip = true;
		LineRasterizationMode lineRasterizationMode = LineRasterizationMode::Smooth;
		unsigned int forcedSampleCount = 0;
		bool enableConservativeRaster = false;

		// No culling takes place.
		static const RasterizerDesc NoCull;

		// Back faces are culled. This is the default rasterizer for most 3D rendering.
		static const RasterizerDesc BackCull;

		// Front faces are culled. Useful for debugging.
		static const RasterizerDesc FrontCull;
	};

	struct SamplerDesc
	{
		TextureFilter filter = TextureFilter::Trilinear;
		TextureWrapMode wrapU = TextureWrapMode::Clamp;
		TextureWrapMode wrapV = TextureWrapMode::Clamp;
		TextureWrapMode wrapW = TextureWrapMode::Clamp;
		float minLOD = -IGLO_FLOAT32_MAX;
		float maxLOD = IGLO_FLOAT32_MAX;
		float mipMapLODBias = 0;
		ComparisonFunc comparisonFunc = ComparisonFunc::None;
		Color borderColor = Colors::White;

		// This sampler uses point filtering to make stretched textures look pixelated, and textures repeat.
		static const SamplerDesc PixelatedRepeatSampler;

		// This sampler uses x16 anisotropic filtering to make stretched textures look smooth, and textures repeat.
		static const SamplerDesc SmoothRepeatSampler;

		// This sampler uses x16 anisotropic filtering to make stretched textures look smooth, and textures are clamped.
		static const SamplerDesc SmoothClampSampler;
	};

	struct Descriptor
	{
		Descriptor(uint32_t heapIndex = IGLO_UINT32_MAX)
		{
			this->heapIndex = heapIndex;
		}

		bool IsNull() const { return (heapIndex == IGLO_UINT32_MAX); }
		void SetToNull() { heapIndex = IGLO_UINT32_MAX; }

		uint32_t heapIndex;
	};

	class Sampler
	{
	public:
		Sampler() = default;
		~Sampler() { Unload(); }

		Sampler& operator=(Sampler&) = delete;
		Sampler(Sampler&) = delete;

		void Unload();
		bool IsLoaded() const { return isLoaded; }

		// Creates a sampler state. Returns true if success.
		bool Load(const IGLOContext&, const SamplerDesc& desc);

		const Descriptor* GetDescriptor() const;

	private:
		bool isLoaded = false;
		const IGLOContext* context = nullptr;
		Descriptor descriptor;
	};

	enum class Format
	{
		None = 0,
		BYTE, // 8-bit unsigned integer. Grayscale. Values go from 0 to 255. Uses red color in pixel shader.
		BYTE_BYTE, // Two colors channels: Red and green.
		BYTE_BYTE_BYTE_BYTE, // RGBA. Four 8-bit channels. 32 bits per pixel. Values go from 0 to 255.
		INT8, // 8-bit signed integer. Grayscale. Values go from -128 to 127. Uses red color in pixel shader.
		INT8_INT8,
		INT8_INT8_INT8_INT8, // RGBA. Four 8-bit channels. 32 bits per pixel. Values go from -128 to 127.
		UINT16,// Grayscale. Uses red color in pixel shader.
		UINT16_UINT16,
		UINT16_UINT16_UINT16_UINT16, // RGBA. 64 bits per pixel. Values go from 0 to 65535.
		INT16,// Grayscale. Uses red color in pixel shader.
		INT16_INT16,
		INT16_INT16_INT16_INT16,
		FLOAT16, //16-bit float format. Grayscale. Uses red color in pixel shader.
		FLOAT16_FLOAT16,
		FLOAT16_FLOAT16_FLOAT16_FLOAT16,
		FLOAT, // 32-bit float format. Grayscale. Uses red color in pixel shader.
		FLOAT_FLOAT,
		FLOAT_FLOAT_FLOAT,
		FLOAT_FLOAT_FLOAT_FLOAT, // RGBA. Each color is a float. 128 bits per pixel.

		// ARGB. HDR format. 10 bit color. 2 bit alpha. 32 bits per pixel. RGBA in D3D12.
		UINT2_UINT10_UINT10_UINT10,
		UINT2_UINT10_UINT10_UINT10_NotNormalized,

		// BGR. HDR format. 32 bits per pixel. RGB in D3D12.
		UFLOAT10_UFLOAT11_UFLOAT11,

		// EBGR. HDR format. 5 bit shared exponent. 32 bits per pixel. RGBE in D3D12.
		UFLOAT5_UFLOAT9_UFLOAT9_UFLOAT9,

		// Uses the standard RGB color space.
		BYTE_BYTE_BYTE_BYTE_sRGB,

		// Color byte order is BGRA instead of RGBA. 
		// This format allows you to write to the pixels like this: uint32_t pixel = 0xAARRGGBB;
		BYTE_BYTE_BYTE_BYTE_BGRA,
		BYTE_BYTE_BYTE_BYTE_BGRA_sRGB,

		// Non normalized. Shader sees color values ranging from 0 to 255 instead of 0.0 to 1.0.
		BYTE_BYTE_BYTE_BYTE_NotNormalized,
		BYTE_BYTE_NotNormalized,
		BYTE_NotNormalized,

		// Non normalized. Shader sees color values ranging from -128 to 127 instead of -1.0 to 1.0.
		INT8_NotNormalized,
		INT8_INT8_NotNormalized,
		INT8_INT8_INT8_INT8_NotNormalized,

		// Non normalized. Shader sees color values ranging from 0 to 65535 instead of 0.0 to 1.0.
		UINT16_NotNormalized,
		UINT16_UINT16_NotNormalized,
		UINT16_UINT16_UINT16_UINT16_NotNormalized,

		// Non normalized. Shader sees color values ranging from -32768 to 32767 instead of -1.0 to 1.0.
		INT16_NotNormalized,
		INT16_INT16_NotNormalized,
		INT16_INT16_INT16_INT16_NotNormalized,

		// Non normalized. Shader sees color values ranging from 0 to 4294967295 instead of 0.0 to 1.0.
		UINT32_NotNormalized,
		UINT32_UINT32_NotNormalized,
		UINT32_UINT32_UINT32_NotNormalized,
		UINT32_UINT32_UINT32_UINT32_NotNormalized,

		// Non normalized. Shader sees color values from -2147483648 to 2147483647 instead of -1.0 to 1.0.
		INT32_NotNormalized,
		INT32_INT32_NotNormalized,
		INT32_INT32_INT32_NotNormalized,
		INT32_INT32_INT32_INT32_NotNormalized,

		//-------------- Block compression formats that consist of 4x4 pixel blocks --------------//

		// RGBA. 1 bit alpha. Good for cutout grass and for textures without an alpha channel. Unsigned and normalized.
		BC1,
		BC1_sRGB,

		// RGBA. Low quality alpha. Not used much. BC3 is better.
		BC2,
		BC2_sRGB,

		// RGBA. High quality alpha. Good for regular textures.
		BC3,
		BC3_sRGB,

		// Grayscale.
		BC4_UNSIGNED,
		BC4_SIGNED,

		// Two color channels: Red and Green. Good for normalmaps.
		BC5_UNSIGNED,
		BC5_SIGNED,

		// RGBA. 16-bit float. Can store HDR.
		BC6H_UNSIGNED_FLOAT16,
		BC6H_SIGNED_FLOAT16,

		// RGBA. High quality color with full alpha.
		BC7,
		BC7_sRGB,

		//-------------- Depth buffer formats --------------//
		DEPTHFORMAT_UINT16, // 16-bit unsigned normalized integer for depth component
		DEPTHFORMAT_UINT24_BYTE, // 24-bit unsigned normalized integer for depth component, 8-bit unsigned non normalized integer for stencil component.
		DEPTHFORMAT_FLOAT,  // 32-bit float for depth component
		DEPTHFORMAT_FLOAT_BYTE, // 32-bit float for depth component, 8-bit unsigned non normalized integer for stencil component.
	};

	enum class IndexFormat
	{
		UINT16 = (int)Format::UINT16_NotNormalized,
		UINT32 = (int)Format::UINT32_NotNormalized,
	};

	struct FormatInfo
	{
		// Amount of color channels for textures, or amount of elements in a vertex.
		uint32_t elementCount = 0;

		// Number of bytes each pixel (or vertex) takes up. Will be 0 for all block compression formats.
		uint32_t bytesPerPixel = 0;

		// Will be 0 if format does not use block compression.
		// To calculate the row pitch for block-compressed formats, do this: max( 1, ((width+3)/4) ) * blockSize
		uint32_t blockSize = 0;

		// True if this format is a depth format.
		bool isDepthFormat = false;

		// The sRGB opposite of given format if one exists.
		// Example: BYTE_BYTE_BYTE_BYTE_sRGB has the sRGB opposite BYTE_BYTE_BYTE_BYTE and vice versa.
		// Will be Format::None if a sRGB opposite does not exist for given format.
		Format sRGB_opposite = Format::None;

		// If true, this format uses sRGB color space.
		bool is_sRGB = false;
	};
	FormatInfo GetFormatInfo(Format format);

#ifdef IGLO_D3D12
	struct FormatInfoDXGI
	{
		DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
		DXGI_FORMAT typeless = DXGI_FORMAT_UNKNOWN;
	};
	// Returns DXGI_FORMAT_UNKNOWN if format is not supported in D3D12 or if format is unknown.
	FormatInfoDXGI GetFormatInfoDXGI(Format format);
#endif

	class Image
	{
	public:
		Image() = default;
		~Image() { Unload(); }

		Image& operator=(Image&) = delete;
		Image(Image&) = delete;

		Image& operator=(Image&&) noexcept; // Move assignment operator
		Image(Image&&) noexcept; // Move constructor

		// How the mipmaps and faces are arranged in memory
		enum class Arrangement
		{
			// DDS files use this arrangement.
			// foreach face {foreach mipmap{}}
			CompleteMipChains = 0,

			// KTX files use this arrangement.
			// foreach mipmap {foreach face{}}
			MipStrips = 1,
		};

		// Calculates the total byte size of an image, including all miplevels and faces.
		static size_t CalculateTotalSize(uint32_t mipLevels, uint32_t numFaces, uint32_t width, uint32_t height, Format format);
		static size_t CalculateMipSize(uint32_t mipIndex, uint32_t width, uint32_t height, Format format);
		static uint32_t CalculateMipRowPitch(uint32_t mipIndex, uint32_t width, uint32_t height, Format format);
		static Extent2D CalculateMipExtent(uint32_t mipIndex, uint32_t width, uint32_t height);
		static uint32_t CalculateNumMips(uint32_t width, uint32_t height);

		void Unload();
		bool IsLoaded() const { return pixelsPtr != nullptr; }

		// Creates an image and fills all pixels with an initial value of 0.
		bool Load(uint32_t width, uint32_t height, Format format,
			uint32_t mipLevels = 1, uint32_t numFaces = 1, bool isCubemap = false, Arrangement arrangement = Arrangement::CompleteMipChains);

		// Loads an image from a file.
		bool LoadFromFile(const std::string& filename);

		// Loads an image from a file in memory.
		// 'forceCopy' determines whether image must copy the data to its own buffer
		// or continue to depend on the provided memory in 'fileData' even after being loaded.
		// If 'forceCopy' is false, it may improve performance by avoiding copying data (mostly for DDS and KTX files),
		// but then you must make sure the memory in 'fileData' remains valid throughout the lifetime of this image.
		bool LoadFromMemory(const byte* fileData, size_t numBytes, bool forceCopy = true);

		// Associates raw pixel data in memory to this image, with no copying taking place.
		// Image will not contain any data itself, it will just use the provided pointer for reading/writing pixel data.
		// Helpful for when you need to interact with raw pixel data in memory.
		// The memory in 'pixels' must remain valid throughout the lifetime of this image.
		// Image will not free the memory in 'pixels' when unloaded.
		bool LoadAsPointer(const void* pixels, uint32_t width, uint32_t height, Format format,
			uint32_t mipLevels = 1, uint32_t numFaces = 1, bool isCubemap = false, Arrangement arrangement = Arrangement::CompleteMipChains);

		// Saves image to file. Supported file extensions: PNG, BMP, TGA, JPG/JPEG, HDR.
		bool SaveToFile(const std::string& filename) const;

		// The lowest number of mip levels an image can have is 1.
		uint32_t GetNumMipLevels() const { return mipLevels; }
		bool HasMipmaps() const { return mipLevels > 1; }
		uint32_t GetNumFaces() const { return numFaces; }

		// Width of the first mip level
		uint32_t GetWidth() const { return width; }
		// Height of the first mip level
		uint32_t GetHeight() const { return height; }

		Format GetFormat() const { return format; }

		// Sets the format to an sRGB or non-sRGB equivalent to current format.
		// Not all formats support being sRGB. If the requested sRGB or non-sRGB format does not exist, no changes will be made.
		void SetSRGB(bool sRGB);

		// Returns true if the format of this image is of type sRGB.
		bool IsSRGB() const;

		bool IsCubemap() const { return isCubemap; }

		// Replaces all pixel values that are colorA with colorB (applies to all miplevels and faces).
		// For this method to work, the format must be 32 bits per pixel unsigned integer with 4 color channels.
		// This is useful for adding transparency to sprite textures.
		void ReplaceColors(Color32 colorA, Color32 colorB);

		// Gets the byte size of this image (including all faces and mipLevels).
		size_t GetSize() const { return size; }

		// Gets the byte size of a mipmap slice. 'mipIndex' is zero based.
		size_t GetMipSize(uint32_t mipIndex) const;
		uint32_t GetMipRowPitch(uint32_t mipIndex) const;
		Extent2D GetMipExtent(uint32_t mipIndex) const;

		void* GetPixels() const { return (void*)pixelsPtr; }

		// Gets the pixel data starting at specified mip index and face index.
		// faceIndex=0, mipIndex=0 will get the pixels located at the first mip at the first face.
		void* GetMipPixels(uint32_t faceIndex, uint32_t mipIndex) const;

		// Gets which order the mipmaps and faces are arranged.
		Arrangement GetArrangement() const { return arrangement; }

	private:
		static bool LogErrorIfNotValid(uint32_t width, uint32_t height, Format format,
			uint32_t mipLevels, uint32_t numFaces, bool isCubemap);
		bool FileIsOfTypeDDS(const byte* fileData, size_t numBytes);
		bool FileIsOfTypeKTX(const byte* fileData, size_t numBytes);
		bool LoadFromDDS(const byte* fileData, size_t numBytes, bool forceCopy);
		bool LoadFromKTX(const byte* fileData, size_t numBytes, bool forceCopy);

		// A byte array containing the source of the pixel data (may be raw pixel data only or an entire DDS/KTX file).
		// Only used if this image owns the pixel data itself.
		std::vector<byte> ownedBuffer;
		// A pointer to where the pixel data begins.
		byte* pixelsPtr = nullptr;
		// If true, 'pixelsPtr' points to memory allocated by stb_image which must be freed using stb_image at some point.
		bool mustFreeSTBI = false;

		size_t size = 0; // Byte size of the entire image (including all faces and mipLevels).
		uint32_t width = 0; // Width of the first mip level.
		uint32_t height = 0; // Height of the first mip level.
		uint32_t mipLevels = 0;
		uint32_t numFaces = 0; // Most textures has 1 face. A cubemap has 6 faces.
		Format format = Format::None;
		Arrangement arrangement = Arrangement::CompleteMipChains;
		bool isCubemap = false;

	};

	enum class TextureUsage
	{
		Default = 0,

		// Enables CPU read access.
		// You can copy to a readable texture max once per frame, unless you wait for commands to finish executing in between.
		// The contents of a readable texture is not persistent and becomes stale the next frame.
		Readable,

		// Unordered access is commonly used in compute shaders for read-write operations.
		// A texture with this usage will have both an SRV (for read-only) and a UAV (for read-write or write-only).
		// Use the SRV descriptor for read operations, and the UAV for read-write or write only operations in shaders.
		// Remember to use texture barriers to transition between unordered access and shader resource states as needed.
		UnorderedAccess,

		// RenderTexture and DepthBuffer usage allows the texture to be used as a render target.
		RenderTexture,
		DepthBuffer,
	};

	struct ClearValue
	{
		// For render texture
		Color color = Colors::Black;

		// For depth buffer
		float depth = 1.0f;
		byte stencil = 255;
	};

	class Texture
	{
	public:
		Texture() = default;
		~Texture() { Unload(); }

		Texture& operator=(Texture&) = delete;
		Texture(Texture&) = delete;

		Texture& operator=(Texture&&) noexcept; // Move assignment operator
		Texture(Texture&&) noexcept; // Move constructor

		// Destroys the texture.
		// It's OK to call this on already unloaded textures (in that case nothing happens).
		void Unload();
		// If true, this texture has been created and can be used for things.
		bool IsLoaded() const { return isLoaded; }

		// Creates a texture. Replaces existing texture if already loaded.
		// 'optimizedClearValue' is only relevant for render texture and depth buffer usage.
		// 'optionalInitLayout' lets you specify the initial barrier layout. If not specified, layout is chosen based on the texture usage.
		// By default, descriptor(s) will be created for this texture which you can fetch with GetDescriptor() etc.
		// If you know you won't be needing these descriptors, you can skip the creation of them by setting 'createDescriptors' to false.
		bool Load(const IGLOContext&, uint32_t width, uint32_t height, Format format, TextureUsage usage,
			MSAA msaa = MSAA::Disabled, uint32_t numFaces = 1, uint32_t numMipLevels = 1, bool isCubemap = false,
			ClearValue optimizedClearValue = ClearValue(), BarrierLayout* optionalInitLayout = nullptr, bool createDescriptors = true);

		// Loads a texture from file. Returns true if success.
		// If 'generateMipMaps' is true then mipmaps will be generated if the image doesn't already contain mipmaps.
		bool LoadFromFile(const IGLOContext&, CommandList&, const std::string& filename, bool generateMipmaps = true, bool sRGB = false);

		// Loads a texture from a file in memory (DDS,PNG,JPG,GIF etc...). Returns true if success.
		// If 'generateMipMaps' is true then mipmaps will be generated if the image doesn't already contain mipmaps.
		bool LoadFromMemory(const IGLOContext&, CommandList&, const byte* fileData, size_t numBytes, bool generateMipmaps = true, bool sRGB = false);

		// Loads a texture from image data in memory. Returns true if success.
		bool LoadFromMemory(const IGLOContext&, CommandList&, const Image& image, bool generateMipmaps = true);

		// Wraps resources and descriptors that have been created outside this class.
		// Note that Readable texture usage requires multiple d3d12 resources, one for each frame in flight.
		// For all other texture usages, only one d3d12 resource is requried.
		// Specifying descriptors is optional.
		// NOTE: This texture will free the given descriptors upon unloading, so you shouldn't free the given descriptors yourself.
		bool LoadAsWrapped(const IGLOContext&,
#ifdef IGLO_D3D12
			std::vector<ComPtr<ID3D12Resource>> resources,
#endif
			uint32_t width, uint32_t height, Format format, TextureUsage usage,
			MSAA msaa, bool isCubemap, uint32_t numFaces, uint32_t numMipLevels,
			Descriptor srvDescriptor = Descriptor(), Descriptor srvStencilDescriptor = Descriptor(),
			Descriptor uavDescriptor = Descriptor(),
			std::vector<void*> mappedPtrForReadableUsage = {});

		// Updates the contents of this texture.
		// Usage must not be Readable.
		void SetPixels(CommandList& cmd, const Image& srcImage);
		void SetPixels(CommandList& cmd, const void* pixelData);
		void SetPixelsAtSubresource(CommandList& cmd, const Image& srcImage, uint32_t destFaceIndex, uint32_t destMipIndex);
		void SetPixelsAtSubresource(CommandList& cmd, const void* pixelData, uint32_t destFaceIndex, uint32_t destMipIndex);

		// Reads the contents of this texture and writes it to the given image.
		// The given image must be loaded and must match the format and dimensions of this texture.
		// Usage must be Readable.
		void ReadPixels(Image& destImage);
		Image ReadPixels();

		uint32_t GetWidth() const { return width; }
		uint32_t GetHeight() const { return height; }
		TextureUsage GetUsage() const { return usage; }
		Format GetFormat() const { return format; }
		uint32_t GetColorChannelCount() const;
		MSAA GetMSAA() const { return msaa; }
		bool IsCubemap() const { return isCubemap; }
		uint32_t GetNumFaces() const { return numFaces; }
		uint32_t GetNumMipLevels() const { return numMipLevels; }

		// Gets the SRV descriptor for this texture, if it has one.
		// NOTE: Will return nullptr if texture has no SRV descriptor. This is to protect you
		// from accidentally passing an invalid heap index to the shader and crashing the GPU.
		const Descriptor* GetDescriptor() const;

		// Gets the SRV descriptor for the stencil component, if it has one.
		// In D3D12, the stencil component of a depth format needs its own descriptor to be accessable to the shader.
		const Descriptor* GetStencilDescriptor() const;

		// Gets the UAV descriptor for this texture, if it has one.
		const Descriptor* GetUnorderedAccessDescriptor() const;

#ifdef IGLO_D3D12
		ID3D12Resource* GetD3D12Resource() const;
		D3D12_DEPTH_STENCIL_VIEW_DESC GenerateD3D12DSVDesc() const;
		D3D12_UNORDERED_ACCESS_VIEW_DESC GenerateD3D12UAVDesc() const;
#endif

	private:
		bool isLoaded = false;
		const IGLOContext* context = nullptr;
		TextureUsage usage = TextureUsage::Default;
		uint32_t width = 0;
		uint32_t height = 0;
		Format format = Format::None;
		MSAA msaa = MSAA::Disabled;
		bool isCubemap = false;
		uint32_t numFaces = 0;
		uint32_t numMipLevels = 0;
		Descriptor descriptor; // SRV
		Descriptor stencilDescriptor; // SRV for stencil component
		Descriptor unorderedAccessDescriptor; // UAV
		std::vector<void*> readMapped; // One for each frame in flight

#ifdef IGLO_D3D12
		std::vector<ComPtr<ID3D12Resource>> d3d12Resource;
#endif

	};

	enum class BufferUsage
	{
		// Suitable for buffers that don't update their contents frequently.
		// Default usage buffers can update their contents unlimited times per frame,
		// although Dynamic and Temporary usage may be more suitable for such cases.
		Default = 0,

		// Suitable for buffers that update their contents once per frame.
		// Dynamic buffers can update their contents max once per frame, unless you wait for commands to finish executing in between.
		// The contents of a dynamic buffer is persistent and will last between frames when not updated with new data.
		Dynamic,

		// Enables CPU read access.
		// You can copy to a readable buffer max once per frame, unless you wait for commands to finish executing in between.
		// The contents of a readable buffer is not persistent and becomes stale the next frame.
		Readable,

		// 'Temporary' is another buffer usage type that exists, but it's not defined in this enum
		// because the 'Buffer' class doesn't handle temporary buffers by itself.
		// Temporary buffers are created by IGLOContext and get stale after 1 frame.

		// Unordered access is commonly used in compute shaders for read-write operations.
		// A buffer with this usage will have both an SRV (for read-only) and a UAV (for read-write or write-only).
		// Use the SRV descriptor for read operations, and the UAV for read-write or write only operations in shaders.
		// Remember to use buffer barriers to transition between unordered access and shader resource states as needed.
		UnorderedAccess,
	};

	enum class BufferType
	{
		VertexBuffer = 0,
		IndexBuffer,
		StructuredBuffer,
		RawBuffer,
		ShaderConstant,
	};

	class Buffer
	{
	public:
		Buffer() = default;
		~Buffer() { Unload(); }

		Buffer& operator=(Buffer&) = delete;
		Buffer(Buffer&) = delete;

		Buffer& operator=(Buffer&&) noexcept; // Move assignment operator
		Buffer(Buffer&&) noexcept; // Move constructor

		void Unload();
		bool IsLoaded() const { return isLoaded; }

		bool LoadAsVertexBuffer(const IGLOContext&, uint32_t vertexStride, uint32_t numVertices, BufferUsage usage);
		bool LoadAsIndexBuffer(const IGLOContext&, IndexFormat format, uint32_t numIndices, BufferUsage usage);
		bool LoadAsStructuredBuffer(const IGLOContext&, uint32_t elementStride, uint32_t numElements, BufferUsage usage);
		bool LoadAsRawBuffer(const IGLOContext&, uint64_t numBytes, BufferUsage usage);
		bool LoadAsShaderConstant(const IGLOContext&, uint64_t numBytes, BufferUsage usage);

		// Updates the contents of this buffer.
		// Default or UnorderedAccess buffer usage is required.
		// The number of bytes that is read from 'srcData' is equal to the size of this buffer.
		void SetData(CommandList&, void* srcData);

		// Updates the contents of this buffer.
		// Dynamic buffer usage is required.
		// The number of bytes that is read from 'srcData' is equal to the size of this buffer.
		void SetDynamicData(void* srcData);

		// Reads the contents of this buffer and writes it to the given pointer.
		// Readable buffer usage is required.
		// The number of bytes that is written to 'destData' is equal to the size of this buffer.
		void ReadData(void* destData);

		// Total byte size of this buffer. No alignment, just the actual data.
		uint64_t GetSize() const { return size; }

		// Stride in bytes.
		uint32_t GetStride() const { return stride; }

		BufferType GetType() const { return type; }
		BufferUsage GetUsage() const { return usage; }
		uint32_t GetNumElements() const { return numElements; }

		// Gets the SRV or CBV descriptor for this buffer, if it has one (determined by buffer usage).
		// Will return nullptr if buffer doesn't have an SRV or CBV descriptor.
		const Descriptor* GetDescriptor() const;

		// Gets the UAV descriptor for this buffer, if it has one.
		const Descriptor* GetUnorderedAccessDescriptor() const;

#ifdef IGLO_D3D12
		ID3D12Resource* GetD3D12Resource() const;
		D3D12_UNORDERED_ACCESS_VIEW_DESC GenerateD3D12UAVDesc() const;
#endif

	private:
		bool InternalLoad(const IGLOContext&, uint64_t size, uint32_t stride, uint32_t numElements, BufferUsage, BufferType);

		bool isLoaded = false;
		const IGLOContext* context = nullptr;
		BufferType type = BufferType::VertexBuffer;
		BufferUsage usage = BufferUsage::Default;
		uint64_t size = 0; // total size in bytes
		uint32_t stride = 0; // stride in bytes
		uint32_t numElements = 0;
		std::vector<Descriptor> descriptor; // SRV or CBV. Is a vector because Dynamic usage requires multiple descriptors.
		Descriptor unorderedAccessDescriptor;
		std::vector<void*> mapped;
		uint32_t dynamicSetCounter = 0; // Increases by one when setting dynamic data. Capped at number of frames in flight.

#ifdef IGLO_D3D12
		std::vector<ComPtr<ID3D12Resource>> d3d12Resource;
#endif
	};

	enum class InputClass
	{
		PerVertex = 0,
		PerInstance
	};

	struct VertexElement
	{
		VertexElement()
		{
			format = Format::FLOAT;
			semanticName = "";
			semanticIndex = 0;
			inputSlot = 0;
			inputSlotClass = InputClass::PerVertex;
			instanceDataStepRate = 0;
		}

		VertexElement(Format format, const char* semanticName, uint32_t semanticIndex = 0, uint32_t inputSlot = 0,
			InputClass inputSlotClass = InputClass::PerVertex, uint32_t instanceDataStepRate = 0)
		{
			this->format = format;
			this->semanticName = semanticName;
			this->semanticIndex = semanticIndex;
			this->inputSlot = inputSlot;
			this->inputSlotClass = inputSlotClass;
			this->instanceDataStepRate = instanceDataStepRate;
		}

		Format format;
		const char* semanticName;
		uint32_t semanticIndex;
		uint32_t inputSlot;
		InputClass inputSlotClass;
		uint32_t instanceDataStepRate;
	};

#ifdef IGLO_D3D12
	static std::vector<D3D12_INPUT_ELEMENT_DESC> ConvertVertexElementsToD3D12(const std::vector<VertexElement>& elems);
#endif

	struct Shader
	{
		Shader()
		{
			shaderBytecode = nullptr;
			bytecodeLength = 0;
			entryPointName = "";
		}

		// Specifying 'entryPointName' is mandatory in Vulkan, but in D3D12 it's not needed.
		Shader(const byte* shaderBytecode, size_t bytecodeLength, const std::string& entryPointName = "main")
		{
			this->shaderBytecode = shaderBytecode;
			this->bytecodeLength = bytecodeLength;
			this->entryPointName = entryPointName;
		}

		Shader(const std::vector<byte>& shaderBytecode, const std::string& entryPointName = "main")
		{
			this->shaderBytecode = shaderBytecode.data();
			this->bytecodeLength = shaderBytecode.size();
			this->entryPointName = entryPointName;
		}

		const byte* shaderBytecode;
		size_t bytecodeLength;
		std::string entryPointName;
	};

	struct RenderTargetDesc
	{
		RenderTargetDesc()
		{
			colorFormats.clear();
			colorFormats.shrink_to_fit();
			depthFormat = Format::None;
			msaa = MSAA::Disabled;
		}

		RenderTargetDesc(std::vector<Format> colorFormats, Format depthFormat = Format::None, MSAA msaa = MSAA::Disabled)
		{
			this->colorFormats = colorFormats;
			this->depthFormat = depthFormat;
			this->msaa = msaa;
		}

		std::vector<Format> colorFormats; // Render texture formats
		Format depthFormat; // Depth buffer format
		MSAA msaa;
	};

	struct PipelineDesc
	{
		Shader VS; // Vertex shader
		Shader PS; // Pixel shader
		Shader DS; // Domain shader
		Shader HS; // Hull shader
		Shader GS; // Geometry shader

		//TODO: add stream output here?

		// You must provide one blend state for each render texture you expect to render onto.
		std::vector<BlendDesc> blendStates;
		RasterizerDesc rasterizerState;
		DepthDesc depthState;

		std::vector<VertexElement> vertexLayout;

		uint32_t sampleMask = IGLO_UINT32_MAX;
		bool enableAlphaToCoverage = false;

		Primitive primitiveTopology = Primitive::Undefined;

		// The expected render target format.
		// You can specify Format::None for the depth format if you don't intend to render to a depth buffer.
		// You can specify an empty vector of color formats if you don't intend to render to any render textures.
		RenderTargetDesc renderTargetDesc;

	};

	class Pipeline
	{
	public:
		Pipeline() = default;
		~Pipeline() { Unload(); }

		Pipeline& operator=(Pipeline&) = delete;
		Pipeline(Pipeline&) = delete;

		void Unload();
		bool IsLoaded() const { return isLoaded; }

		// Creates a graphics pipeline state object.
		// Returns true if success.
		bool Load(const IGLOContext&, const PipelineDesc&);

		// A helper function that creates a graphics pipeline state object in a single line of code.
		// Vertex shader bytecode and pixel shader bytecode is loaded from file.
		// You must provide one blend state (BlendDesc) for each expected render texture.
		// Returns true if success.
		bool LoadFromFile(const IGLOContext&, const std::string& filepathVS, const std::string filepathPS,
			const std::string& entryPointNameVS, const std::string& entryPointNamePS, const std::vector<VertexElement>&,
			Primitive, DepthDesc, RasterizerDesc, const std::vector<BlendDesc>&, RenderTargetDesc);

		// A helper function that creates a graphics pipeline state object in a single line of code.
		// You must provide one blend state (BlendDesc) for each expected render texture.
		// Returns true if success.
		bool Load(const IGLOContext&, Shader VS, Shader PS, const std::vector<VertexElement>&, Primitive,
			DepthDesc, RasterizerDesc, const std::vector<BlendDesc>&, RenderTargetDesc);

		// Creates a compute pipeline state object.
		bool LoadAsCompute(const IGLOContext&, Shader CS);

#ifdef IGLO_D3D12
		ID3D12PipelineState* GetD3D12PipelineState() const { return d3d12PipelineState.Get(); }
#endif

	private:
		bool isLoaded = false;
		const IGLOContext* context = nullptr;

#ifdef IGLO_D3D12
		ComPtr<ID3D12PipelineState> d3d12PipelineState;
#endif
	};

	// Manages the allocation of descriptors
	class DescriptorHeap
	{
	public:
		DescriptorHeap() = default;
		~DescriptorHeap() { Unload(); }

		DescriptorHeap& operator=(DescriptorHeap&) = delete;
		DescriptorHeap(DescriptorHeap&) = delete;

		void Unload();
		bool IsLoaded() const { return isLoaded; }

		bool Load(const IGLOContext&, uint32_t maxPersistentResourceDescriptors, uint32_t maxTemporaryResourceDescriptorsPerFrame,
			uint32_t maxSamplerDescriptors);

		void NextFrame();

		// Clears all temporary resource descriptors
		void ClearAllTempResources();

		// Get max number of descriptors
		uint32_t GetMaxPersistentResources() const { return maxPersistentResources; }
		uint32_t GetMaxTemporaryResourcesPerFrame() const { return maxTemporaryResourcesPerFrame; }
		uint32_t GetMaxSamplers() const { return maxSamplers; }

		// Number of descriptors used
		uint32_t GetNumUsedPersistentResources() const { return numUsedPersistentResources; }
		uint32_t GetNumUsedTemporaryResources() const { return numUsedTempResources.at(frameIndex); }
		uint32_t GetNumUsedSamplers() const { return numUsedSamplers; }

		// Allocate persistent resource descriptor
		Descriptor AllocatePersistentResource();
		// Allocate temporary resource descriptor
		Descriptor AllocateTemporaryResource();
		// Allocate sampler descriptor
		Descriptor AllocateSampler();

		// Free descriptor
		void FreePersistentResource(Descriptor);
		void FreeSampler(Descriptor);

#ifdef IGLO_D3D12
		D3D12_CPU_DESCRIPTOR_HANDLE GetD3D12CPUDescriptorHandleForResource(Descriptor d) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetD3D12GPUDescriptorHandleForResource(Descriptor d) const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetD3D12CPUDescriptorHandleForSampler(Descriptor d) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetD3D12GPUDescriptorHandleForSampler(Descriptor d) const;

		ID3D12DescriptorHeap* GetD3D12DescriptorHeap_SRV_CBV_UAV() const { return d3d12DescriptorHeap_SRV_CBV_UAV.Get(); }
		ID3D12DescriptorHeap* GetD3D12DescriptorHeap_Sampler() const { return d3d12DescriptorHeap_Sampler.Get(); }
#endif

	private:
		bool isLoaded = false;
		const IGLOContext* context = nullptr;
		uint32_t frameIndex = 0;
		uint32_t numFrames = 0;

		bool LogErrorIfNotLoaded();

		// Maximum number of descriptors permitted for resources (SRV_CBV_UAV) and samplers.
		uint32_t maxPersistentResources = 0;
		uint32_t maxTemporaryResourcesPerFrame = 0;
		uint32_t maxSamplers = 0;

		// How many persistent descriptors are currently allocated
		uint32_t numUsedPersistentResources = 0;
		uint32_t numUsedSamplers = 0; // samplers are persistent

		// How many temporary descriptors are allocated for each frame
		std::vector<uint32_t> numUsedTempResources;

		// Index of next descriptor to allocate
		uint32_t nextPersistentResource = 0;
		uint32_t nextSampler = 0;

		// Indices of all freed persistent descriptors
		std::vector<uint32_t> freedPersistentResources;
		std::vector<uint32_t> freedSamplers;

#ifdef IGLO_D3D12
		ComPtr<ID3D12DescriptorHeap> d3d12DescriptorHeap_SRV_CBV_UAV;
		ComPtr<ID3D12DescriptorHeap> d3d12DescriptorHeap_Sampler;
#endif
	};

	enum class CommandListType
	{
		// A command list of this type can submit commands of any type.
		Graphics = 0,

		// A command list of this type can only submit compute and copy commands.
		Compute = 1,

		// A command list of this type can only submit copy commands.
		Copy = 2
	};

	struct Receipt
	{
		bool IsNull() const { return fenceValue == IGLO_UINT64_MAX; }

#ifdef IGLO_D3D12
		ID3D12Fence* d3d12Fence = nullptr;
#endif
		uint64_t fenceValue = IGLO_UINT64_MAX;
	};

	class CommandQueue
	{
	public:
		CommandQueue() = default;
		~CommandQueue() { Unload(); }

		CommandQueue& operator=(CommandQueue&) = delete;
		CommandQueue(CommandQueue&) = delete;

		void Unload();
		bool IsLoaded() const { return isLoaded; }

		bool Load(const IGLOContext&);

		Receipt SubmitCommands(const CommandList& commandList);
		Receipt SubmitCommands(const CommandList** commandLists, uint32_t numCommandLists);

		bool IsComplete(Receipt receipt) const;
		bool IsIdle() const;
		void WaitForCompletion(Receipt receipt);
		void WaitForIdle();
		Receipt SignalFence(CommandListType type);

#ifdef IGLO_D3D12
		ID3D12CommandQueue* GetD3D12CommandQueue(CommandListType type) const { return d3d12CommandQueue[(uint32_t)type].Get(); }
#endif
	private:
		bool isLoaded = false;
		const IGLOContext* context = nullptr;

		bool LogErrorIfInvalidCommandListType(CommandListType type) const;


#ifdef IGLO_D3D12
		// One of each: Graphics, Compute, Copy
		ComPtr<ID3D12CommandQueue> d3d12CommandQueue[3];
		ComPtr<ID3D12Fence> d3d12Fence[3];
		uint64_t d3d12FenceValue[3] = {};
		HANDLE fenceEvent = nullptr;
		std::mutex receiptMutex;
		std::mutex waitMutex;
#endif
	};

	enum class BarrierSync : uint64_t
	{
#ifdef IGLO_D3D12
		None = D3D12_BARRIER_SYNC_NONE,
		All = D3D12_BARRIER_SYNC_ALL,
		Draw = D3D12_BARRIER_SYNC_DRAW,
		IndexInput = D3D12_BARRIER_SYNC_INDEX_INPUT,
		VertexShading = D3D12_BARRIER_SYNC_VERTEX_SHADING,
		PixelShading = D3D12_BARRIER_SYNC_PIXEL_SHADING,
		DepthStencil = D3D12_BARRIER_SYNC_DEPTH_STENCIL,
		RenderTarget = D3D12_BARRIER_SYNC_RENDER_TARGET,
		ComputeShading = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
		Raytracing = D3D12_BARRIER_SYNC_RAYTRACING,
		Copy = D3D12_BARRIER_SYNC_COPY,
		Resolve = D3D12_BARRIER_SYNC_RESOLVE,
		ExecuteIndirect = D3D12_BARRIER_SYNC_EXECUTE_INDIRECT,
		Predication = D3D12_BARRIER_SYNC_PREDICATION,
		AllShading = D3D12_BARRIER_SYNC_ALL_SHADING,
		NonPixelShading = D3D12_BARRIER_SYNC_NON_PIXEL_SHADING,
		EmitRaytracingAccelerationStructurePostbuildInfo = D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO,
		ClearUnorderedAccessView = D3D12_BARRIER_SYNC_CLEAR_UNORDERED_ACCESS_VIEW,
		VideoDecode = D3D12_BARRIER_SYNC_VIDEO_DECODE,
		VideoProcess = D3D12_BARRIER_SYNC_VIDEO_PROCESS,
		VideoEncode = D3D12_BARRIER_SYNC_VIDEO_ENCODE,
		BuildRaytracingAccelerationStructure = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
		CopyRaytracingAccelerationStructure = D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE,
		SyncSplit = D3D12_BARRIER_SYNC_SPLIT,
#endif
#ifdef IGLO_VULKAN

		//TODO: figure out all values here

		None = VK_PIPELINE_STAGE_2_NONE,
		All = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, //TODO: is this correct?
		Draw = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, //TODO: is this correct?
		//IndexInput = D3D12_BARRIER_SYNC_INDEX_INPUT,
		//VertexShading = D3D12_BARRIER_SYNC_VERTEX_SHADING,
		PixelShading = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		DepthStencil = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, //TODO: is this correct?
		RenderTarget = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		ComputeShading = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		//Raytracing = D3D12_BARRIER_SYNC_RAYTRACING,
		Copy = VK_PIPELINE_STAGE_2_COPY_BIT,
		Resolve = VK_PIPELINE_STAGE_2_RESOLVE_BIT,
		//ExecuteIndirect = D3D12_BARRIER_SYNC_EXECUTE_INDIRECT,
		//Predication = D3D12_BARRIER_SYNC_PREDICATION,
		AllShading = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		//NonPixelShading = D3D12_BARRIER_SYNC_NON_PIXEL_SHADING,
		//EmitRaytracingAccelerationStructurePostbuildInfo = D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO,
		//ClearUnorderedAccessView = D3D12_BARRIER_SYNC_CLEAR_UNORDERED_ACCESS_VIEW,
		//VideoDecode = D3D12_BARRIER_SYNC_VIDEO_DECODE,
		//VideoProcess = D3D12_BARRIER_SYNC_VIDEO_PROCESS,
		//VideoEncode = D3D12_BARRIER_SYNC_VIDEO_ENCODE,
		//BuildRaytracingAccelerationStructure = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
		//CopyRaytracingAccelerationStructure = D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE,
		//SyncSplit = D3D12_BARRIER_SYNC_SPLIT,
#endif
	};

	enum class BarrierAccess
	{
#ifdef IGLO_D3D12
		Common = D3D12_BARRIER_ACCESS_COMMON,
		VertexBuffer = D3D12_BARRIER_ACCESS_VERTEX_BUFFER,
		ConstantBuffer = D3D12_BARRIER_ACCESS_CONSTANT_BUFFER,
		IndexBuffer = D3D12_BARRIER_ACCESS_INDEX_BUFFER,
		RenderTarget = D3D12_BARRIER_ACCESS_RENDER_TARGET,
		UnorderedAccess = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
		DepthStencilWrite = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,
		DepthStencilRead = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ,
		ShaderResource = D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
		StreamOutput = D3D12_BARRIER_ACCESS_STREAM_OUTPUT,
		IndirectArgument = D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT,
		Predication = D3D12_BARRIER_ACCESS_PREDICATION,
		CopyDest = D3D12_BARRIER_ACCESS_COPY_DEST,
		CopySource = D3D12_BARRIER_ACCESS_COPY_SOURCE,
		ResolveDest = D3D12_BARRIER_ACCESS_RESOLVE_DEST,
		ResolveSource = D3D12_BARRIER_ACCESS_RESOLVE_SOURCE,
		RaytracingAccelerationStructureRead = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ,
		RaytracingAccelerationStructureWrite = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE,
		ShadingRateSource = D3D12_BARRIER_ACCESS_SHADING_RATE_SOURCE,
		VideoDecodeRead = D3D12_BARRIER_ACCESS_VIDEO_DECODE_READ,
		VideoDecodeWrite = D3D12_BARRIER_ACCESS_VIDEO_DECODE_WRITE,
		VideoProcessRead = D3D12_BARRIER_ACCESS_VIDEO_PROCESS_READ,
		VideoProcessWrite = D3D12_BARRIER_ACCESS_VIDEO_PROCESS_WRITE,
		VideoEncodeRead = D3D12_BARRIER_ACCESS_VIDEO_ENCODE_READ,
		VideoEncodeWrite = D3D12_BARRIER_ACCESS_VIDEO_ENCODE_WRITE,
		NoAccess = D3D12_BARRIER_ACCESS_NO_ACCESS,
#endif
#ifdef IGLO_VULKAN

		//TODO: figure out all values here

		Common = 0, //TODO: is this correct? (no flags required for presenting backbuffer?)
		//VertexBuffer = D3D12_BARRIER_ACCESS_VERTEX_BUFFER,
		//ConstantBuffer = D3D12_BARRIER_ACCESS_CONSTANT_BUFFER,
		//IndexBuffer = D3D12_BARRIER_ACCESS_INDEX_BUFFER,
		RenderTarget = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		UnorderedAccess = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		DepthStencilWrite = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		DepthStencilRead = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
		ShaderResource = VK_ACCESS_SHADER_READ_BIT,
		//StreamOutput = D3D12_BARRIER_ACCESS_STREAM_OUTPUT,
		//IndirectArgument = D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT,
		//Predication = D3D12_BARRIER_ACCESS_PREDICATION,
		CopyDest = VK_ACCESS_TRANSFER_WRITE_BIT,
		CopySource = VK_ACCESS_TRANSFER_READ_BIT,
		ResolveDest = VK_ACCESS_TRANSFER_WRITE_BIT,
		ResolveSource = VK_ACCESS_TRANSFER_READ_BIT,
		//RaytracingAccelerationStructureRead = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ,
		//RaytracingAccelerationStructureWrite = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE,
		//ShadingRateSource = D3D12_BARRIER_ACCESS_SHADING_RATE_SOURCE,
		//VideoDecodeRead = D3D12_BARRIER_ACCESS_VIDEO_DECODE_READ,
		//VideoDecodeWrite = D3D12_BARRIER_ACCESS_VIDEO_DECODE_WRITE,
		//VideoProcessRead = D3D12_BARRIER_ACCESS_VIDEO_PROCESS_READ,
		//VideoProcessWrite = D3D12_BARRIER_ACCESS_VIDEO_PROCESS_WRITE,
		//VideoEncodeRead = D3D12_BARRIER_ACCESS_VIDEO_ENCODE_READ,
		//VideoEncodeWrite = D3D12_BARRIER_ACCESS_VIDEO_ENCODE_WRITE,
		//NoAccess = D3D12_BARRIER_ACCESS_NO_ACCESS,
#endif
	};

	enum class BarrierLayout
	{
#ifdef IGLO_D3D12
		Undefined = D3D12_BARRIER_LAYOUT_UNDEFINED,
		Common = D3D12_BARRIER_LAYOUT_COMMON,
		Present = D3D12_BARRIER_LAYOUT_PRESENT,
		GenericRead = D3D12_BARRIER_LAYOUT_GENERIC_READ,
		RenderTarget = D3D12_BARRIER_LAYOUT_RENDER_TARGET,
		UnorderedAccess = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
		DepthStencilWrite = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE,
		DepthStencilRead = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ,
		ShaderResource = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,
		CopySource = D3D12_BARRIER_LAYOUT_COPY_SOURCE,
		CopyDest = D3D12_BARRIER_LAYOUT_COPY_DEST,
		ResolveSource = D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE,
		ResolveDest = D3D12_BARRIER_LAYOUT_RESOLVE_DEST,
		ShadingRateSource = D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE,

		// Video
		VideoDecodeRead = D3D12_BARRIER_LAYOUT_VIDEO_DECODE_READ,
		VideoDecodeWrite = D3D12_BARRIER_LAYOUT_VIDEO_DECODE_WRITE,
		VideoProcessRead = D3D12_BARRIER_LAYOUT_VIDEO_PROCESS_READ,
		VideoProcessWrite = D3D12_BARRIER_LAYOUT_VIDEO_PROCESS_WRITE,
		VideoEncodeRead = D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_READ,
		VideoEncodeWrite = D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_WRITE,

		// Graphics queue (AKA direct queue)
		_GraphicsQueue_Common = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON,
		_GraphicsQueue_GenericRead = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_GENERIC_READ,
		_GraphicsQueue_UnorderedAccess = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,
		_GraphicsQueue_ShaderResource = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE,
		_GraphicsQueue_CopySource = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_SOURCE,
		_GraphicsQueue_CopyDest = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST,

		// Compute queue
		_ComputeQueue_Common = D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COMMON,
		_ComputeQueue_GenericRead = D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_GENERIC_READ,
		_ComputeQueue_UnorderedAccess = D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_UNORDERED_ACCESS,
		_ComputeQueue_ShaderResource = D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_SHADER_RESOURCE,
		_ComputeQueue_CopySource = D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_SOURCE,
		_ComputeQueue_CopyDest = D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_DEST,

		// Video queue
		_VideoQueue_Common = D3D12_BARRIER_LAYOUT_VIDEO_QUEUE_COMMON,
#endif
#ifdef IGLO_VULKAN

		//TODO: figure out all values here.

		Undefined = VK_IMAGE_LAYOUT_UNDEFINED,
		Common = VK_IMAGE_LAYOUT_GENERAL,
		//Present = D3D12_BARRIER_LAYOUT_PRESENT,
		//GenericRead = D3D12_BARRIER_LAYOUT_GENERIC_READ,
		RenderTarget = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		UnorderedAccess = VK_IMAGE_LAYOUT_GENERAL,
		DepthStencilWrite = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		DepthStencilRead = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		ShaderResource = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		CopySource = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		CopyDest = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		ResolveSource = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		ResolveDest = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//ShadingRateSource = D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE,

		// Video
		//VideoDecodeRead = D3D12_BARRIER_LAYOUT_VIDEO_DECODE_READ,
		//VideoDecodeWrite = D3D12_BARRIER_LAYOUT_VIDEO_DECODE_WRITE,
		//VideoProcessRead = D3D12_BARRIER_LAYOUT_VIDEO_PROCESS_READ,
		//VideoProcessWrite = D3D12_BARRIER_LAYOUT_VIDEO_PROCESS_WRITE,
		//VideoEncodeRead = D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_READ,
		//VideoEncodeWrite = D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_WRITE,

		// Graphics queue (AKA direct queue)
		_GraphicsQueue_Common = Common,
		//_GraphicsQueue_GenericRead = GenericRead,
		_GraphicsQueue_UnorderedAccess = UnorderedAccess,
		_GraphicsQueue_ShaderResource = ShaderResource,
		_GraphicsQueue_CopySource = CopySource,
		_GraphicsQueue_CopyDest = CopyDest,

		// Compute queue
		_ComputeQueue_Common = Common,
		//_ComputeQueue_GenericRead = GenericRead,
		_ComputeQueue_UnorderedAccess = UnorderedAccess,
		_ComputeQueue_ShaderResource = ShaderResource,
		_ComputeQueue_CopySource = CopySource,
		_ComputeQueue_CopyDest = CopyDest,

		// Video queue
		//_VideoQueue_Common = D3D12_BARRIER_LAYOUT_VIDEO_QUEUE_COMMON,
#endif
	};

	// An abstraction that simplifies barriers by combining sync, access and layout into one single enum.
	// Contains only the most commonly used barriers.
	enum class SimpleBarrier
	{
		Common = 0,
		PixelShaderResource,
		ComputeShaderResource,
		ComputeShaderUnorderedAccess,
		RenderTarget,
		DepthStencil,
		CopySource,
		CopyDest,
		ResolveSource,
		ResolveDest,
	};
	struct SimpleBarrierInfo
	{
		BarrierSync sync = BarrierSync::None;
		BarrierAccess access = BarrierAccess::Common;
		BarrierLayout layout = BarrierLayout::Undefined;
	};
	SimpleBarrierInfo GetSimpleBarrierInfo(SimpleBarrier simpleBarrier, bool useQueueLayout, CommandListType queueType);

	class CommandList
	{
	public:
		CommandList() = default;
		~CommandList() { Unload(); }

		CommandList& operator=(CommandList&) = delete;
		CommandList(CommandList&) = delete;

		void Unload();
		bool IsLoaded() const { return isLoaded; }

		bool Load(const IGLOContext&, CommandListType);

		// Clears the command list and begins recording commands. 
		// NOTE: A CommandList can begin recording commands max once per frame, unless you manually wait
		// for command execution to finish before recording commands again.
		// If you need to begin recording commands multiple times per frame, use multiple command lists.
		void Begin();

		// Stops recording commands.
		void End();

		void AddGlobalBarrier(BarrierSync syncBefore, BarrierSync syncAfter, BarrierAccess accessBefore, BarrierAccess accessAfter);
		void AddTextureBarrier(const Texture& texture, BarrierSync syncBefore, BarrierSync syncAfter,
			BarrierAccess accessBefore, BarrierAccess accessAfter, BarrierLayout layoutBefore, BarrierLayout layoutAfter, bool discard = false);
		void AddTextureBarrier(const Texture& texture, SimpleBarrier before, SimpleBarrier after, bool useQueueSpecificLayout = true);
		void AddTextureBarrierAtSubresource(const Texture& texture, BarrierSync syncBefore, BarrierSync syncAfter,
			BarrierAccess accessBefore, BarrierAccess accessAfter, BarrierLayout layoutBefore, BarrierLayout layoutAfter,
			uint32_t faceIndex, uint32_t mipIndex, bool discard = false);
		void AddTextureBarrierAtSubresource(const Texture& texture, SimpleBarrier before, SimpleBarrier after,
			uint32_t faceIndex, uint32_t mipIndex, bool useQueueSpecificLayout = true);
		void AddBufferBarrier(const Buffer& buffer, BarrierSync syncBefore, BarrierSync syncAfter, BarrierAccess accessBefore, BarrierAccess accessAfter);
		void FlushBarriers();

		void SetPipeline(const Pipeline&);

		// It's OK to set 'renderTexture' to nullptr if you intend to only render to a depth buffer.
		void SetRenderTarget(const Texture* renderTexture, const Texture* depthBuffer = nullptr);
		void SetRenderTargets(const Texture** renderTextures, uint32_t numRenderTextures, const Texture* depthBuffer);

		// Clears a render texture
		void ClearColor(const Texture& renderTexture, Color color = Colors::Black, uint32_t numRects = 0, const IntRect* rects = nullptr);
		// Clears a depth buffer
		void ClearDepth(const Texture& depthBuffer, float depth = 1.0f, byte stencil = 255, bool clearDepth = true, bool clearStencil = true,
			uint32_t numRects = 0, const IntRect* rects = nullptr);
		void ClearUnorderedAccessBufferFloat(const Buffer& buffer, const float values[4], uint32_t numRects = 0, const IntRect* rects = nullptr);
		void ClearUnorderedAccessBufferUInt32(const Buffer& buffer, const uint32_t values[4], uint32_t numRects = 0, const IntRect* rects = nullptr);
		void ClearUnorderedAccessTextureFloat(const Texture& texture, ig::Color values = ig::Color(0, 0, 0, 0),
			uint32_t numRects = 0, const IntRect* rects = nullptr);
		void ClearUnorderedAccessTextureUInt32(const Texture& texture, const uint32_t values[4], 
			uint32_t numRects = 0, const IntRect* rects = nullptr);

		// Sets graphics push constants, which are directly accessible to the shader.
		// 'sizeInBytes' and 'destOffsetInBytes' must be multiples of 4.
		// NOTE: For compute shader pipelines, call 'SetComputePushConstants' instead!
		void SetPushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes = 0);
		void SetComputePushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes = 0);

		void SetPrimitiveTopology(Primitive primitiveTopology);

		void SetVertexBuffer(const Buffer& vertexBuffer, uint32_t slot = 0);
		void SetVertexBuffers(const Buffer** vertexBuffers, uint32_t count, uint32_t startSlot = 0);
		void SetTempVertexBuffer(const void* data, uint64_t sizeInBytes, uint32_t vertexStride, uint32_t slot = 0);
		void SetIndexBuffer(const Buffer& indexBuffer);

		void Draw(uint32_t vertexCount, uint32_t startVertexLocation = 0);
		void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation = 0, uint32_t startInstanceLocation = 0);
		void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation = 0, uint32_t baseVertexLocation = 0);
		void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation = 0,
			int32_t baseVertexLocation = 0, uint32_t startInstanceLocation = 0);

		void SetViewport(float width, float height);
		void SetViewport(Viewport viewPort);
		void SetViewports(const Viewport* viewPorts, uint32_t count);

		void SetScissorRectangle(int32_t width, int32_t height);
		void SetScissorRectangle(IntRect scissorRect);
		void SetScissorRectangles(const IntRect* scissorRects, uint32_t count);

		void DispatchCompute(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);

		void CopyTexture(const Texture& source, const Texture& destination);
		void CopyTextureSubresource(const Texture& source, uint32_t sourceFaceIndex, uint32_t sourceMipIndex,
			const Texture& destination, uint32_t destFaceIndex, uint32_t destMipIndex);
		void CopyBuffer(const Buffer& source, const Buffer& destination);

		void CopyTempBufferToTexture(const TempBuffer& source, const Texture& destination);
		void CopyTempBufferToTextureSubresource(const TempBuffer& source, const Texture& destination, uint32_t destFaceIndex, uint32_t destMipIndex);
		void CopyTempBufferToBuffer(const TempBuffer& source, const Buffer& destination);

		void ResolveTexture(const Texture& source, const Texture& destination);

		CommandListType GetCommandListType() const { return commandListType; }

#ifdef IGLO_D3D12
		ID3D12GraphicsCommandList7* GetD3D12GraphicsCommandList() const { return d3d12GraphicsCommandList.Get(); }
#endif

	private:
		void CopyTextureToReadableTexture(const Texture& source, const Texture& destination);
		static bool LogErrorIfInvalidPushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes);

		bool isLoaded = false;
		const IGLOContext* context = nullptr;
		uint32_t numFrames = 0;
		uint32_t frameIndex = 0;
		CommandListType commandListType = CommandListType::Graphics;

#ifdef IGLO_D3D12
		std::vector<ComPtr<ID3D12CommandAllocator>> d3d12CommandAllocator; // One for each frame
		ComPtr<ID3D12GraphicsCommandList7> d3d12GraphicsCommandList;
		std::vector<D3D12_GLOBAL_BARRIER> d3d12GlobalBarriers;
		std::vector<D3D12_TEXTURE_BARRIER> d3d12TextureBarriers;
		std::vector<D3D12_BUFFER_BARRIER> d3d12BufferBarriers;
		uint32_t numGlobalBarriers = 0;
		uint32_t numTextureBarriers = 0;
		uint32_t numBufferBarriers = 0;
#endif
	};

	// Placement alignments of various resources that can be placed inside buffers, measured in bytes.
	enum class BufferPlacementAlignment : uint32_t
	{
		VertexOrIndexBuffer = 4,
		RawBuffer = 16,

		// Structured buffers don't have any placement alignment requirements.

#ifdef IGLO_D3D12
		Constant = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT,
		Texture = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT,
		TextureRowPitch = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT,
#endif
#ifdef IGLO_VULKAN
		//TODO: are these values correct?
		Constant = 16,
		Texture = 16,
		TextureRowPitch = 256,
#endif
	};

	struct TempBuffer
	{
		bool IsNull() const { return (data == nullptr); }

#ifdef IGLO_D3D12
		ID3D12Resource* d3d12Resource = nullptr;
#endif
#ifdef IGLO_VULKAN
		//TODO: this
#endif

		uint64_t offset = 0;
		void* data = nullptr;
	};

	class BufferAllocator
	{
	public:
		BufferAllocator() = default;
		~BufferAllocator() { Unload(); }

		BufferAllocator& operator=(BufferAllocator&) = delete;
		BufferAllocator(BufferAllocator&) = delete;

		void Unload();
		bool IsLoaded() const { return isLoaded; }
		bool Load(const IGLOContext&, uint64_t pageSizeInBytes);

		void NextFrame();
		void ClearAllTempPages();

		TempBuffer AllocateTempBuffer(uint64_t sizeInBytes, uint32_t alignment);

		size_t GetNumUsedPages() const;

	private:
		bool isLoaded = false;
		const IGLOContext* context = nullptr;
		uint32_t frameIndex = 0;
		uint32_t numFrames = 0;
		uint64_t pageSize = 0;

		struct Page
		{
			bool IsNull() const { return (mapped == nullptr); }

#ifdef IGLO_D3D12
			ComPtr<ID3D12Resource> d3d12Resource;
#endif
			void* mapped = nullptr;
		};

		struct PerFrame
		{
			std::vector<Page> largePages; // Each page here is larger than pageSize and has a unique size. All temporary.
			std::vector<Page> linearPages; // First pages are persistent, rest are temporary.
			uint64_t linearNextByte = 0; // For the current linear page
		};

		std::vector<PerFrame> perFrame; // One for each frame

		bool LogErrorIfNotLoaded();

		static const size_t numPersistentPages = 1;
		static void ClearTempPagesAtFrame(PerFrame& perFrame);
		static Page CreatePage(const IGLOContext&, uint64_t sizeInBytes);
	};

	struct Viewport
	{
		Viewport()
		{
			topLeftX = 0;
			topLeftY = 0;
			width = 0;
			height = 0;
			minDepth = 0;
			maxDepth = 0;
		}
		Viewport(float topLeftX, float topLeftY, float width, float height, float minDepth = 0, float maxDepth = 1.0f)
		{
			this->topLeftX = topLeftX;
			this->topLeftY = topLeftY;
			this->width = width;
			this->height = height;
			this->minDepth = minDepth;
			this->maxDepth = maxDepth;
		}

		float topLeftX;
		float topLeftY;
		float width;
		float height;
		float minDepth;
		float maxDepth;
	};

	enum class MouseButton
	{
		None = 0,
		Left,
		Middle,
		Right,
		XButton1,
		XButton2,
	};

	enum class Key
	{
		Unknown = 0,
		A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
		Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
		Numpad0, Numpad1, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
		F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15,
		F16, F17, F18, F19, F20, F21, F22, F23, F24,
		LeftShift, RightShift,
		LeftControl, RightControl,
		LeftAlt, RightAlt,
		LeftSystem, RightSystem, // The windows keys
		Backspace, Tab, Clear, Enter, Pause, CapsLock, Escape, Space, PageUp, PageDown, End, Home,
		Left, Up, Right, Down, PrintScreen, Insert, Delete,
		Sleep, Multiply, Add, Seperator, Subtract, Decimal, Divide,
		NumLock, ScrollLock,
		Apps, // Next to right ctrl key. Shows a context menu when pressed.
		VolumeMute, VolumeDown, VolumeUp, MediaNextTrack, MediaPrevTrack, MediaStop, MediaPlayPause,
		Plus, Comma, Minus, Period,
		OEM_1,// ';:' for US, '^¨~' for SE
		OEM_2,// '/?' for US, ''*' for SE
		OEM_3,// '`~' for US, 'Ö' for SE
		OEM_4,//  '[{' for US, '´`' for SE
		OEM_5,//  '\|' for US, '§½' for SE
		OEM_6,//  ']}' for US, 'Å' for SE
		OEM_7,//  ''"' for US, 'Ä' for SE
		OEM_8,
		OEM_102, // '<>|' for SE
	};

	enum class EventType
	{
		None = 0,
		Resize, // The backbuffer changed size.
		CloseRequest, // User either pressed Alt F4 or pressed the X button to close the window.
		LostFocus,
		GainedFocus,
		MouseMove,
		MouseWheel,
		MouseButtonDown,
		MouseButtonUp,
		KeyDown,
		KeyPress, // KeyPress repeats after a key is held down long enough.
		KeyUp,
		TextEntered,
		DragAndDrop, // User has dragged and dropped atleast 1 file onto the iglo window
		Minimized, // Window is minimized. (you can't see the window anymore)
		Restored, // Window is no longer minimized. (you can now see the window)
	};

	class Event
	{
	public:
		Event()
		{
			type = EventType::None;
			dragAndDrop = DragAndDropEvent();
			key = Key::Unknown;
			resize = Extent2D();
			mouse = MouseEvent();
			textEntered = TextEnteredEvent();
		}

	private:
		struct MouseEvent
		{
			int32_t x = 0;
			int32_t y = 0;
			int32_t scrollWheel = 0;
			MouseButton button = MouseButton::None;
		};
		struct DragAndDropEvent
		{
			std::vector<std::string> filenames;
		};
		struct TextEnteredEvent
		{
			uint32_t character_utf32 = 0; // UTF-32 codepoint
		};
	public:
		EventType type;
		DragAndDropEvent dragAndDrop; // Used by these events: DragAndDrop
		union
		{
			Key key; // Used by: KeyDown, KeyPress, KeyUp
			Extent2D resize; // Used by: Resize
			MouseEvent mouse; // Used by: MouseMove, MouseWheel, MouseButtonDown, MouseButtonUp
			TextEnteredEvent textEntered; // Used by: TextEntered
		};
	};

	struct WindowSettings
	{
		WindowSettings(
			std::string title = igloVersion,
			uint32_t width = 640,
			uint32_t height = 480,
			bool resizable = false,
			bool centered = false,
			bool bordersVisible = true,
			bool visible = true)
		{
			this->title = title;
			this->width = width;
			this->height = height;
			this->resizable = resizable;
			this->centered = centered;
			this->bordersVisible = bordersVisible;
			this->visible = visible;
		}
		std::string title; // Window title. The string is in utf8.
		uint32_t width; // Window width.
		uint32_t height; // Window height.
		bool resizable; // Whether the window is resizable.
		bool centered; // Whether the window should be positioned in the center of the screen.
		bool bordersVisible; // Whether the window should have its borders visible.
		bool visible; // Whether the window should be visible.
	};

	struct RenderSettings
	{
		RenderSettings(
			PresentMode presentMode = PresentMode::Vsync,
			Format backBufferFormat = Format::BYTE_BYTE_BYTE_BYTE,
			uint32_t numFramesInFlight = 2,
			uint32_t numBackBuffers = 3,
			uint64_t bufferAllocatorPageSize = IGLO_MEGABYTE * 32,
			uint32_t maxPersistentResourceDescriptors = 16384,
			uint32_t maxTemporaryResourceDescriptorsPerFrame = 4096,
			uint32_t maxSamplerDescriptors = 1024)
		{
			this->presentMode = presentMode;
			this->backBufferFormat = backBufferFormat;
			this->numFramesInFlight = numFramesInFlight;
			this->numBackBuffers = numBackBuffers;
			this->bufferAllocatorPageSize = bufferAllocatorPageSize;
			this->maxPersistentResourceDescriptors = maxPersistentResourceDescriptors;
			this->maxTemporaryResourceDescriptorsPerFrame = maxTemporaryResourceDescriptorsPerFrame;
			this->maxSamplerDescriptors = maxSamplerDescriptors;
		}

		PresentMode presentMode;
		Format backBufferFormat;

		uint32_t numFramesInFlight;
		uint32_t numBackBuffers;

		uint64_t bufferAllocatorPageSize;

		uint32_t maxPersistentResourceDescriptors;
		uint32_t maxTemporaryResourceDescriptorsPerFrame;
		uint32_t maxSamplerDescriptors;
	};

	//////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////// IGLO Context  ////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////

	class IGLOContext
	{
	public:
		IGLOContext() = default;
		~IGLOContext() { Unload(); }

		IGLOContext& operator=(IGLOContext&) = delete;
		IGLOContext(IGLOContext&) = delete;

		//------------------ Core ------------------//

		// Creates a graphics device context and a window. Returns true if success.
		bool Load(WindowSettings, RenderSettings, bool showPopupIfFailed = true);

		// Destroys the graphics device context and window.
		// It's OK to call this function even when the IGLOContext is already unloaded.
		void Unload();

		// If true, graphics device context and window have been created and can be used.
		bool IsLoaded() const { return isLoaded; }

		//------------------ Window ------------------//

		// Gets the next event. If there are no new events, this will return false.
		bool GetEvent(Event& out_event);

		// Gets the next event. If there are no new events, the thread will sleep here until a new event is received.
		// Returns false if no new event is received until timeout (if this is the case, 'out_event' is not modified).
		bool WaitForEvent(Event& out_event, uint32_t timeoutMilliseconds);

		// On the Windows platform, dragging the window will create something called a modal message loop which freezes
		// the app until the modal action is finished. To prevent your app from freezing during window move/resize,
		// set 'callbackModalLoop' to point to your draw/update/event/timing functions.
		// NOTE: If the thread doesn't return from the modal loop callback function, the window will be unresponsive.
		// Don't run a GetEvent() loop or MainLoop inside Update(),Draw(),OnEvent() if those were called from this callback function.
		void SetModalLoopCallback(CallbackModalLoop callbackModalLoop, void* callbackModalLoopUserData);
		void GetModalLoopCallback(CallbackModalLoop& out_callbackModalLoop, void*& out_callbackModalLoopUserData);
		// Whether or not the main thread is currently inside the modal loop callback function call.
		// If this returns true, running a GetEvent() loop or MainLoop on the main thread will make the window unresponsive.
		bool IsInsideModalLoopCallback() const { return this->insideModalLoopCallback; }

		void SetOnDeviceRemovedCallback(CallbackOnDeviceRemoved callbackOnDeviceRemoved);

		// Gets the mouse position relative to the topleft corner of the window.
		IntPoint GetMousePosition() const { return mousePosition; }
		int32_t GetMouseX() const { return mousePosition.x; }
		int32_t GetMouseY() const { return mousePosition.y; }

		// Checks if a mouse button is currently held down
		bool IsMouseButtonDown(MouseButton button) const;

		// Checks if a keyboard key is currently down
		bool IsKeyDown(Key key) const;

		// Sets the preferred window visibility in windowed mode. (Shows or hides the window)
		void SetWindowVisible(bool visible);
		// Gets the preferred window visibility in windowed mode.
		bool GetWindowVisible() const { return windowedVisible; }
		// Whether or not the window is currently minimized.
		bool GetWindowMinimized() const { return windowMinimizedState; }
		// Sets the window title.
		void SetWindowTitle(std::string title);
		// Gets the window title.
		const std::string& GetWindowTitle() const { return windowTitle; }
		// Sets the preferred window client size (and back buffer size) while in windowed mode.
		void SetWindowSize(uint32_t width, uint32_t height);
		// Gets the preferred window client size (and back buffer size) while in windowed mode.
		Extent2D GetWindowSize() const { return windowedSize; }
		// Sets the preferred window position while in windowed mode.
		void SetWindowPosition(int32_t x, int32_t y);
		// Gets the preferred window position while in windowed mode.
		IntPoint GetWindowPosition() const { return windowedPos; }
		// If user should be able to resize the window.
		void SetWindowResizable(bool resizable);
		// Gets whether window can be resized by dragging.
		bool GetWindowResizable() { return windowedResizable; }
		// Sets the preferred window border visibility in windowed mode.
		void SetWindowBordersVisible(bool visible);
		// Gets window border visibility.
		bool GetWindowBordersVisible() const { return windowedBordersVisible; }
		// Sets the minimum size the window can be while in windowed mode. Setting values to 0 will disable these size constraints.
		void SetMinWindowSize(uint32_t minWidth, uint32_t minHeight) { this->windowedMinSize = Extent2D(minWidth, minHeight); }
		// Gets the maximum size the window can be while in windowed mode.
		Extent2D GetMinWindowSize() const { return this->windowedMinSize; }
		// Sets the maximum size the window can be while in windowed mode. Setting values to 0 will disable these size constraints.
		void SetMaxWindowSize(uint32_t maxWidth, uint32_t maxHeight) { this->windowedMaxSize = Extent2D(maxWidth, maxHeight); }
		// Gets the maximum size the window can be while in windowed mode.
		Extent2D GetMaxWindowSize() const { return this->windowedMaxSize; }

		// Centers the window while in windowed mode.
		void CenterWindow();
		// If enabled, files are allowed to be dragged and dropped onto the iglo window.
		void EnableDragAndDrop(bool enable);
		bool GetDragAndDropEnabled() { return windowEnableDragAndDrop; }

		// Sets the window icon from an image.
		void SetWindowIconFromImage(const Image& icon);
#ifdef _WIN32
		// Sets the window icon from a resource defined in 'resource.h'. Example usage: SetWindowIconFromResource(IDI_ICON1).
		// This function is only available on the Windows platform.
		void SetWindowIconFromResource(int icon);

		HWND GetWindowHWND() const { return windowHWND; }

		typedef bool(*CallbackWndProcHook)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		// Allows you to handle WndProc messages before iglo handles them.
		// This is needed for when you use imgui with iglo.
		void SetWndProcHookCallback(CallbackWndProcHook callback);
#endif

		// Gets the screen resolution of the monitor this window is currently inside.
		// Returns {0,0} if failed.
		Extent2D GetActiveMonitorScreenResolution();

		std::vector<Extent2D> GetAvailableScreenResolutions();

		// Toggles between windowed mode or borderless fullscreen mode depending on which one is active at the moment.
		// The borderless fullscreen mode will always use a resolution that matches the active desktop resolution.
		void ToggleFullscreen();

		// Enters windowed mode.
		// 'width' and 'height' determines the resolution of the backbuffer and the client size of the window.
		void EnterWindowedMode(uint32_t width, uint32_t height, Format format = Format::BYTE_BYTE_BYTE_BYTE);

		// Enters borderless windowed fullscreen mode.
		// 'monitor' determines what monitor to enter fullscreen in.
		// The backbuffer size will match the active desktop resolution.
		// If you want to use a different resolution than the desktop resolution,
		// you should render your scene in a rendertarget with whatever resolution you want,
		// then draw that rendertarget texture stretched onto the entire backbuffer.
		// Basically, in fullscreen mode you manage the rendering resolution manually.
		void EnterFullscreenMode(Format format = Format::BYTE_BYTE_BYTE_BYTE, uint32_t monitor = 0);

		DisplayMode GetDisplayMode() const { return displayMode; }

		//------------------ Graphics ------------------//

		std::string GetGraphicsAPIShortName() const { return graphicsAPIShortName; }
		std::string GetGraphicsAPIFullName() const { return graphicsAPIFullName; }
		std::string GetGraphicsAPIRenderer() const { return graphicsAPIRenderer; }
		std::string GetGraphicsAPIVendor() const { return graphicsAPIVendor; }

		void SetPresentMode(PresentMode mode);
		PresentMode GetPresentMode() const { return presentMode; }

		void SetNumBackBuffers(uint32_t numBackBuffers);

		// Sends a command to the GPU to present the backbuffer to the screen.
		// This begins a new frame.
		// The CPU will wait a bit here if the GPU is too far behind.
		void Present();

		// Submits commands to the GPU
		Receipt Submit(const CommandList& commandList);
		// Submits commands to the GPU from multiple command lists
		Receipt Submit(const CommandList** commandLists, uint32_t numCommandLists);

		// Returns true if the commands associated with the given receipt has finished executing.
		// Null receipts are legal to pass to this function, they will be treated as completed.
		bool IsComplete(Receipt receipt) const;

		// The CPU will wait for the commands associated with the given receipt to finish executing.
		// Null receipts are legal to pass to this function, they will be treated as completed.
		void WaitForCompletion(Receipt receipt);

		// Waits for the graphics device to finish executing all commands.
		void WaitForIdleDevice();

		// Unloading a texture that's used in the middle of a scene can cause problems
		// because the GPU may still depend on that resource in a previous frame.
		// To solve this, store the texture using a shared pointer, then call this function to
		// share it with IGLOContext the moment you need to discard that texture.
		// IGLOContext will hold the shared pointer long enough to guarantee the GPU is finished using it before discarding it.
		// For this to work, don't call texture->Unload() directly. When the last instance of the shared pointer is discarded,
		// the texture object will automatically call its destructor which calls Texture::Unload() which frees its GPU resources.
		void DelayedTextureUnload(std::shared_ptr<Texture> texture) const;

		Descriptor CreateTempConstant(const void* data, uint64_t numBytes) const;
		Descriptor CreateTempStructuredBuffer(const void* data, uint32_t elementStride, uint32_t numElements) const;
		Descriptor CreateTempRawBuffer(const void* data, uint64_t numBytes) const;

		uint32_t GetMaxTextureSize() const { return maxTextureSize; }
		uint32_t GetMaxVerticesPerDrawCall() const { return maxVerticesPerDrawCall; }

		uint32_t GetNumFramesInFlight() const { return numFramesInFlight; }
		uint32_t GetNumBackBuffers() const { return numBackBuffers; }
		uint32_t GetFrameIndex() const { return frameIndex; }

		// Gets the width of the backbuffer.
		uint32_t GetWidth() const { return backBufferSize.width; }
		// Gets the height of the backbuffer.
		uint32_t GetHeight() const { return backBufferSize.height; }
		// Gets the width and height of the backbuffer.
		Extent2D GetBackBufferSize() const { return backBufferSize; }
		RenderTargetDesc GetBackBufferRenderTargetDesc() const { return RenderTargetDesc({ backBufferFormat }, Format::None, MSAA::Disabled); }

		const Texture& GetBackBuffer() const;
		DescriptorHeap& GetDescriptorHeap() const { return descriptorHeap; }
		BufferAllocator& GetBufferAllocator() const { return bufferAllocator; }
		CommandQueue& GetCommandQueue() { return commandQueue; }

		// Gets the max allowed MSAA per texture format.
		MSAA GetMaxMultiSampleCount(Format textureFormat) const;

		const Pipeline& GetMipmapGenerationPipeline() const { return generateMipmapsPipeline; }

		// The bilinear clamp sampler is needed for the mipmap generation pipeline.
		const Descriptor* GetBilinearClampSamplerDescriptor() const { return bilinearClampSampler.GetDescriptor(); }

#ifdef IGLO_D3D12
		ID3D12Device10* GetD3D12Device() const { return d3d12Device.Get(); }
		IDXGISwapChain3* GetD3D12SwapChain() const { return d3d12SwapChain.Get(); }

		uint32_t GetD3D12DescriptorSize_RTV() const { return d3d12DescriptorSize_RTV; }
		uint32_t GetD3D12DescriptorSize_DSV() const { return d3d12DescriptorSize_DSV; }
		uint32_t GetD3D12DescriptorSize_Sampler() const { return d3d12DescriptorSize_Sampler; }
		uint32_t GetD3D12DescriptorSize_CBV_SRV_UAV() const { return d3d12DescriptorSize_CBV_SRV_UAV; }

		ID3D12RootSignature* GetD3D12BindlessRootSignature() const { return d3d12BindlessRootSignature.Get(); }

		D3D12_CPU_DESCRIPTOR_HANDLE GetD3D12RTVHandle() const { return d3d12RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetD3D12DSVHandle() const { return d3d12DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetD3D12UAVHandle() const { return d3d12UAVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(); }
#endif

	private:
		//------------------ Core ------------------//
		bool isLoaded = false; // True if window and graphics device are loaded.

		bool LoadWindow(WindowSettings, bool showPopupIfFailed);
		bool LoadGraphicsDevice(RenderSettings, bool showPopupIfFailed, Extent2D backBufferSize);
		void UnloadWindow();
		void UnloadGraphicsDevice();

		void OnFatalError(const std::string& message, bool popupMessage);

		//------------------ Window ------------------//

		std::queue<Event> eventQueue; // Window event queue
		IntPoint mousePosition = IntPoint(0, 0);
		PackedBoolArray mouseButtonIsDown;
		PackedBoolArray keyIsDown;

		std::string windowTitle = "";
		bool windowMinimizedState = false; // Whether the window is currently minimized.
		bool windowEnableDragAndDrop = false; // If true, files can be drag and dropped on the window

		bool windowedBordersVisible = false; // User specified this to be the border visibility of the window in windowed mode
		bool windowedResizable = false; // User specified this to be the resizability of the window in windowed mode
		bool windowedVisible = false; // User specified this to be the visibility of the window in windowed mode
		IntPoint windowedPos; // User specified this to be the position of the window in windowed mode.
		Extent2D windowedSize;  // User specified this to be the size of the window in windowed mode
		Extent2D windowedMinSize;
		Extent2D windowedMaxSize;

		CallbackModalLoop callbackModalLoop = nullptr;
		void* callbackModalLoopUserData = nullptr;
		bool insideModalLoopCallback = false; // If true, the main thread is inside a callback function called from inside a modal operation.

		//------------------ WIN32 Specific ------------------//
#ifdef _WIN32
		static constexpr UINT windowClassStyle = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		static constexpr LONG windowStyleWindowed = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		static constexpr LONG windowStyleWindowedResizable = windowStyleWindowed | WS_THICKFRAME | WS_MAXIMIZEBOX;
		static constexpr LONG windowStyleBorderless = WS_POPUP;

		// The number of iglo windows that exist in total (windows are counted accross multiple IGLOContext's).
		// We must keep track of the number of windows that exist because a window class should only be registered once,
		// and unregistered after the last window is destroyed.
		static unsigned int numWindows;
		static bool windowClassIsRegistered;

		HWND windowHWND = 0; // Window handle
		LPCWSTR	windowClassName = L"iglo";
		HICON windowIconOwned = 0; // This icon should be released with DestroyIcon() when it stops being used.

		bool windowActiveResizing = false; // If true, window has entered size move (user is moving or resizing window)
		bool windowActiveMenuLoop = false; // If true, user created a context menu on this window.
		bool ignoreSizeMsg = false; // To ignore all WM_SIZE messages sent by SetWindowLongPtr().

		void CaptureMouse(bool captured);
		void UpdateWin32Window(IntPoint pos, Extent2D clientSize, bool resizable, bool bordersVisible, bool visible,
			bool topMost, bool gainFocus);

		//TODO: Implement cursor stuff for Windows and Linux.
		HCURSOR windowCursor = 0;

		CallbackWndProcHook callbackWndProcHook = nullptr;
		LRESULT CALLBACK PrivateWndProc(UINT msg, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

		//------------------ Linux Specific ------------------//
#ifdef __linux__
		Display* displayHandle = nullptr;
		Window windowHandle = 0;
		Screen* screenHandle = nullptr;
		XVisualInfo visualInfo;
		int screenId = 0;
		XSetWindowAttributes windowAttributes;
		Atom atomWmDeleteWindow = 0;
		int x11fileDescriptor = 0;

		void PrivateLinuxProc(XEvent e);
#endif

		//------------------ Graphics ------------------//

		std::vector<std::unique_ptr<Texture>> backBuffers;
		Extent2D backBufferSize;
		Format backBufferFormat = Format::None;

		struct EndOfFrame
		{
			Receipt graphicsReceipt;
			Receipt computeReceipt;
			Receipt copyReceipt;
			mutable std::vector<std::shared_ptr<Texture>> delayedUnloadTextures;
		};
		std::vector<EndOfFrame> endOfFrame; // One for each frame

		std::string graphicsAPIShortName = "";
		std::string graphicsAPIFullName = "";
		std::string graphicsAPIRenderer = "";
		std::string graphicsAPIVendor = "";

		DisplayMode displayMode = DisplayMode::Windowed;
		PresentMode presentMode = PresentMode::Vsync;

		uint32_t maxTextureSize = 0;
		uint32_t maxVerticesPerDrawCall = 0;

		uint32_t numFramesInFlight = 0;
		uint32_t numBackBuffers = 0;
		uint32_t frameIndex = 0;

		Pipeline generateMipmapsPipeline;
		Sampler bilinearClampSampler; // For the mipmap generation compute shader

		mutable DescriptorHeap descriptorHeap;
		mutable BufferAllocator bufferAllocator;
		CommandQueue commandQueue;

		CallbackOnDeviceRemoved callbackOnDeviceRemoved = nullptr;

		void MoveToNextFrame();
		void ClearAllTempResources();

		// Changes the backbuffer size and format. Returns true if success.
		bool ResizeBackBuffer(Extent2D dimensions, Format format);
		bool CreateSwapChain(Extent2D dimensions, Format format, uint32_t bufferCount, std::string& out_errStr);

		//------------------ D3D12 specific ------------------//
#ifdef IGLO_D3D12

		const D3D_FEATURE_LEVEL d3d12FeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_0;
		const DXGI_SWAP_EFFECT d3d12SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		const UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		ComPtr<ID3D12Device10> d3d12Device;
		ComPtr<IDXGISwapChain3> d3d12SwapChain;
		ComPtr<IDXGIFactory4> d3d12Factory;
		ComPtr<ID3D12RootSignature> d3d12BindlessRootSignature;
		ComPtr<ID3D12DescriptorHeap> d3d12RTVDescriptorHeap; // For clearing and setting render textures
		ComPtr<ID3D12DescriptorHeap> d3d12DSVDescriptorHeap; // For clearing and setting depth buffers
		ComPtr<ID3D12DescriptorHeap> d3d12UAVDescriptorHeap; // For clearing unordered access textures/buffers

		uint32_t d3d12DescriptorSize_RTV = 0;
		uint32_t d3d12DescriptorSize_DSV = 0;
		uint32_t d3d12DescriptorSize_Sampler = 0;
		uint32_t d3d12DescriptorSize_CBV_SRV_UAV = 0;

		// A cached list of max MSAA values for all formats.
		mutable std::vector<byte> maxD3D12MSAAPerFormat;

#endif

	};


} //end of namespace ig




/*
------------------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright(c) 2025 Christoffer Chiniquy
Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files(the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions : The
above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain. Anyone is free to
copy, modify, publish, use, compile, sell, or distribute this software, either in source
code form or as a compiled binary, for any purpose, commercial or non - commercial, and by
any means. In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public domain.
We make this dedication for the benefit of the public at large and to the detriment of our
heirs and successors. We intend this dedication to be an overt act of relinquishment in
perpetuity of all present and future rights to this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------

*/
