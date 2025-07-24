/*
	iglo is a public domain rendering abstraction layer for D3D12 and Vulkan.
	https://github.com/c-chiniquy/iglo
	See the bottom of this document for licensing details.
*/

// -------------------- Version --------------------//
#define IGLO_VERSION_MAJOR 0
#define IGLO_VERSION_MINOR 3
#define IGLO_VERSION_PATCH 1

#define IGLO_STRINGIFY_HELPER(x) #x
#define IGLO_STRINGIFY(x) IGLO_STRINGIFY_HELPER(x)
#define IGLO_VERSION_STRING \
    IGLO_STRINGIFY(IGLO_VERSION_MAJOR) "." \
    IGLO_STRINGIFY(IGLO_VERSION_MINOR) "." \
    IGLO_STRINGIFY(IGLO_VERSION_PATCH)

// -------------------- Includes --------------------//
#pragma once
#include "iglo_config.h"
#include "iglo_utility.h"

#include <array>
#include <queue>
#include <mutex>
#include <optional>
#include <cassert>
#include <functional>

// -------------------- Forward Declarations --------------------//
namespace ig
{
	enum class LogType;
	struct DetailedResult;
	enum class MSAA;
	enum class PresentMode;
	struct SupportedPresentModes;
	enum class DisplayMode;
	enum class TextureWrapMode;
	enum class PrimitiveTopology;
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
	enum class DescriptorType;
	struct Descriptor;
	class Sampler;
	enum class Format;
	enum class IndexFormat;
	struct ImageDesc;
	class Image;
	enum class TextureUsage;
	struct ClearValue;
	struct TextureDesc;
	struct WrappedTextureDesc;
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
	enum class BarrierAccess : uint64_t;
	enum class BarrierLayout : uint64_t;
	enum class SimpleBarrier;
	struct SimpleBarrierInfo;
	class CommandList;
	struct TempBuffer;
	class TempBufferAllocator;
	struct Viewport;
	enum class MouseButton;
	enum class Key;
	enum class EventType;
	class Event;
	struct WindowSettings;
	struct RenderSettings;
	struct WindowState;
	struct WindowResizeConstraints;
	struct BufferPlacementAlignments;
	struct GraphicsSpecs;
	class IGLOContext;

	// -------------------- Constants --------------------//
	constexpr uint32_t NUM_DESCRIPTOR_TYPES = 5;
	constexpr uint32_t MAX_SIMULTANEOUS_RENDER_TARGETS = 8;
	constexpr uint32_t MAX_BATCHED_BARRIERS_PER_TYPE = 16;
	constexpr uint32_t MAX_VERTEX_BUFFER_BIND_SLOTS = 32;
	constexpr uint32_t MAX_COMMAND_LISTS_PER_SUBMIT = 64;

	// The guaranteed minimum push constant size is 256 bytes in D3D12 and 128 bytes in Vulkan 1.3.
	// To ensure cross-compatibility, we cap the maximum at 128 bytes.
	// Push constants should not be used for large data transfers anyway.
	constexpr uint64_t MAX_PUSH_CONSTANTS_BYTE_SIZE = 128;
}

// -------------------- Backend Implementations --------------------//

// Windows
#ifdef _WIN32
#ifdef IGLO_WIN32_VULKAN
#define IGLO_VULKAN
#else
#define IGLO_D3D12
#endif
#include "backends/iglo_impl_win32.h"
#endif

// Linux
#ifdef __linux__
#define IGLO_VULKAN
#include "backends/iglo_impl_x11.h"
#endif

// D3D12
#ifdef IGLO_D3D12
#define IGLO_GRAPHICS_API_STRING "D3D12"
#include "backends/iglo_impl_d3d12.h"
#endif

// Vulkan
#ifdef IGLO_VULKAN
#define IGLO_GRAPHICS_API_STRING "Vulkan"
#include "backends/iglo_impl_vulkan.h"
#endif

// -------------------- Declarations --------------------//
namespace ig
{
	// Callbacks
	using CallbackLog = std::function<void(LogType messageType, const std::string& message)>;
	using CallbackModalLoop = std::function<void()>;
	using CallbackOnDeviceRemoved = std::function<void(std::string deviceRemovalReason)>;

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

	std::string GetGpuVendorNameFromID(uint32_t vendorID);

	struct DetailedResult
	{
		const bool success = false;
		const std::string errorMessage; // Will be empty if the operation succeeded.

		[[nodiscard]] static DetailedResult MakeSuccess() { return { true }; }
		[[nodiscard]] static DetailedResult MakeFail(const std::string& errorMessage) { return { false, errorMessage }; }
		explicit operator bool() const { return success; }
	};

	// Multisampled anti aliasing
	enum class MSAA
	{
		Disabled = 1,
		X2 = 2,
		X4 = 4,
		X8 = 8,
		X16 = 16
	};

#ifdef IGLO_D3D12
	enum class PresentMode
	{
		// Lowest latency of all presentation modes.
		// This is also known as "Vsync Off".
		ImmediateWithTearing = 0,

		// No tearing, lower latency than Vsync, but can drop frames.
		// In D3D12, the more frames in flight you use the lower latency you get, at the cost of higher GPU usage.
		// This is also known as "Mailbox" and "Fast Vsync".
		Immediate,

		// No dropped frames, no tearing, high latency.
		// The more frames in flight you use, the higher the latency.
		Vsync,

		VsyncHalf,
	};
#endif
#ifdef IGLO_VULKAN
	enum class PresentMode
	{
		ImmediateWithTearing = VK_PRESENT_MODE_IMMEDIATE_KHR,
		Immediate = VK_PRESENT_MODE_MAILBOX_KHR,
		Vsync = VK_PRESENT_MODE_FIFO_KHR,
		VsyncRelaxed = VK_PRESENT_MODE_FIFO_RELAXED_KHR,
	};
#endif

	// Must match 'PresentMode'
	struct SupportedPresentModes
	{
		bool immediateWithTearing = false;
		bool immediate = false;
		bool vsync = false;
#ifdef IGLO_D3D12
		bool vsyncHalf = false;
#endif
#ifdef IGLO_VULKAN
		bool vsyncRelaxed = false;
#endif
	};

	enum class DisplayMode
	{
		Windowed = 0,
		BorderlessFullscreen = 1,
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
		Clamp = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		BorderColor = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
#endif
	};

	enum class PrimitiveTopology
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
		Zero = 0,
		All
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
		Back = 2, // The back faces (that face away from you) are culled. This is the default cull mode.
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

	struct BlendDesc
	{
		bool enabled = false;
		BlendData srcBlend = BlendData::One;
		BlendData destBlend = BlendData::Zero;
		BlendOperation blendOp = BlendOperation::Add;
		BlendData srcBlendAlpha = BlendData::One;
		BlendData destBlendAlpha = BlendData::Zero;
		BlendOperation blendOpAlpha = BlendOperation::Add;
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

		// Back faces are culled. This is the default rasterizer state for most 3D rendering.
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

	enum class DescriptorType
	{
		ConstantBuffer_CBV = 0, // CBV
		RawOrStructuredBuffer_SRV_UAV = 1, // SRV or UAV
		Texture_SRV = 2, // SRV
		Texture_UAV = 3, // UAV
		Sampler = 4,
	};
	static_assert((uint32_t)DescriptorType::Sampler + 1 == NUM_DESCRIPTOR_TYPES,
		"NUM_DESCRIPTOR_TYPES must match number of DescriptorType enum values.");

	struct Descriptor
	{
		Descriptor()
		{
			SetToNull(); // The default constructor must produce a null descriptor.
			type = DescriptorType::ConstantBuffer_CBV; // dummy value
		}
		Descriptor(uint32_t heapIndex, DescriptorType type)
		{
			this->heapIndex = heapIndex;
			this->type = type;
		}

		bool IsNull() const { return (heapIndex == IGLO_UINT32_MAX); }
		void SetToNull() { heapIndex = IGLO_UINT32_MAX; }

		uint32_t heapIndex;
		DescriptorType type;
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
#ifdef IGLO_VULKAN
		VkSampler vkSampler = VK_NULL_HANDLE;
#endif

		DetailedResult Impl_Load(const IGLOContext&, const SamplerDesc& desc);
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
		FLOAT16, // 16-bit float format. Grayscale. Uses red color in pixel shader.
		FLOAT16_FLOAT16,
		FLOAT16_FLOAT16_FLOAT16_FLOAT16,
		FLOAT, // 32-bit float format. Grayscale. Uses red color in pixel shader.
		FLOAT_FLOAT,
		FLOAT_FLOAT_FLOAT,
		FLOAT_FLOAT_FLOAT_FLOAT, // RGBA. Each color is a float. 128 bits per pixel.

		// HDR format. 10 bit color. 2 bit alpha. 32 bits per pixel. Order: RGBA in D3D12, ARGB in Vulkan.
		UINT10_UINT10_UINT10_UINT2,
		UINT10_UINT10_UINT10_UINT2_NotNormalized,

		// HDR format. 32 bits per pixel. Order: RGB in D3D12, BGR in Vulkan.
		UFLOAT11_UFLOAT11_UFLOAT10,

		// HDR format. 32 bits per pixel. 5 bit shared exponent. Order: RGBE in D3D12, EBGR in Vulkan.
		UFLOAT9_UFLOAT9_UFLOAT9_UFLOAT5,

		// RGBA. Uses the standard RGB color space.
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

		// HDR Format. RGBA. 16-bit float. UFLOAT means unsigned float, and SFLOAT means signed float.
		BC6H_UFLOAT16,
		BC6H_SFLOAT16,

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

		// For block-compressed formats, this is > 0.
		// For non-block-compressed formats, this is 0.
		// To calculate the row pitch for block-compressed formats, do this: max( 1, ((width+3)/4) ) * blockSize
		uint32_t blockSize = 0;

		// True if this format is a depth format.
		bool isDepthFormat = false;

		// True if this format is a depth format with a stencil component.
		bool hasStencilComponent = false;

		// If true, this format uses sRGB color space.
		bool is_sRGB = false;

		// The sRGB opposite of given format if one exists.
		// Example: BYTE_BYTE_BYTE_BYTE_sRGB has the sRGB opposite BYTE_BYTE_BYTE_BYTE and vice versa.
		// Will be Format::None if a sRGB opposite does not exist for given format.
		Format sRGB_opposite = Format::None;

	};
	FormatInfo GetFormatInfo(Format format);

	struct ImageDesc
	{
		Extent2D extent;
		Format format = Format::None;
		uint32_t mipLevels = 1; // The lowest number of mip levels an image can have is 1.
		uint32_t numFaces = 1; // The lowest number of faces an image can have is 1.
		bool isCubemap = false;

		DetailedResult Validate() const;
	};

	class Image
	{
	public:
		Image() = default;
		~Image() { Unload(); }

		Image& operator=(Image&) = delete;
		Image(Image&) = delete;

		Image& operator=(Image&&) noexcept; // Move assignment operator
		Image(Image&&) noexcept; // Move constructor

		// Calculates the total byte size of an image, including all miplevels and faces.
		static size_t CalculateTotalSize(Extent2D extent, Format format, uint32_t mipLevels, uint32_t numFaces);
		static size_t CalculateMipSize(Extent2D extent, Format format, uint32_t mipIndex);
		static uint32_t CalculateMipRowPitch(Extent2D extent, Format format, uint32_t mipIndex);
		static Extent2D CalculateMipExtent(Extent2D extent, uint32_t mipIndex);
		static uint32_t CalculateNumMips(Extent2D extent);

		void Unload();
		bool IsLoaded() const { return pixelsPtr != nullptr; }

		// Creates an image. All pixels are filled with an initial value of 0.
		bool Load(uint32_t width, uint32_t height, Format format);

		// Creates an image using more advanced parameters. All pixels are filled with an initial value of 0.
		bool Load(const ImageDesc& desc);

		// Loads an image from a file.
		bool LoadFromFile(const std::string& filename);

		// Loads an image from a file in memory (PNG, JPEG, GIF, BMP, DDS, etc...).
		// 
		// When 'guaranteeOwnership' is true:
		// - The image will always make an internal copy of the image data.
		// - Slightly slower due to memory copying, but always safe.
		// 
		// When 'guaranteeOwnership' is false:
		// - May directly reference 'fileData' without copying for better performance for DDS filetypes.
		// - WARNING: You must keep 'fileData' valid for the lifetime of this image.
		bool LoadFromMemory(const byte* fileData, size_t numBytes, bool guaranteeOwnership = true);

		// Creates a non-owning view of existing pixel memory.
		// 
		// OWNERSHIP AND LIFETIME:
		// - This image does NOT take ownership of the pixel data
		// - The provided 'pixels' memory MUST remain valid as long as this image exists
		// - No data is copied - the image operates directly on your memory
		// - You are responsible for freeing 'pixels' yourself
		// 
		// USAGE:
		// - Useful for zero-copy interaction with existing pixel buffers
		// - Allows reading/writing to your memory through image interface
		bool LoadAsPointer(const void* pixels, const ImageDesc& desc);

		// Saves image to file. Supported file extensions: PNG, BMP, TGA, JPG/JPEG, HDR.
		bool SaveToFile(const std::string& filename) const;

		const ImageDesc& GetDesc() const { return desc; }
		Extent2D GetExtent() const { return desc.extent; }
		uint32_t GetWidth() const { return desc.extent.width; }
		uint32_t GetHeight() const { return desc.extent.height; }
		Format GetFormat() const { return desc.format; }
		uint32_t GetMipLevels() const { return desc.mipLevels; }
		bool HasMipmaps() const { return desc.mipLevels > 1; }
		uint32_t GetNumFaces() const { return desc.numFaces; }
		bool IsCubemap() const { return desc.isCubemap; }

		// Returns true if using an sRGB format.
		bool IsSRGB() const;

		// Sets the format to an sRGB or non-sRGB equivalent to current format.
		// Not all formats support being sRGB. If the requested sRGB or non-sRGB format does not exist, no changes will be made.
		void SetSRGB(bool sRGB);

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

	private:
		ImageDesc desc;
		
		// Byte size of the entire image (including all faces and mipLevels).
		size_t size = 0; 

		// A pointer to where the pixel data begins.
		byte* pixelsPtr = nullptr;

		// If true, 'pixelsPtr' points to memory allocated by stb_image which must be freed using stb_image at some point.
		bool mustFreeSTBI = false;

		// A byte array containing the source of the pixel data (just pixel data or an entire DDS file).
		// Only used if this image owns the pixel data itself.
		std::vector<byte> ownedBuffer;

		static bool FileIsOfTypeDDS(const byte* fileData, size_t numBytes);
		bool LoadFromDDS(const byte* fileData, size_t numBytes, bool guaranteeOwnership);
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

	struct TextureDesc
	{
		Extent2D extent;
		Format format = Format::None;
		TextureUsage usage = TextureUsage::Default;
		MSAA msaa = MSAA::Disabled;
		uint32_t numFaces = 1;
		uint32_t mipLevels = 1;
		bool isCubemap = false;

		// Only relevant for render texture and depth buffer usage.
		ClearValue optimizedClearValue = ClearValue();

		// Optional. If set to Format::None, the texture's main format is used for the SRV.
		// It can be useful to use different formats for the SRV and RTV of a render texture,
		// like for example to avoid automatic sRGB conversions by using a non-sRGB RTV and an sRGB SRV.
		Format overrideSRVFormat = Format::None;

		// By default, descriptor(s) are created for textures that need them.
		// You can set this to false if you know you won't be needing any descriptors for this texture.
		bool createDescriptors = true;
	};

	struct WrappedTextureDesc
	{
		TextureDesc textureDesc;

		Descriptor srvDescriptor; // [Optional] Can be a null descriptor.
		Descriptor uavDescriptor; // [Optional] Can be a null descriptor.

		std::vector<void*> readMapped; // Per-frame if Readable; otherwise not used.

		// All vectors in 'impl' must be sized based on texture usage.
		// Tip: If the texture is not Readable, all vectors must have size 1.
		// The contents themselves are optional. For example, resource[0] or image[0] can be nullptr or VK_NULL_HANDLE.
		Impl_Texture impl;
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
		// It's OK to unload textures that are already unloaded (in that case nothing happens).
		void Unload();

		// If true, this texture has been created and can be used for things.
		bool IsLoaded() const { return isLoaded; }

		// Creates a texture. Replaces existing texture if already loaded.
		bool Load(const IGLOContext&, uint32_t width, uint32_t height, Format, TextureUsage,
			MSAA msaa = MSAA::Disabled, ClearValue optimizedClearValue = ClearValue());

		// Creates a texture using more advanced parameters.
		bool Load(const IGLOContext&, const TextureDesc&);

		// Loads a texture from file. Returns true if success.
		// If 'generateMipmaps' is true, mipmaps will be generated if the image does not already have them.
		bool LoadFromFile(const IGLOContext&, CommandList&, const std::string& filename, bool generateMipmaps = true, bool sRGB = false);

		// Loads a texture from a file in memory (DDS,PNG,JPG,GIF etc...). Returns true if success.
		// If 'generateMipmaps' is true, mipmaps will be generated if the image does not already have them.
		bool LoadFromMemory(const IGLOContext&, CommandList&, const byte* fileData, size_t numBytes, bool generateMipmaps = true, bool sRGB = false);

		// Loads a texture from image data in memory. Returns true if success.
		// If 'generateMipmaps' is true, mipmaps will be generated if the image does not already have them.
		bool LoadFromMemory(const IGLOContext&, CommandList&, const Image& image, bool generateMipmaps = true);

		// Creates a non-owning wrapper for existing graphics API resources and descriptors.
		// NOTE: This texture has no ownership over its resources/descriptors, and will not free them when unloading.
		// You must ensure the given resources/descriptors are valid during the lifetime of this texture.
		// You are responsible for freeing the given resources/descriptors yourself.
		bool LoadAsWrapped(const IGLOContext&, const WrappedTextureDesc&);

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

		const TextureDesc& GetDesc() const { return desc; }
		Extent2D GetExtent() const { return desc.extent; }
		uint32_t GetWidth() const { return desc.extent.width; }
		uint32_t GetHeight() const { return desc.extent.height; }
		Format GetFormat() const { return desc.format; }
		TextureUsage GetUsage() const { return desc.usage; }
		MSAA GetMSAA() const { return desc.msaa; }
		uint32_t GetNumFaces() const { return desc.numFaces; }
		uint32_t GetMipLevels() const { return desc.mipLevels; }
		bool IsCubemap() const { return desc.isCubemap; }
		ClearValue GetOptimiziedClearValue() const { return desc.optimizedClearValue; }

		// Gets the SRV descriptor for this texture, if it has one.
		// NOTE: These GetDescriptor() functions will return nullptr if no such descriptor exists. This is to
		// protect you from accidentally passing an invalid heap index to the shader and crashing the GPU.
		const Descriptor* GetDescriptor() const;

		// Gets the UAV descriptor for this texture, if it has one.
		const Descriptor* GetUnorderedAccessDescriptor() const;

#ifdef IGLO_D3D12
		ID3D12Resource* GetD3D12Resource() const;
		const D3D12_RENDER_TARGET_VIEW_DESC& GetD3D12Desc_RTV() const;
		const D3D12_DEPTH_STENCIL_VIEW_DESC& GetD3D12Desc_DSV() const;
		const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetD3D12Desc_UAV() const;

		static D3D12_RENDER_TARGET_VIEW_DESC GenerateD3D12Desc_RTV(Format, MSAA, uint32_t numFaces);
		static D3D12_DEPTH_STENCIL_VIEW_DESC GenerateD3D12Desc_DSV(Format, MSAA, uint32_t numFaces);
		static D3D12_UNORDERED_ACCESS_VIEW_DESC GenerateD3D12Desc_UAV(Format, MSAA, uint32_t numFaces);
#endif
#ifdef IGLO_VULKAN
		VkImage GetVulkanImage() const;
		VkDeviceMemory GetVulkanMemory() const;
		VkImageView GetVulkanImageView_SRV() const { return impl.imageView_SRV; }
		VkImageView GetVulkanImageView_UAV() const { return impl.imageView_UAV; }
		VkImageView GetVulkanImageView_RTV_DSV() const { return impl.imageView_RTV_DSV; }
#endif

	private:
		bool isLoaded = false;
		const IGLOContext* context = nullptr;
		TextureDesc desc;
		Descriptor srvDescriptor;
		Descriptor uavDescriptor;
		std::vector<void*> readMapped; // Per-frame if Readable; otherwise not used.
		bool isWrapped = false;

		Impl_Texture impl;

		void Impl_Unload();
		DetailedResult Impl_Load(const IGLOContext&, const TextureDesc&);
		DetailedResult GenerateMips(CommandList& cmd, const Image& image);
		static DetailedResult ValidateMipGeneration(CommandListType, const Image& image);
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
		// NOTE: The order at which you update and bind the dynamic buffer matters! Correct order: Update->Bind. Wrong order: Bind->Update.
		// This is because after calling SetDynamicData(), you will get a different descriptor next time you call GetDescriptor().
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

		// Stride is in bytes.
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
		const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetD3D12Desc_UAV() const;
		static D3D12_UNORDERED_ACCESS_VIEW_DESC GenerateD3D12Desc_UAV_Structured(uint32_t numElements, uint32_t stride);
		static D3D12_UNORDERED_ACCESS_VIEW_DESC GenerateD3D12Desc_UAV_Raw(uint64_t size);
#endif
#ifdef IGLO_VULKAN
		VkBuffer GetVulkanBuffer() const;
		VkDeviceMemory GetVulkanMemory() const;
#endif

	private:
		bool isLoaded = false;
		const IGLOContext* context = nullptr;
		BufferType type = BufferType::VertexBuffer;
		BufferUsage usage = BufferUsage::Default;
		uint64_t size = 0; // total size in bytes
		uint32_t stride = 0; // stride in bytes
		uint32_t numElements = 0;
		std::vector<Descriptor> descriptor_SRV_or_CBV; // Per-frame if Dynamic.
		Descriptor descriptor_UAV;
		uint32_t dynamicSetCounter = 0; // Increases by 1 when setting dynamic data. Capped at number of frames in flight.
		std::vector<void*> mapped;

		Impl_Buffer impl;

		void Impl_Unload();
		bool InternalLoad(const IGLOContext&, uint64_t size, uint32_t stride, uint32_t numElements, BufferUsage, BufferType);
		DetailedResult Impl_InternalLoad(const IGLOContext&, uint64_t size, uint32_t stride, uint32_t numElements, BufferUsage, BufferType);
	};

	enum class InputClass
	{
		PerVertex = 0,
		PerInstance
	};

	struct VertexElement
	{
		VertexElement() = default;

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

		Format format = Format::FLOAT;
		const char* semanticName = "";
		uint32_t semanticIndex = 0;
		uint32_t inputSlot = 0;
		InputClass inputSlotClass = InputClass::PerVertex;
		uint32_t instanceDataStepRate = 0;
	};

	struct Shader
	{
		Shader() = default;

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

		const byte* shaderBytecode = nullptr;
		size_t bytecodeLength = 0;
		std::string entryPointName;
	};

	struct RenderTargetDesc
	{
		std::vector<Format> colorFormats;
		Format depthFormat = Format::None;
		MSAA msaa = MSAA::Disabled;
	};

	struct PipelineDesc
	{
		Shader VS; // Vertex shader
		Shader PS; // Pixel shader
		Shader DS; // Domain shader
		Shader HS; // Hull shader
		Shader GS; // Geometry shader

		// You must provide one blend state for each render texture you expect to render onto.
		std::vector<BlendDesc> blendStates;
		RasterizerDesc rasterizerState;
		DepthDesc depthState;
		std::vector<VertexElement> vertexLayout;

		uint32_t sampleMask = IGLO_UINT32_MAX;
		bool enableAlphaToCoverage = false;
		bool blendLogicOpEnabled = false;
		LogicOp blendLogicOp = LogicOp::NoOperation;

		PrimitiveTopology primitiveTopology = PrimitiveTopology::Undefined;

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
		bool LoadFromFile(const IGLOContext&, const std::string& filepathVS, const std::string& entryPointNameVS,
			const std::string filepathPS, const std::string& entryPointNamePS, const RenderTargetDesc&,
			const std::vector<VertexElement>&, PrimitiveTopology, DepthDesc, RasterizerDesc, const std::vector<BlendDesc>&);

		// A helper function that creates a graphics pipeline state object in a single line of code.
		// You must provide one blend state (BlendDesc) for each expected render texture.
		// Returns true if success.
		bool Load(const IGLOContext&, const Shader& VS, const Shader& PS, const RenderTargetDesc&,
			const std::vector<VertexElement>&, PrimitiveTopology, DepthDesc, RasterizerDesc, const std::vector<BlendDesc>&);

		// Creates a compute pipeline state object.
		bool LoadAsCompute(const IGLOContext&, const Shader& CS);

		bool IsComputePipeline() const { return isComputePipeline; }

		PrimitiveTopology GetPrimitiveTopology() const { return primitiveTopology; }

#ifdef IGLO_D3D12
		ID3D12PipelineState* GetD3D12PipelineState() const { return impl.pipeline.Get(); }
#endif
#ifdef IGLO_VULKAN
		VkPipeline GetVulkanPipeline() const { return impl.pipeline; }
#endif

	private:
		bool isLoaded = false;
		const IGLOContext* context = nullptr;
		bool isComputePipeline = false;
		PrimitiveTopology primitiveTopology = PrimitiveTopology::Undefined;

		Impl_Pipeline impl;

		void Impl_Unload();
		DetailedResult Impl_Load(const IGLOContext&, const PipelineDesc&);
		DetailedResult Impl_LoadAsCompute(const IGLOContext& context, const Shader& CS);
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

		DetailedResult Load(const IGLOContext&, uint32_t maxPersistentResources, uint32_t maxTempResourcesPerFrame,
			uint32_t maxSamplers, uint32_t numFramesInFlight);

		void NextFrame();

		// Frees all temporary resource descriptors
		void FreeAllTempResources();

		[[nodiscard]] Descriptor AllocatePersistent(DescriptorType);
		Descriptor AllocateTemp(DescriptorType);
		void FreePersistent(Descriptor);

		struct Stats
		{
			uint32_t maxPersistentResources = 0;
			uint32_t maxTempResources = 0;
			uint32_t maxSamplers = 0;
			uint32_t livePersistentResources = 0;
			uint32_t liveTempResources = 0;
			uint32_t liveSamplers = 0;

			std::string ToString() const
			{
				return ig::ToString
				(
					"---- Descriptor stats (Live/Max) ----\n",
					"Persistent Resources: ", livePersistentResources, "/", maxPersistentResources, "\n",
					"Temp Resources: ", liveTempResources, "/", maxTempResources, "\n"
					"Samplers: ", liveSamplers, "/", maxSamplers, "\n"
				);
			}
		};

		Stats GetStats() const;
		std::string GetStatsString() const;

#ifdef IGLO_D3D12

		// These handles belong to small CPU-only descriptor heaps.
		D3D12_CPU_DESCRIPTOR_HANDLE GetD3D12CPUHandle_NonShaderVisible_RTV() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetD3D12CPUHandle_NonShaderVisible_DSV() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetD3D12CPUHandle_NonShaderVisible_UAV() const;

		// These handles belong to shader visible descriptor heaps.
		D3D12_CPU_DESCRIPTOR_HANDLE GetD3D12CPUHandle(Descriptor) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetD3D12GPUHandle(Descriptor) const;

		ID3D12DescriptorHeap* GetD3D12DescriptorHeap_Resources() const { return impl.descriptorHeap_Resources.Get(); }
		ID3D12DescriptorHeap* GetD3D12DescriptorHeap_Samplers() const { return impl.descriptorHeap_Samplers.Get(); }

		ID3D12RootSignature* GetD3D12BindlessRootSignature() const { return impl.bindlessRootSignature.Get(); }

		uint32_t GetD3D12DescriptorSize_RTV() const { return impl.descriptorSize_RTV; }
		uint32_t GetD3D12DescriptorSize_DSV() const { return impl.descriptorSize_DSV; }
		uint32_t GetD3D12DescriptorSize_Sampler() const { return impl.descriptorSize_Sampler; }
		uint32_t GetD3D12DescriptorSize_CBV_SRV_UAV() const { return impl.descriptorSize_CBV_SRV_UAV; }
#endif
#ifdef IGLO_VULKAN

		// Must map directly to 'DescriptorType'
		inline static constexpr VkDescriptorType vkDescriptorTypeTable[] =
		{
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			VK_DESCRIPTOR_TYPE_SAMPLER
		};
		static_assert(std::size(vkDescriptorTypeTable) == NUM_DESCRIPTOR_TYPES,
			"vkDescriptorTypeTable size must match NUM_DESCRIPTOR_TYPES.");

		// Must map directly to 'DescriptorType'
		inline static constexpr uint32_t vkDescriptorBindingTable[] =
		{
			0,
			0,
			0,
			0,
			1
		};
		static_assert(std::size(vkDescriptorBindingTable) == NUM_DESCRIPTOR_TYPES,
			"vkDescriptorBindingTable size must match NUM_DESCRIPTOR_TYPES.");

		// Must map directly to 'DescriptorType'
		static std::array<uint32_t, NUM_DESCRIPTOR_TYPES> GetVulkanPropsMaxDescriptors(VkPhysicalDevice);

		VkDescriptorSetLayout GetVulkanDescriptorSetLayout() const { return impl.descriptorSetLayout; }
		VkDescriptorSet GetVulkanDescriptorSet() const { return impl.descriptorSet; }
		VkPipelineLayout GetVulkanBindlessPipelineLayout() const { return impl.bindlessPipelineLayout; }

		void WriteBufferDescriptor(Descriptor descriptor, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
		void WriteImageDescriptor(Descriptor descriptor, VkImageView imageView, VkImageLayout imageLayout);
		void WriteSamplerDescriptor(Descriptor descriptor, VkSampler sampler);

		// Creates a temporary VkImageView that is valid for 1 frame.
		// NOTE: Don't destroy this VkImageView yourself!
		VkResult CreateTempVulkanImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo,
			const VkAllocationCallbacks* pAllocator, VkImageView* pView);

#endif

	private:
		class PersistentIndexAllocator
		{
		public:
			PersistentIndexAllocator() = default;

			void Reset(uint32_t maxIndices);
			void Clear();
			std::optional<uint32_t> Allocate(); // Returns nothing if failed 
			void Free(uint32_t index);
			uint32_t GetAllocationCount() const { return allocationCount; }
			uint32_t GetMaxIndices() const { return maxIndices; }
		private:
			uint32_t maxIndices = 0;
			uint32_t allocationCount = 0;
			std::vector<uint32_t> freed; // Grows every time you free an index. Shrinks when you allocate them back.
#ifdef _DEBUG
			PackedBoolArray isAllocated;
#endif
		};

		class TempIndexAllocator
		{
		public:
			TempIndexAllocator() = default;

			void Reset(uint32_t maxIndices, uint32_t offset);
			void Clear();
			std::optional<uint32_t> Allocate(); // Returns nothing if failed 
			void FreeAllIndices();
			uint32_t GetAllocationCount() const { return allocationCount; }
			uint32_t GetMaxIndices() const { return maxIndices; }
		private:
			uint32_t maxIndices = 0;
			uint32_t allocationCount = 0;
			uint32_t offset = 0;
		};

	private:
		bool isLoaded = false;
		const IGLOContext* context = nullptr;
		uint32_t frameIndex = 0;
		uint32_t numFrames = 0;
		PersistentIndexAllocator persResourceIndices;
		PersistentIndexAllocator persSamplerIndices;
		std::vector<TempIndexAllocator> tempResourceIndices; // Per-frame

		Impl_DescriptorHeap impl;

		bool LogErrorIfNotLoaded() const;
		void Impl_Unload();
		DetailedResult Impl_Load(const IGLOContext&, uint32_t maxPersistentResources, uint32_t maxTempResourcesPerFrame,
			uint32_t maxSamplers, uint32_t numFramesInFlight);
		void Impl_NextFrame();
		void Impl_FreeAllTempResources();
		static uint32_t CalcTotalResDescriptors(uint32_t maxPersistent, uint32_t maxTempPerFrame, uint32_t numFramesInFlight);
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

		uint64_t fenceValue = IGLO_UINT64_MAX;

		Impl_Receipt impl;
	};

	// Manages the command queues
	class CommandQueue
	{
	public:
		CommandQueue() = default;
		~CommandQueue() { Unload(); }

		CommandQueue& operator=(CommandQueue&) = delete;
		CommandQueue(CommandQueue&) = delete;

		void Unload();
		bool IsLoaded() const { return isLoaded; }

		DetailedResult Load(const IGLOContext&, uint32_t numFramesInFlight, uint32_t numBackBuffers);

		Receipt SubmitCommands(const CommandList& commandList);
		Receipt SubmitCommands(const CommandList* const* commandLists, uint32_t numCommandLists);

		bool IsComplete(Receipt receipt) const;
		bool IsIdle() const;
		void WaitForCompletion(Receipt receipt);
		void WaitForIdle();
		Receipt SubmitSignal(CommandListType type);

#ifdef IGLO_D3D12
		ID3D12CommandQueue* GetD3D12CommandQueue(CommandListType type) const { return impl.commandQueues[(uint32_t)type].Get(); }
#endif
#ifdef IGLO_VULKAN
		void RecreateSwapChainSemaphores(uint32_t numFramesInFlight, uint32_t numBackBuffers);
		void DestroySwapChainSemaphores();
		Receipt SubmitBinaryWaitSignal(VkSemaphore binarySemaphore);
		VkResult Present(VkSwapchainKHR swapChain);
		VkResult AcquireNextVulkanSwapChainImage(VkDevice device, VkSwapchainKHR swapChain, uint64_t timeout);
		uint32_t GetCurrentBackBufferIndex() { return impl.currentBackBufferIndex; }
		VkQueue GetVulkanQueue(CommandListType type) const { return impl.queues[(uint32_t)type]; }
		VkQueue GetVulkanPresentQueue() const { return impl.presentQueue; }
		uint32_t GetVulkanQueueFamilyIndex(CommandListType type) const { return impl.queueFamIndices[(uint32_t)type]; }
#endif
	private:
		bool isLoaded = false;
		const IGLOContext* context = nullptr;

		Impl_CommandQueue impl;

		void Impl_Unload();
		DetailedResult Impl_Load(const IGLOContext&, uint32_t numFramesInFlight, uint32_t numBackBuffers);
		Receipt Impl_SubmitCommands(const CommandList* const* commandLists, uint32_t numCommandLists, CommandListType cmdType);
		bool Impl_IsComplete(Receipt receipt) const;
		void Impl_WaitForCompletion(Receipt receipt);

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
		None = VK_PIPELINE_STAGE_2_NONE,
		All = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		Draw = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, //TODO: is this correct?
		IndexInput = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
		VertexShading = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
		PixelShading = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		DepthStencil = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
		RenderTarget = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		ComputeShading = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		Raytracing = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
		Copy = VK_PIPELINE_STAGE_2_COPY_BIT,
		Resolve = VK_PIPELINE_STAGE_2_RESOLVE_BIT,
		ExecuteIndirect = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, //TODO: is this correct?
		Predication = VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT,
		AllShading = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		NonPixelShading = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
		VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT |
		VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT,
		EmitRaytracingAccelerationStructurePostbuildInfo = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		ClearUnorderedAccessView = VK_PIPELINE_STAGE_2_CLEAR_BIT,
		VideoDecode = VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR,
		//VideoProcess = ,
		VideoEncode = VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR,
		BuildRaytracingAccelerationStructure = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		//CopyRaytracingAccelerationStructure = ,
		//SyncSplit = ,
#endif
	};

	enum class BarrierAccess : uint64_t
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
		Common = VK_ACCESS_2_NONE,
		VertexBuffer = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
		ConstantBuffer = VK_ACCESS_2_UNIFORM_READ_BIT,
		IndexBuffer = VK_ACCESS_2_INDEX_READ_BIT,
		RenderTarget = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		UnorderedAccess = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
		DepthStencilWrite = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		DepthStencilRead = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
		ShaderResource = VK_ACCESS_2_SHADER_READ_BIT,
		StreamOutput = VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT,
		IndirectArgument = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
		Predication = VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT,
		CopyDest = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		CopySource = VK_ACCESS_2_TRANSFER_READ_BIT,
		ResolveDest = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		ResolveSource = VK_ACCESS_2_TRANSFER_READ_BIT,
		RaytracingAccelerationStructureRead = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR,
		RaytracingAccelerationStructureWrite = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
		ShadingRateSource = VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR,
		VideoDecodeRead = VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR,
		VideoDecodeWrite = VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR,
		//VideoProcessRead = ,
		//VideoProcessWrite = ,
		VideoEncodeRead = VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR,
		VideoEncodeWrite = VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR,
		NoAccess = VK_ACCESS_2_NONE,
#endif
	};

	enum class BarrierLayout : uint64_t
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
		Undefined = VK_IMAGE_LAYOUT_UNDEFINED,
		Common = VK_IMAGE_LAYOUT_GENERAL,
		Present = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		GenericRead = VK_IMAGE_LAYOUT_GENERAL,
		RenderTarget = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		UnorderedAccess = VK_IMAGE_LAYOUT_GENERAL,
		DepthStencilWrite = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		DepthStencilRead = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		ShaderResource = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		CopySource = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		CopyDest = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		ResolveSource = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		ResolveDest = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		ShadingRateSource = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR,

		// Video
		VideoDecodeRead = VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR,
		VideoDecodeWrite = VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR,
		//VideoProcessRead = ,
		//VideoProcessWrite = ,
		VideoEncodeRead = VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR,
		VideoEncodeWrite = VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR,

		// Graphics queue (AKA direct queue)
		_GraphicsQueue_Common = Common,
		_GraphicsQueue_GenericRead = GenericRead,
		_GraphicsQueue_UnorderedAccess = UnorderedAccess,
		_GraphicsQueue_ShaderResource = ShaderResource,
		_GraphicsQueue_CopySource = CopySource,
		_GraphicsQueue_CopyDest = CopyDest,

		// Compute queue
		_ComputeQueue_Common = Common,
		_ComputeQueue_GenericRead = GenericRead,
		_ComputeQueue_UnorderedAccess = UnorderedAccess,
		_ComputeQueue_ShaderResource = ShaderResource,
		_ComputeQueue_CopySource = CopySource,
		_ComputeQueue_CopyDest = CopyDest,

		// Video queue
		_VideoQueue_Common = VK_IMAGE_LAYOUT_GENERAL,
#endif
	};

	// An abstraction that simplifies barriers by combining sync, access, and layout into a single enum.
	// Contains only the most commonly used barriers.
	enum class SimpleBarrier
	{
		Common = 0, // Uses queue specific layouts.
		Present, // For presenting the back buffer. Does not use queue specific layouts.
		Discard,
		PixelShaderResource,
		ComputeShaderResource,
		ComputeShaderUnorderedAccess,
		RenderTarget,
		DepthWrite,
		DepthRead,
		CopySource,
		CopyDest,
		ResolveSource,
		ResolveDest,
		ClearUnorderedAccess,
	};
	struct SimpleBarrierInfo
	{
		BarrierSync sync = BarrierSync::None;
		BarrierAccess access = BarrierAccess::Common;
		BarrierLayout layout = BarrierLayout::Undefined;
		bool discard = false;
	};
	SimpleBarrierInfo GetSimpleBarrierInfo(SimpleBarrier simpleBarrier, CommandListType queueType);

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
		void AddTextureBarrier(const Texture& texture, SimpleBarrier before, SimpleBarrier after);
		void AddTextureBarrierAtSubresource(const Texture& texture, BarrierSync syncBefore, BarrierSync syncAfter,
			BarrierAccess accessBefore, BarrierAccess accessAfter, BarrierLayout layoutBefore, BarrierLayout layoutAfter,
			uint32_t faceIndex, uint32_t mipIndex, bool discard = false);
		void AddTextureBarrierAtSubresource(const Texture& texture, SimpleBarrier before, SimpleBarrier after, uint32_t faceIndex, uint32_t mipIndex);
		void AddBufferBarrier(const Buffer& buffer, BarrierSync syncBefore, BarrierSync syncAfter, BarrierAccess accessBefore, BarrierAccess accessAfter);
		void FlushBarriers();

		void SetPipeline(const Pipeline&);

		// Sets the render target(s) for subsequent rendering.
		// It is valid to pass nullptr for 'renderTexture' if rendering only to a depth buffer.
		// If 'optimizedClear' is true, all bound color and depth attachments are cleared immediately when bound.
		// The optimized clear values are chosen at texture creation time.
		void SetRenderTarget(const Texture* renderTexture, const Texture* depthBuffer = nullptr, bool optimizedClear = false);
		void SetRenderTargets(const Texture* const* renderTextures, uint32_t numRenderTextures, const Texture* depthBuffer, bool optimizedClear = false);

		// Clears a render texture
		void ClearColor(const Texture& renderTexture, Color color = Colors::Black, uint32_t numRects = 0, const IntRect* rects = nullptr);
		// Clears a depth buffer
		void ClearDepth(const Texture& depthBuffer, float depth = 1.0f, byte stencil = 255, bool clearDepth = true, bool clearStencil = true,
			uint32_t numRects = 0, const IntRect* rects = nullptr);

		void ClearUnorderedAccessBufferUInt32(const Buffer& buffer, const uint32_t value);
		void ClearUnorderedAccessTextureFloat(const Texture& texture, const float values[4]);
		void ClearUnorderedAccessTextureUInt32(const Texture& texture, const uint32_t values[4]);

		// Sets graphics push constants, which are directly accessible to the shader.
		// 'sizeInBytes' and 'destOffsetInBytes' must be multiples of 4.
		// NOTE: For compute shader pipelines, call 'SetComputePushConstants' instead!
		void SetPushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes = 0);
		void SetComputePushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes = 0);

		void SetVertexBuffer(const Buffer& vertexBuffer, uint32_t slot = 0);
		void SetVertexBuffers(const Buffer* const* vertexBuffers, uint32_t count, uint32_t startSlot = 0);
		void SetTempVertexBuffer(const void* data, uint64_t sizeInBytes, uint32_t vertexStride, uint32_t slot = 0);
		void SetIndexBuffer(const Buffer& indexBuffer);

		void Draw(uint32_t vertexCount, uint32_t startVertexLocation = 0);
		void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation = 0, uint32_t startInstanceLocation = 0);
		void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation = 0, int32_t baseVertexLocation = 0);
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
		ID3D12GraphicsCommandList7* GetD3D12GraphicsCommandList() const { return impl.graphicsCommandList.Get(); }
#endif
#ifdef IGLO_VULKAN
		VkCommandBuffer GetVulkanCommandBuffer() const { return impl.commandBuffer[frameIndex]; }
#endif

	private:
		bool isLoaded = false;
		const IGLOContext* context = nullptr;
		uint32_t numFrames = 0;
		uint32_t frameIndex = 0;
		CommandListType commandListType = CommandListType::Graphics;

		Impl_CommandList impl;

#ifdef IGLO_VULKAN
		void SafeEndRendering();
		void SafePauseRendering(); // MUST be followed by a call to SafeResumeRendering() in the same function.
		void SafeResumeRendering(); // MUST be called shortly after SafePauseRendering() in the same function.
#endif

		void Impl_Unload();
		DetailedResult Impl_Load(const IGLOContext&, CommandListType);
		void Impl_Begin();
		void Impl_End();
		void Impl_SetRenderTargets(const Texture* const* renderTextures, uint32_t numRenderTextures,
			const Texture* depthBuffer, bool optimizedClear);
		void Impl_ClearColor(const Texture& renderTexture, Color color, uint32_t numRects, const IntRect* rects);
		void Impl_ClearDepth(const Texture& depthBuffer, float depth, byte stencil, bool clearDepth, bool clearStencil,
			uint32_t numRects, const IntRect* rects);
		void Impl_ClearUnorderedAccessBufferUInt32(const Buffer& buffer, const uint32_t value);
		void Impl_ClearUnorderedAccessTextureFloat(const Texture& texture, const float values[4]);
		void Impl_ClearUnorderedAccessTextureUInt32(const Texture& texture, const uint32_t values[4]);
		void Impl_SetPushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes);
		void Impl_SetComputePushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes);
		void Impl_SetVertexBuffers(const Buffer* const* vertexBuffers, uint32_t count, uint32_t startSlot);
		void Impl_CopyTexture(const Texture& source, const Texture& destination);
		void Impl_CopyTextureSubresource(const Texture& source, uint32_t sourceFaceIndex, uint32_t sourceMipIndex,
			const Texture& destination, uint32_t destFaceIndex, uint32_t destMipIndex);
		void Impl_CopyTextureToReadableTexture(const Texture& source, const Texture& destination);

		void CopyTextureToReadableTexture(const Texture& source, const Texture& destination);
		static DetailedResult ValidatePushConstants(const void* data, uint32_t sizeInBytes, uint32_t destOffsetInBytes);
	};

	struct TempBuffer
	{
		bool IsNull() const { return (data == nullptr); }

		uint64_t offset = 0;
		void* data = nullptr;

		Impl_TempBuffer impl;
	};

	class TempBufferAllocator
	{
	public:
		TempBufferAllocator() = default;
		~TempBufferAllocator() { Unload(); }

		TempBufferAllocator& operator=(TempBufferAllocator&) = delete;
		TempBufferAllocator(TempBufferAllocator&) = delete;

		void Unload();
		bool IsLoaded() const { return isLoaded; }
		DetailedResult Load(const IGLOContext&, uint64_t linearPageSize, uint32_t numFramesInFlight);

		// Must be called once per frame. Rolls to next frameIndex, frees that frame's temp pages.
		void NextFrame();
		void FreeAllTempPages();

		// Sub‐allocate a temporary chunk inside an existing page, or from a new page if there's no room left.
		TempBuffer AllocateTempBuffer(uint64_t sizeInBytes, uint32_t alignment);

		// How many VkBuffers / D3D12Resources are currently allocated in this frame (persistent + temp)
		size_t GetNumLivePages() const;

		// Gets the byte size of a linear page.
		uint64_t GetLinearPageSize() const { return linearPageSize; }

	private:
		struct Page
		{
		public:
			bool IsNull() const { return (mapped == nullptr); }
			void Free(const IGLOContext&);

			// Creates a new page. Returns a null page if failed.
			static Page Create(const IGLOContext&, uint64_t sizeInBytes);

			void* mapped = nullptr;

			Impl_BufferAllocatorPage impl;

		private:
			void Impl_Free(const IGLOContext&);
		};

		struct PerFrame
		{
			std::vector<Page> largePages; // Each page here is larger than pageSize and has a unique size. All temporary.
			std::vector<Page> linearPages; // First pages are persistent, rest are temporary.
			uint64_t linearNextByte = 0; // For the current linear page
		};

	private:
		bool isLoaded = false;
		const IGLOContext* context = nullptr;
		uint32_t frameIndex = 0;
		uint32_t numFrames = 0;
		uint64_t linearPageSize = 0;
		std::vector<PerFrame> perFrame; // One for each frame

		bool LogErrorIfNotLoaded();
		static constexpr size_t numPersistentPages = 1;
		static void FreeTempPagesAtFrame(const IGLOContext&, PerFrame& perFrame);
	};

	struct Viewport
	{
		Viewport() = default;

		Viewport(float x, float y, float width, float height, float minDepth = 0, float maxDepth = 1.0f)
		{
			this->x = x;
			this->y = y;
			this->width = width;
			this->height = height;
			this->minDepth = minDepth;
			this->maxDepth = maxDepth;
		}

		float x = 0;
		float y = 0;
		float width = 0;
		float height = 0;
		float minDepth = 0;
		float maxDepth = 0;
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
	std::string ConvertKeyToString(Key);

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
		std::string title = "iglo"; // Window title. The string is in utf8.
		uint32_t width = 640; // Window width.
		uint32_t height = 480; // Window height.
		bool resizable = false; // Whether the window is resizable.
		bool centered = false; // Whether the window should be positioned in the center of the screen.
		bool bordersVisible = true; // Whether the window should have its borders visible.
		bool visible = true; // Whether the window should be visible.
	};

	struct RenderSettings
	{
		PresentMode presentMode = PresentMode::Vsync;
		
		// Default backbuffer format is BGRA for X11 compatibility
		Format backBufferFormat = Format::BYTE_BYTE_BYTE_BYTE_BGRA; 

		uint32_t maxFramesInFlight = 2;
		uint32_t numBackBuffers = 3;

		uint64_t tempBufferAllocatorLinearPageSize = IGLO_MEGABYTE * 32;

		uint32_t maxPersistentResourceDescriptors = 16384;
		uint32_t maxTemporaryResourceDescriptorsPerFrame = 4096;
		uint32_t maxSamplerDescriptors = 512;
	};

	struct WindowState
	{
		IntPoint pos;
		Extent2D size;
		bool bordersVisible = false;
		bool resizable = false;
		bool visible = false;
	};

	struct WindowResizeConstraints
	{
		Extent2D minSize; // Set to 0 to disable.
		Extent2D maxSize; // Set to 0 to disable.

		//TODO: Implement aspect ratio resize constraints.
		uint32_t aspectRatioWidth = 0; // Set to 0 to disable.
		uint32_t aspectRatioHeight = 0; // Set to 0 to disable.
	};

	// Placement alignments of various resources that can be placed inside buffers, measured in bytes.
	struct BufferPlacementAlignments
	{
		// Structured buffers (not raw buffers) have no placement alignment constraints in D3D12, but they do in Vulkan.

		uint32_t vertexOrIndexBuffer = 0;
		uint32_t rawOrStructuredBuffer = 0;
		uint32_t constant = 0;
		uint32_t texture = 0;
		uint32_t textureRowPitch = 0;
	};

	struct GraphicsSpecs
	{
		std::string apiName;
		std::string rendererName;
		std::string vendorName;

		uint32_t maxTextureDimension = 0;

		BufferPlacementAlignments bufferPlacementAlignments;

		// In D3D12, all present modes provided by iglo are guaranteed to be supported.
		// In Vulkan, only the 'Vsync' present mode is guaranteed to be supported.
		SupportedPresentModes supportedPresentModes;
	};

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
		void SetModalLoopCallback(CallbackModalLoop callbackModalLoop);
		CallbackModalLoop GetModalLoopCallback() { return callbackModalLoop; }
		// Whether or not the main thread is currently inside the modal loop callback function call.
		// If this returns true, running a GetEvent() loop or MainLoop on the main thread will make the window unresponsive.
		bool IsInsideModalLoopCallback() const { return insideModalLoopCallback; }

		void SetOnDeviceRemovedCallback(CallbackOnDeviceRemoved callbackOnDeviceRemoved);

		// Gets the mouse position relative to the topleft corner of the window.
		IntPoint GetMousePosition() const { return mousePosition; }
		int32_t GetMouseX() const { return mousePosition.x; }
		int32_t GetMouseY() const { return mousePosition.y; }

		// Checks if a mouse button is currently held down
		bool IsMouseButtonDown(MouseButton button) const;

		// Checks if a keyboard key is currently down
		bool IsKeyDown(Key key) const;

		// Sets windowed mode visibility.
		void SetWindowVisible(bool visible);
		// Gets windowed mode visibility.
		bool GetWindowVisible() const { return windowedMode.visible; }
		// Whether or not the window is currently minimized. (Relevant for both windowed mode and borderless fullscreen mode)
		bool GetWindowMinimized() const { return window.minimized; }
		// Sets the window title.
		void SetWindowTitle(std::string title);
		// Gets the window title.
		const std::string& GetWindowTitle() const { return window.title; }
		// Sets windowed mode client size.
		void SetWindowSize(uint32_t width, uint32_t height);
		// Gets windowed mode client size.
		Extent2D GetWindowSize() const { return windowedMode.size; }
		// Sets windowed mode window position.
		void SetWindowPosition(int32_t x, int32_t y);
		// Gets windowed mode window position.
		IntPoint GetWindowPosition() const { return windowedMode.pos; }
		// Sets windowed mode window resizability.
		void SetWindowResizable(bool resizable);
		// Gets windowed mode window resizability.
		bool GetWindowResizable() { return windowedMode.resizable; }
		// Sets windowed mode window border visibility.
		void SetWindowBordersVisible(bool visible);
		// Gets windowed mode window border visibility.
		bool GetWindowBordersVisible() const { return windowedMode.bordersVisible; }
		void SetWindowResizeConstraints(WindowResizeConstraints constraints) { windowResizeConstraints = constraints; }
		WindowResizeConstraints GetWindowResizeConstraints() const { return windowResizeConstraints; }
		// Gets the remembered windowed mode state.
		WindowState GetWindowedModeState() const { return windowedMode; }

		// Centers the window (for windowed mode).
		void CenterWindow();
		// If enabled, files can be drag-and-dropped onto the window.
		void EnableDragAndDrop(bool enable);
		bool GetDragAndDropEnabled() { return window.enableDragAndDrop; }

		void SetWindowIconFromImage(const Image& icon);
#ifdef _WIN32
		// Sets the window icon from a resource defined in 'resource.h'. Example usage: SetWindowIconFromResource(IDI_ICON1).
		// This function is only available on the Windows platform.
		void SetWindowIconFromResource(int icon);

		HWND GetWindowHWND() const { return window.hwnd; }

		// Allows you to handle WndProc messages before iglo handles them.
		// This is needed for when you use imgui with iglo.
		void SetWndProcHookCallback(CallbackWndProcHook callback);
#endif
#ifdef __linux__
		Display* GetX11WindowDisplay() const { return window.display; }
		Window GetX11WindowHandle() const { return window.handle; }

		void SetX11EventHookCallback(CallbackX11EventHook callback);
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
		void EnterWindowedMode(uint32_t width, uint32_t height);

		// Enters borderless fullscreen mode.
		// The backbuffer extent will match the active desktop resolution.
		// If you want to use a different resolution than the desktop resolution while in fullscreen,
		// you should render your scene onto a rendertarget with whatever resolution you want,
		// then draw that render target texture onto the backbuffer using ScreenRenderer.
		void EnterFullscreenMode();

		DisplayMode GetDisplayMode() const { return displayMode; }

		//------------------ Graphics ------------------//

		const GraphicsSpecs& GetGraphicsSpecs() const { return graphicsSpecs; }

		// NOTE: In Vulkan, changing the present mode requires destroying and recreating the swapchain,
		// which is an expensive operation.
		void SetPresentMode(PresentMode presentMode);
		PresentMode GetPresentMode() const { return swapChain.presentMode; }

		// NOTE: Changing the number of backbuffers and/or frames in flight requires recreating the swapchain,
		// which is an expensive operation.
		void SetFrameBuffering(uint32_t numFramesInFlight, uint32_t numBackBuffers);

		// NOTE: Changing the backbuffer format requires recreating the swapchain, which is an expensive operation.
		void SetBackBufferFormat(Format format);

		// Sends a command to the GPU to present the backbuffer to the screen.
		// This begins a new frame.
		// The CPU will wait a bit here if the GPU is too far behind.
		void Present();

		// Submits commands to the GPU
		Receipt Submit(const CommandList& commandList);
		// Submits commands to the GPU from multiple command lists
		Receipt Submit(const CommandList* const* commandLists, uint32_t numCommandLists);

		// Returns true if the commands associated with the given receipt has finished executing.
		// Null receipts are legal to pass to this function, they will be treated as completed.
		bool IsComplete(Receipt receipt) const;

		// The CPU will wait for the commands associated with the given receipt to finish executing.
		// Null receipts are legal to pass to this function, they will be treated as completed.
		void WaitForCompletion(Receipt receipt);

		// Waits for the graphics device to finish executing all commands.
		void WaitForIdleDevice();

		// Manages delayed safe unloading of textures that might still be in use by the GPU.
		// 
		// Problem:
		// - Unloading a texture mid-frame (e.g., replacing a font atlas texture that has outgrown its size)
		//   can cause GPU instability if the texture is still referenced in a previous frame's draw calls.
		//   The GPU may not have finished processing commands that depend on the texture.
		//
		// Solution:
		// - Instead of calling `texture->Unload()` directly, pass ownership to IGLOContext via this function
		//   using a `shared_ptr<Texture>`. IGLOContext will retain the shared pointer until the GPU is guaranteed
		//   to have finished all pending work referencing the texture.
		// - When the last shared pointer to the texture is released, `Texture::Unload()` is automatically called
		//   in the destructor, freeing GPU resources safely.
		//
		// Usage:
		// - Call this ONLY for textures that need mid-frame replacement (e.g., dynamic font atlases).
		// - For most textures (loaded/unloaded during scene transitions), this is unnecessary.
		//
		// Warning:
		// - NEVER call `texture->Unload()` manually for textures that you manage with this function.
		void DelayedTextureUnload(std::shared_ptr<Texture> texture) const;

		Descriptor CreateTempConstant(const void* data, uint64_t numBytes) const;
		Descriptor CreateTempStructuredBuffer(const void* data, uint32_t elementStride, uint32_t numElements) const;
		Descriptor CreateTempRawBuffer(const void* data, uint64_t numBytes) const;

		uint32_t GetMaxFramesInFlight() const { return maxFramesInFlight; }
		uint32_t GetNumFramesInFlight() const { return numFramesInFlight; }
		uint32_t GetNumBackBuffers() const { return swapChain.numBackBuffers; }
		uint32_t GetFrameIndex() const { return frameIndex; }

		// Gets the width of the backbuffer.
		uint32_t GetWidth() const { return swapChain.extent.width; }
		// Gets the height of the backbuffer.
		uint32_t GetHeight() const { return swapChain.extent.height; }
		// Gets the width and height of the backbuffer.
		Extent2D GetBackBufferExtent() const { return swapChain.extent; }
		const RenderTargetDesc& GetBackBufferRenderTargetDesc(bool get_opposite_sRGB_view = false) const;

		// Gets the current back buffer for this frame.
		const Texture& GetBackBuffer(bool get_opposite_sRGB_view = false) const;
		DescriptorHeap& GetDescriptorHeap() const { return descriptorHeap; }
		TempBufferAllocator& GetTempBufferAllocator() const { return tempBufferAllocator; }
		CommandQueue& GetCommandQueue() const { return commandQueue; }

		// Gets the max allowed MSAA per texture format.
		MSAA GetMaxMultiSampleCount(Format textureFormat) const;

		const Pipeline& GetMipmapGenerationPipeline() const { return generateMipmapsPipeline; }

		// The bilinear clamp sampler is needed for the mipmap generation pipeline.
		const Descriptor* GetBilinearClampSamplerDescriptor() const { return bilinearClampSampler.GetDescriptor(); }

#ifdef IGLO_D3D12
		ID3D12Device10* GetD3D12Device() const { return graphics.device.Get(); }
		IDXGISwapChain3* GetD3D12SwapChain() const { return graphics.swapChain.Get(); }
#endif
#ifdef IGLO_VULKAN
		VkInstance GetVulkanInstance() const { return graphics.instance; }
		VkPhysicalDevice GetVulkanPhysicalDevice() const { return graphics.physicalDevice; }
		VkDevice GetVulkanDevice() const { return graphics.device; }
		VkSurfaceKHR GetVulkanSurface() const { return graphics.surface; }
		VkSwapchainKHR GetVulkanSwapChain() const { return graphics.swapChain; }
#endif

	private:
		//------------------ Core ------------------//
		bool isLoaded = false;

		DetailedResult LoadWindow(const WindowSettings&);
		DetailedResult LoadGraphicsDevice(const RenderSettings&, Extent2D backBufferSize);
		void UnloadWindow();
		void UnloadGraphicsDevice();

		void OnFatalError(const std::string& message, bool popupMessage);

		//------------------ Window ------------------//

		std::queue<Event> eventQueue; // Window event queue
		IntPoint mousePosition = IntPoint(0, 0);
		PackedBoolArray mouseButtonIsDown;
		PackedBoolArray keyIsDown;

		Impl_WindowData window; // Window context
		WindowState windowedMode; // The window states to use while in windowed mode
		WindowResizeConstraints windowResizeConstraints;

		CallbackModalLoop callbackModalLoop = nullptr;
		bool insideModalLoopCallback = false; // If true, the main thread is inside a callback function called from inside a modal operation.

		DetailedResult Impl_LoadWindow(const WindowSettings&);
		void PrepareWindowPostGraphics(const WindowSettings&);
		void Impl_UnloadWindow();

		void Impl_SetWindowVisible(bool visible);
		void Impl_SetWindowSize(uint32_t width, uint32_t height);
		void Impl_SetWindowPosition(int32_t x, int32_t y);
		void Impl_SetWindowResizable(bool resizable);
		void Impl_SetWindowBordersVisible(bool visible);
		void Impl_SetWindowIconFromImage_BGRA(uint32_t width, uint32_t height, const uint32_t* iconPixels);

		//------------------ WIN32 Specific ------------------//
#ifdef _WIN32
		static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK PrivateWndProc(UINT msg, WPARAM wParam, LPARAM lParam);
#endif

		//------------------ Linux Specific ------------------//
#ifdef __linux__
		void ProcessX11Event(XEvent e);
#endif

		//------------------ Graphics ------------------//

		Impl_GraphicsContext graphics;

		struct SwapChainInfo
		{
			Extent2D extent;
			Format format = Format::None;
			PresentMode presentMode = PresentMode::Vsync;
			uint32_t numBackBuffers = 0;
			std::vector<std::unique_ptr<Texture>> wrapped;
			std::vector<std::unique_ptr<Texture>> wrapped_sRGB_opposite;
			RenderTargetDesc renderTargetDesc;
			RenderTargetDesc renderTargetDesc_sRGB_opposite;
		};
		SwapChainInfo swapChain;

		struct EndOfFrame
		{
			Receipt graphicsReceipt;
			Receipt computeReceipt;
			Receipt copyReceipt;
			mutable std::vector<std::shared_ptr<Texture>> delayedUnloadTextures;
		};
		std::vector<EndOfFrame> endOfFrame;

		GraphicsSpecs graphicsSpecs;

		DisplayMode displayMode = DisplayMode::Windowed;

		uint32_t maxFramesInFlight = 0; // Set at context creation. Can't be changed at runtime.
		uint32_t numFramesInFlight = 0; // Can change at runtime. Can't be higher than maxFramesInFlight.
		uint32_t frameIndex = 0; // always capped by numFramesInFlight

		Pipeline generateMipmapsPipeline;
		Sampler bilinearClampSampler; // For the mipmap generation compute shader

		mutable DescriptorHeap descriptorHeap;
		mutable TempBufferAllocator tempBufferAllocator;
		mutable CommandQueue commandQueue;

		mutable std::vector<uint32_t> maxMSAAPerFormat; // A cached list of max MSAA values for all iglo formats.

		CallbackOnDeviceRemoved callbackOnDeviceRemoved = nullptr;

		void MoveToNextFrame();
		void FreeAllTempResources();

#ifdef IGLO_D3D12
		bool ResizeD3D12SwapChain(Extent2D extent);
#endif
#ifdef IGLO_VULKAN
		void HandleVulkanSwapChainResult(VkResult, const std::string& scenario);
#endif
		DetailedResult CreateSwapChain(Extent2D extent, Format format, uint32_t numBackBuffers,
			uint32_t numFramesInFlight, PresentMode presentMode);
		void DestroySwapChainResources();
		uint32_t GetCurrentBackBufferIndex() const;

		uint32_t Impl_GetMaxMultiSampleCount(Format textureFormat) const;

		DetailedResult Impl_LoadGraphicsDevice();
		void Impl_UnloadGraphicsDevice();
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
