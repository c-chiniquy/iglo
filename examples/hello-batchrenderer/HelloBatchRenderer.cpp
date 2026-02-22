#include "iglo.h"
#include "iglo_font.h"
#include "iglo_batch_renderer.h"
#include "iglo_main_loop.h"

#ifdef IGLO_D3D12
#include "shaders/compiled/PS_Color.h"
#include "shaders/compiled/PS_Texture.h"
#include "shaders/compiled/PS_Wobble.h"
#include "shaders/compiled/VS_InstancedRect.h"
#include "shaders/compiled/VS_InstancedSprite.h"
#include "shaders/compiled/VS_RawRect.h"
#include "shaders/compiled/VS_RawSprite.h"
#include "shaders/compiled/VS_StructuredRect.h"
#include "shaders/compiled/VS_StructuredSprite.h"
#else
#include "shaders/compiled/PS_Color_SPIRV.h"
#include "shaders/compiled/PS_Texture_SPIRV.h"
#include "shaders/compiled/PS_Wobble_SPIRV.h"
#include "shaders/compiled/VS_InstancedRect_SPIRV.h"
#include "shaders/compiled/VS_InstancedSprite_SPIRV.h"
#include "shaders/compiled/VS_RawRect_SPIRV.h"
#include "shaders/compiled/VS_RawSprite_SPIRV.h"
#include "shaders/compiled/VS_StructuredRect_SPIRV.h"
#include "shaders/compiled/VS_StructuredSprite_SPIRV.h"
#endif

#include "AdvDrawStringFuncs.h"
#include <array>

#ifdef IGLO_D3D12
// Agility SDK path and version
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 717; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
#endif

#define SHADER_VS(a) ig::Shader(a, sizeof(a), "VSMain")
#define SHADER_PS(a) ig::Shader(a, sizeof(a), "PSMain")

// A simple button class for the two "Next" and "Prev" demo buttons
class Button
{
public:
	using CallbackOnButtonPress = std::function<void()>;

	Button(const std::string& text, CallbackOnButtonPress callbackOnButtonPress)
	{
		this->text = text;
		this->callbackOnButtonPress = callbackOnButtonPress;
	}

	void SetLocation(ig::IntRect rect)
	{
		this->rect = rect;
	}

	void Draw(ig::BatchRenderer& r, const ig::Texture& buttonTexture, ig::Font& font)
	{
		ig::Color32 rectColor = ig::Color32(255, 255, 255, 128);
		if (mouseHover || (isPressed && mouseHover)) rectColor = ig::Color32(255, 255, 255, 255);

		ig::IntPoint offset = ig::IntPoint(0, 0);
		if (mouseHover) offset = ig::IntPoint(-1, -1);
		if (isPressed) offset = ig::IntPoint(1, 1);

		const ig::FloatRect uv = ig::FloatRect(0, 0, (float)buttonTexture.GetWidth(), (float)buttonTexture.GetHeight());
		r.DrawNineSliceSprite(buttonTexture, rect + offset, uv, rectColor);

		const ig::StringAlignment alignment = ig::StringAlignment::Center;
		const bool wordWrap = true;
		const bool pixelAligned = true;
		ig::DrawAlignedStringInsideRect(r, rect + offset + ig::IntPoint(1, 1),
			text, font, ig::Colors::Black, alignment, wordWrap, pixelAligned);
		ig::DrawAlignedStringInsideRect(r, rect + offset,
			text, font, ig::Colors::White, alignment, wordWrap, pixelAligned);
	}

	// Returns true if event is handled.
	bool OnEvent(const ig::IGLOContext& context, ig::Event e)
	{
		if (isPressed)
		{
			if (e.type == ig::EventType::MouseButtonUp)
			{
				isPressed = false;
				if (rect.ContainsPoint(ig::IntPoint(e.mouse.x, e.mouse.y)))
				{
					if (callbackOnButtonPress) callbackOnButtonPress();
				}
				else
				{
					mouseHover = false;
				}
			}
			else if (e.type == ig::EventType::MouseMove)
			{
				mouseHover = rect.ContainsPoint(ig::IntPoint(e.mouse.x, e.mouse.y));
			}
			return true;
		}
		if (e.type == ig::EventType::MouseButtonDown)
		{
			isPressed = rect.ContainsPoint(ig::IntPoint(e.mouse.x, e.mouse.y));
			if (isPressed) return true;
			return false;
		}
		if (context.IsMouseButtonDown(ig::MouseButton::Left)) return false;
		if (e.type == ig::EventType::MouseMove)
		{
			mouseHover = rect.ContainsPoint(ig::IntPoint(e.mouse.x, e.mouse.y));
		}
		return false;
	}

	ig::IntRect GetRect() const { return rect; }

private:
	ig::IntRect rect;
	bool mouseHover = false;
	bool isPressed = false;
	std::string text = "";
	CallbackOnButtonPress callbackOnButtonPress = nullptr;

};

class App
{
public:

	void Run()
	{
		context = ig::IGLOContext::CreateContext(
			ig::WindowSettings
			{
				.title = sampleName,
				.width = 1024,
				.height = 640,
				.resizable = false,
				.centered = true,
			},
			ig::RenderSettings
			{
				.presentMode = ig::PresentMode::Vsync,
			});
		if (context)
		{
			mainloop.Run(*context,
				std::bind(&App::Start, this),
				std::bind(&App::OnLoopExited, this),
				std::bind(&App::Draw, this),
				std::bind(&App::Update, this, std::placeholders::_1),
				std::bind(&App::FixedUpdate, this),
				std::bind(&App::OnEvent, this, std::placeholders::_1));
		}
	}

private:

	const std::string sampleName = "Hello BatchRenderer";
	const std::string resourceFolder = "resources/";

	std::unique_ptr<ig::IGLOContext> context;
	std::unique_ptr<ig::CommandList> cmd;
	ig::MainLoop mainloop;

	std::unique_ptr<ig::BatchRenderer> batchRenderer;
	std::unique_ptr<ig::BatchRenderer> batchRendererMSAA;

	std::unique_ptr<ig::Font> defaultFont;
	std::unique_ptr<ig::Font> trimSDF;
	std::unique_ptr<ig::Font> vegur;
	std::unique_ptr<ig::Texture> buttonTexture;
	std::unique_ptr<ig::Texture> spriteTexture;
	std::unique_ptr<ig::Texture> leafTexture;
	std::unique_ptr<ig::Texture> woodTexture;
	std::unique_ptr<ig::Texture> mono16bitTexture;
	std::unique_ptr<ig::Texture> duo16bitTexture;
	std::unique_ptr<ig::Texture> rgba16bitTexture;
	std::unique_ptr<ig::Texture> hdrTexture;
	std::unique_ptr<ig::Texture> appleTexture;

	double tick = 0;

	static constexpr uint32_t numDemos = 7;
	const std::array<std::string, numDemos> demos =
	{
		"Custom string drawing functions",
		"Sprites and textures",
		"Basic primitives",
		"World matrix text transformations",
		"Rotate text in 3D",
		"Exotic texture formats",
		"Custom batch types",
	};
	uint32_t currentDemo = 0;

	ig::FloatRect demoStringRect = ig::FloatRect(256 - 200, 100, 256 + 200, 300);
	bool demoStringRectIsDragged = false;
	ig::StringAlignment demoStringAlignment = ig::StringAlignment::Center;
	ig::Font* demoStringFont = nullptr;

	static constexpr uint32_t demoSpriteCount = 100'000;
	std::array<ig::Vector2, demoSpriteCount> demoSpritePos;

	uint32_t demoCakeSides = 12;

	ig::BatchType batchFloatColor = 0; // Uses floating point per-vertex colors
	ig::BatchType batchFloatColorMono = 0; // Uses floating point per-vertex colors. For monochrome textures.
	ig::BatchType batchInstancedRects = 0; // Uses instancing.
	ig::BatchType batchInstancedSprites = 0; // Uses instancing.
	ig::BatchType batchRawRects = 0; // Uses vertex pulling with a raw buffer.
	ig::BatchType batchRawSprites = 0; // Uses vertex pulling with a raw buffer.
	ig::BatchType batchStructuredRects = 0; // Uses vertex pulling with a structured buffer.
	ig::BatchType batchStructuredSprites = 0; // Uses vertex pulling with a structured buffer.
	ig::BatchType batchIndexedRects = 0; // Uses indexing.
	ig::BatchType batchIndexedSprites = 0; // Uses indexing.
	ig::BatchType batchCustomPixelShaderSprites = 0;
	ig::BatchType batchCustomBlendSprites = 0;
	std::unique_ptr<ig::Buffer> quadIndexBuffer;

	// This is the vertex layout used by our floating point color custom batch pipelines.
	struct Vertex_ScaledSpriteFloatColor
	{
		ig::Vector2 position;
		float width;
		float height;
		ig::FloatRect uv;
		ig::Color color; // Floating point color instead of 32-bit color
	};

	struct Vertex_Sprite_Structured
	{
		ig::Vector2 position;
		uint16_t width;
		uint16_t height;
		uint16_t u;
		uint16_t v;
		ig::Color32 color;
		uint32_t padding;
	};
	static_assert((sizeof(Vertex_Sprite_Structured) % 8) == 0, "Vulkan requires an alignment of 8 for structured buffers.");

	struct Vertex_Rect_Structured
	{
		ig::Vector2 position;
		float width;
		float height;
		ig::Color32 color;
		uint32_t padding;
	};
	static_assert((sizeof(Vertex_Rect_Structured) % 8) == 0, "Vulkan requires an alignment of 8 for structured buffers.");

	float demoScaleA = 1.0f;
	float demoScaleB = 1.0f;

	const ig::Color32 clearColor = ig::Color32(32, 32, 32);
	std::unique_ptr<ig::Texture> rendertarget;

	void OnButtonPressed_PrevDemo()
	{
		if (currentDemo == 0)
		{
			currentDemo = numDemos - 1;
		}
		else
		{
			currentDemo--;
		}
	}
	void OnButtonPressed_NextDemo()
	{
		currentDemo++;
		if (currentDemo >= numDemos) currentDemo = 0;
	}

	Button buttonNext = Button("Next", std::bind(&App::OnButtonPressed_NextDemo, this));
	Button buttonPrev = Button("Prev", std::bind(&App::OnButtonPressed_PrevDemo, this));

	void UpdateButtonLocations()
	{
		const int32_t spacing = 15;
		ig::IntRect rect(0, 0, 120, 32); // Button size
		rect += ig::IntPoint(
			context->GetWidth() - rect.GetWidth() - spacing, // X
			context->GetHeight() - rect.GetHeight() - spacing); // Y

		buttonNext.SetLocation(rect);

		rect -= ig::IntPoint(rect.GetWidth() + spacing, 0);
		buttonPrev.SetLocation(rect);
	}

	void Start()
	{
		cmd = ig::CommandList::Create(*context, ig::CommandListType::Graphics);

		ig::RenderTargetDesc backBufferDesc = context->GetBackBufferRenderTargetDesc();
		ig::RenderTargetDesc backBufferDescMSAA = backBufferDesc;
		backBufferDescMSAA.msaa = ig::MSAA::X8;

		batchRenderer = ig::BatchRenderer::Create(*context, backBufferDesc);
		batchRendererMSAA = ig::BatchRenderer::Create(*context, backBufferDescMSAA);

		cmd->Begin();
		{
			defaultFont = ig::Font::CreatePrebaked(*context, *cmd, ig::GetDefaultFont());

			buttonTexture = ig::Texture::LoadFromFile(*context, *cmd, resourceFolder + "button.png");
			spriteTexture = ig::Texture::LoadFromFile(*context, *cmd, resourceFolder + "sprite-animation.png");
			leafTexture = ig::Texture::LoadFromFile(*context, *cmd, resourceFolder + "transparent-leaf.png");
			woodTexture = ig::Texture::LoadFromFile(*context, *cmd, resourceFolder + "wood-planks.jpg");

			mono16bitTexture = ig::Texture::LoadFromFile(*context, *cmd, resourceFolder + "16-bit-mono.png", false);
			duo16bitTexture = ig::Texture::LoadFromFile(*context, *cmd, resourceFolder + "16-bit-duo.png", false);
			rgba16bitTexture = ig::Texture::LoadFromFile(*context, *cmd, resourceFolder + "16-bit-RGBA.png", false);
			hdrTexture = ig::Texture::LoadFromFile(*context, *cmd, resourceFolder + "hdr.hdr");
			appleTexture = ig::Texture::LoadFromFile(*context, *cmd, resourceFolder + "green-apple.png");

			// Create the quad index buffer that's used by the custom batch types that use indexing
			std::vector<uint16_t> indices;
			const uint32_t maxNumPrimitives = 10000;
			for (uint32_t i = 0; i < maxNumPrimitives; i++)
			{
				uint32_t k = i * 4;
				indices.push_back(k);
				indices.push_back(k + 1);
				indices.push_back(k + 2);

				indices.push_back(k + 2);
				indices.push_back(k + 1);
				indices.push_back(k + 3);
			}
			quadIndexBuffer = ig::Buffer::CreateIndexBuffer(*context, ig::IndexFormat::UINT16, (uint32_t)indices.size(), ig::BufferUsage::Default);
			quadIndexBuffer->SetData(*cmd, indices.data());
		}
		cmd->End();
		context->WaitForCompletion(context->Submit(*cmd));

		vegur = ig::Font::LoadFromFile(*context, resourceFolder + "Vegur-Regular.otf", 17);
		trimSDF = ig::Font::LoadFromFile(*context, resourceFolder + "trim.ttf", 25, ig::FontSettings(ig::FontType::SDF));
		if (!vegur || !trimSDF)
		{
			ig::PopupMessage("Failed to load one or more fonts from: " + resourceFolder, "Error", context.get());
			mainloop.Quit();
			return;
		}
		demoStringFont = vegur.get();

		for (uint32_t i = 0; i < demoSpriteCount; i++)
		{
			ig::Vector2 pos = ig::Vector2(470, 300);
			pos += ig::Vector2(ig::Random::NextFloat(0, 500), ig::Random::NextFloat(0, 200));
			demoSpritePos[i] = pos;
		}

		// Create custom batch types
		{
			// When creating custom batch types it's convenient to take parameters
			// from an existing batch type as a base and then make changes to it.
			// In this case, the batch type 'ScaledSprites' is the most similar to what we want.
			ig::BatchParams params = ig::GetStandardBatchParams(ig::StandardBatchType::ScaledSprites, backBufferDesc);

			// This custom batch will use a slightly different vertex layout.
			// Floating point colors is used instead of 32-bit colors.
			// This will allow us to draw hdr textures with brightness higher than 1.0.
			assert(params.pipelineDesc.vertexLayout.size() >= 4);
			params.pipelineDesc.vertexLayout[4].format = ig::Format::FLOAT_FLOAT_FLOAT_FLOAT;
			params.batchDesc.bytesPerVertex = sizeof(Vertex_ScaledSpriteFloatColor);
			batchFloatColor = batchRenderer->CreateBatchType(params);

			// Do the same thing as before but with an opaque monochrome shader, for drawing monochrome textures.
			params = ig::GetStandardBatchParams(ig::StandardBatchType::ScaledSprites_MonoOpaque, backBufferDesc);
			params.pipelineDesc.vertexLayout[4].format = ig::Format::FLOAT_FLOAT_FLOAT_FLOAT;
			params.batchDesc.bytesPerVertex = sizeof(Vertex_ScaledSpriteFloatColor);
			batchFloatColorMono = batchRenderer->CreateBatchType(params);

			// Create custom batch types that draw quads using methods such as instancing, vertex pulling, and indexing.
			// (BatchRenderer draws quads using instancing by default)
			static const std::vector<ig::VertexElement> layout_instanced_Rect =
			{
				ig::VertexElement(ig::Format::FLOAT_FLOAT, "POSITION", 0, 0, ig::InputClass::PerInstance, 1),
				ig::VertexElement(ig::Format::FLOAT, "WIDTH", 0, 0, ig::InputClass::PerInstance, 1),
				ig::VertexElement(ig::Format::FLOAT, "HEIGHT", 0, 0, ig::InputClass::PerInstance, 1),
				ig::VertexElement(ig::Format::BYTE_BYTE_BYTE_BYTE, "COLOR", 0, 0, ig::InputClass::PerInstance, 1),
			};
			static const std::vector<ig::VertexElement> layout_instanced_Sprite =
			{
				ig::VertexElement(ig::Format::FLOAT_FLOAT, "POSITION", 0, 0, ig::InputClass::PerInstance, 1),
				ig::VertexElement(ig::Format::UINT16_NotNormalized, "WIDTH", 0, 0, ig::InputClass::PerInstance, 1),
				ig::VertexElement(ig::Format::UINT16_NotNormalized, "HEIGHT", 0, 0, ig::InputClass::PerInstance, 1),
				ig::VertexElement(ig::Format::UINT16_UINT16_NotNormalized, "TEXCOORD", 0, 0, ig::InputClass::PerInstance, 1),
				ig::VertexElement(ig::Format::BYTE_BYTE_BYTE_BYTE, "COLOR", 0, 0, ig::InputClass::PerInstance, 1),
			};
			params = ig::BatchParams();
			params.batchDesc.vertGenMethod = ig::BatchDesc::VertexGenerationMethod::Instancing;
			params.batchDesc.inputVerticesPerPrimitive = 1;
			params.batchDesc.outputVerticesPerPrimitive = 4;
			params.batchDesc.bytesPerVertex = sizeof(ig::Vertex_Rect);
			params.pipelineDesc.primitiveTopology = ig::PrimitiveTopology::TriangleStrip;
			params.pipelineDesc.vertexLayout = layout_instanced_Rect;
			params.pipelineDesc.VS = SHADER_VS(g_VS_InstancedRect);
			params.pipelineDesc.PS = SHADER_PS(g_PS_Color);
			params.pipelineDesc.rasterizerState = ig::RasterizerDesc::NoCull;
			params.pipelineDesc.blendStates = { ig::BlendDesc::StraightAlpha };
			params.pipelineDesc.depthState = ig::DepthDesc::DepthDisabled;
			params.pipelineDesc.renderTargetDesc = backBufferDesc;
			batchInstancedRects = batchRenderer->CreateBatchType(params);

			params.pipelineDesc.VS = SHADER_VS(g_VS_InstancedSprite);
			params.pipelineDesc.PS = SHADER_PS(g_PS_Texture);
			params.pipelineDesc.vertexLayout = layout_instanced_Sprite;
			batchInstancedSprites = batchRenderer->CreateBatchType(params);

			params.batchDesc = ig::BatchDesc();
			params.batchDesc.vertGenMethod = ig::BatchDesc::VertexGenerationMethod::VertexPullingRaw;
			params.batchDesc.inputVerticesPerPrimitive = 1;
			params.batchDesc.outputVerticesPerPrimitive = 6;
			params.batchDesc.bytesPerVertex = sizeof(ig::Vertex_Rect);
			params.pipelineDesc.primitiveTopology = ig::PrimitiveTopology::TriangleList;
			params.pipelineDesc.vertexLayout = {};
			params.pipelineDesc.VS = SHADER_VS(g_VS_RawRect);
			params.pipelineDesc.PS = SHADER_PS(g_PS_Color);
			batchRawRects = batchRenderer->CreateBatchType(params);

			params.batchDesc.vertGenMethod = ig::BatchDesc::VertexGenerationMethod::VertexPullingStructured;
			params.batchDesc.bytesPerVertex = sizeof(Vertex_Rect_Structured);
			params.pipelineDesc.VS = SHADER_VS(g_VS_StructuredRect);
			batchStructuredRects = batchRenderer->CreateBatchType(params);

			params.batchDesc.vertGenMethod = ig::BatchDesc::VertexGenerationMethod::VertexPullingRaw;
			params.batchDesc.bytesPerVertex = sizeof(ig::Vertex_Sprite);
			params.pipelineDesc.VS = SHADER_VS(g_VS_RawSprite);
			params.pipelineDesc.PS = SHADER_PS(g_PS_Texture);
			batchRawSprites = batchRenderer->CreateBatchType(params);

			params.batchDesc.vertGenMethod = ig::BatchDesc::VertexGenerationMethod::VertexPullingStructured;
			params.batchDesc.bytesPerVertex = sizeof(Vertex_Sprite_Structured);
			params.pipelineDesc.VS = SHADER_VS(g_VS_StructuredSprite);
			batchStructuredSprites = batchRenderer->CreateBatchType(params);

			params = ig::GetStandardBatchParams(ig::StandardBatchType::Triangles_XYC, backBufferDesc);
			params.batchDesc.inputVerticesPerPrimitive = 4;
			params.batchDesc.outputVerticesPerPrimitive = 6;
			params.batchDesc.vertGenMethod = ig::BatchDesc::VertexGenerationMethod::Indexing;
			params.batchDesc.indexBuffer = quadIndexBuffer.get();
			batchIndexedRects = batchRenderer->CreateBatchType(params);

			params = ig::GetStandardBatchParams(ig::StandardBatchType::Triangles_XYCUV, backBufferDesc);
			params.batchDesc.inputVerticesPerPrimitive = 4;
			params.batchDesc.outputVerticesPerPrimitive = 6;
			params.batchDesc.vertGenMethod = ig::BatchDesc::VertexGenerationMethod::Indexing;
			params.batchDesc.indexBuffer = quadIndexBuffer.get();
			batchIndexedSprites = batchRenderer->CreateBatchType(params);

			// Custom blend state
			params = ig::GetStandardBatchParams(ig::StandardBatchType::Sprites, backBufferDesc);
			ig::BlendDesc customBlend;
			customBlend.enabled = true;
			customBlend.blendOp = ig::BlendOperation::Add;
			customBlend.srcBlend = ig::BlendData::One;
			customBlend.srcBlendAlpha = ig::BlendData::One;
			customBlend.destBlend = ig::BlendData::One;
			customBlend.destBlendAlpha = ig::BlendData::Zero;
			assert(params.pipelineDesc.blendStates.size() >= 1);
			params.pipelineDesc.blendStates[0] = customBlend;
			batchCustomBlendSprites = batchRenderer->CreateBatchType(params);

			// Custom pixel shader
			params = ig::GetStandardBatchParams(ig::StandardBatchType::Sprites, backBufferDesc);
			params.pipelineDesc.PS = ig::Shader(g_PS_Wobble, sizeof(g_PS_Wobble), "PSMain");
			batchCustomPixelShaderSprites = batchRenderer->CreateBatchType(params);
		}

		ig::ClearValue clearValue;
		clearValue.color = clearColor;
		rendertarget = ig::Texture::Create(*context, context->GetWidth(), context->GetHeight(),
			backBufferDesc.colorFormats.at(0), ig::TextureUsage::RenderTexture, ig::MSAA::X8, clearValue);

		UpdateButtonLocations();
	}

	void OnLoopExited()
	{
		context->WaitForIdleDevice();
	}

	void Update(double elapsedSeconds)
	{
		tick += elapsedSeconds;
	}

	void FixedUpdate()
	{
	}

	void OnEvent(ig::Event e)
	{
		if (e.type == ig::EventType::CloseRequest)
		{
			mainloop.Quit();
			return;
		}
		else if (e.type == ig::EventType::KeyPress)
		{
			switch (e.key)
			{
			case ig::Key::Escape:
				mainloop.Quit();
				return;

			default:
				break;
			}
		}

		if (buttonNext.OnEvent(*context, e)) return;
		if (buttonPrev.OnEvent(*context, e)) return;

		if (currentDemo == 0)
		{
			if (e.type == ig::EventType::MouseButtonDown)
			{
				if (e.mouse.button == ig::MouseButton::Left) demoStringRectIsDragged = true;
			}
			else if (e.type == ig::EventType::MouseButtonUp)
			{
				if (e.mouse.button == ig::MouseButton::Left) demoStringRectIsDragged = false;
			}
			if (e.type == ig::EventType::MouseMove ||
				e.type == ig::EventType::MouseButtonDown ||
				e.type == ig::EventType::MouseButtonUp)
			{
				if (demoStringRectIsDragged)
				{
					demoStringRect.right = (float)context->GetMouseX();
					demoStringRect.bottom = (float)context->GetMouseY();
				}
			}
			else if (e.type == ig::EventType::KeyPress)
			{
				switch (e.key)
				{
				case ig::Key::Numpad1: demoStringAlignment = ig::StringAlignment::BottomLeft; return;
				case ig::Key::Numpad2: demoStringAlignment = ig::StringAlignment::Bottom; return;
				case ig::Key::Numpad3: demoStringAlignment = ig::StringAlignment::BottomRight; return;
				case ig::Key::Numpad4: demoStringAlignment = ig::StringAlignment::Left; return;
				case ig::Key::Numpad5: demoStringAlignment = ig::StringAlignment::Center; return;
				case ig::Key::Numpad6: demoStringAlignment = ig::StringAlignment::Right; return;
				case ig::Key::Numpad7: demoStringAlignment = ig::StringAlignment::TopLeft; return;
				case ig::Key::Numpad8: demoStringAlignment = ig::StringAlignment::Top; return;
				case ig::Key::Numpad9: demoStringAlignment = ig::StringAlignment::TopRight; return;

				case ig::Key::F:
					if (demoStringFont == vegur.get()) demoStringFont = trimSDF.get();
					else if (demoStringFont == trimSDF.get()) demoStringFont = defaultFont.get();
					else demoStringFont = vegur.get();
					return;

				default:
					break;
				}
			}
		}
		else if (currentDemo == 2)
		{
			if (e.type == ig::EventType::KeyPress)
			{
				if (e.key == ig::Key::K)
				{
					demoCakeSides /= 3;
					if (demoCakeSides < 3) demoCakeSides = 3;
				}
				else if (e.key == ig::Key::L)
				{
					demoCakeSides *= 2;
					if (demoCakeSides > 96) demoCakeSides = 96;
				}
			}
		}
	}

	void Draw()
	{
		cmd->Begin();
		{
			bool useMSAA = (currentDemo == 2 && context->IsMouseButtonDown(ig::MouseButton::Left));
			ig::BatchRenderer& r = useMSAA ? *batchRendererMSAA : *batchRenderer;

			if (useMSAA)
			{
				cmd->AddTextureBarrier(*rendertarget, ig::SimpleBarrier::Discard, ig::SimpleBarrier::RenderTarget);
				cmd->FlushBarriers();

				cmd->SetRenderTarget(rendertarget.get());
				cmd->SetViewport((float)rendertarget->GetWidth(), (float)rendertarget->GetHeight());
				cmd->SetScissorRectangle(rendertarget->GetWidth(), rendertarget->GetHeight());
				cmd->ClearColor(*rendertarget, clearColor);
			}
			else
			{
				cmd->AddTextureBarrier(context->GetBackBuffer(), ig::SimpleBarrier::Discard, ig::SimpleBarrier::RenderTarget);
				cmd->FlushBarriers();

				cmd->SetRenderTarget(&context->GetBackBuffer());
				cmd->SetViewport((float)context->GetWidth(), (float)context->GetHeight());
				cmd->SetScissorRectangle(context->GetWidth(), context->GetHeight());
				cmd->ClearColor(context->GetBackBuffer(), clearColor);
			}

			r.Begin(*cmd);
			{
				if (currentDemo == 0)
				{
					ig::SDFEffect sdfEffect;
					sdfEffect.sdfEffectFlags = (uint32_t)ig::SDFEffectFlags::Outline | (uint32_t)ig::SDFEffectFlags::Glow;
					r.SetSDFEffect(sdfEffect);

					std::string str =
						"This text is wordwrapped and clipped to only appear inside a rectangular area."
						" Use numpad keys to change alignment."
						" Hold down left mouse button to resize rectangle."
						" Press 'F' to change font.";
					r.DrawRectangle(demoStringRect, ig::Color32(64, 255, 128, 75));
					ig::DrawAlignedStringInsideRect(r, demoStringRect, str, *demoStringFont, ig::Colors::White,
						demoStringAlignment, true, true);

					ig::FloatRect rect = demoStringRect + ig::Vector2(1024 / 2, 0);
					str = "This text is aligned but not\nclipped to be inside the rectangle.";
					r.DrawRectangle(rect, ig::Color32(64, 128, 255, 75));
					ig::DrawAlignedString(r, rect, str, *demoStringFont, ig::Colors::White, demoStringAlignment, true);

					ig::DrawColorfulAnimatedString(r, 70, 450, "This is a colorful animated string!",
						*demoStringFont, (float)tick, false);

				}
				else if (currentDemo == 1)
				{
					// Draw a leaf
					r.DrawTexture(*leafTexture, 10, 70);
					// Draw a stretched leaf
					r.DrawTexture(*leafTexture, 5, 450, 400, 140);
					// Draw with red color and a bit smaller
					r.DrawTexture(*leafTexture, 130, 120, 140, 140, ig::Colors::Red);
					// Draw a portion of the leaf with blue color.
					r.DrawTexture(*leafTexture, 260, 50, 140, 256, ig::FloatRect(0, 0, 140, 256), ig::Color32(32, 64, 255));
					// Draw a leaf that fades in and out
					r.DrawTexture(*leafTexture, 290, 100, 100, 100, ig::Color32(255, 255, 255, byte((sinf((float)tick) + 1.0f) * 128.f)));
					// Draw a leaf that rotates around topleft origin
					ig::TransformedSpriteParams transformedSprite =
					{
						.position = ig::Vector2(520, 100),
						.width = 64,
						.height = 64,
						.uv = ig::FloatRect(0, 0, 256, 256),
						.rotationOrigin = ig::Vector2(0, 0),
						.rotation = (float)tick * 2,
						.color = ig::Colors::White,
					};
					r.DrawTransformedSprite(*leafTexture, transformedSprite);
					// Draw a leaf that rotates around center origin
					transformedSprite =
					{
						.position = ig::Vector2(620, 100 - 32),
						.width = 64,
						.height = 64,
						.uv = ig::FloatRect(0, 0, 256, 256),
						.rotationOrigin = ig::Vector2(32, 32),
						.rotation = (float)tick * 2,
						.color = ig::Colors::White,
					};
					r.DrawTransformedSprite(*leafTexture, transformedSprite);
					// Draw the origins
					r.DrawCircle(520, 100, 2, ig::Colors::White);
					r.DrawCircle(620 + 32, 100, 2, ig::Colors::White);
					// Draw leaf upside down
					r.DrawTexture(*leafTexture, 70, 330, 128, 128, ig::FloatRect(256, 256, 0, 0), ig::Colors::White);
					// Draw leaf texture with stretched uv coordinates, making it appear as many leafs
					r.DrawScaledSprite(*leafTexture, 270, 330, 128, 256, ig::FloatRect(0, 0, 128 * 8, 256 * 8));
					// Draw a portion of the wood texture, with changing rotation and transparency.
					transformedSprite =
					{
						.position = ig::Vector2(280, 380),
						.width = 32,
						.height = 128,
						.uv = ig::FloatRect(0, 0, 32, 128),
						.rotationOrigin = ig::Vector2(16, 64),
						.rotation = (float)tick,
						.color = ig::Color(1, 1, 1, float(sin(tick) + 1.0f) / 2.0f).ToColor32(),
					};
					r.DrawTransformedSprite(*woodTexture, transformedSprite);
					// Draw a leaf with alpha blending disabled
					r.DrawTexture(*leafTexture, (float)context->GetWidth() - leafTexture->GetWidth(), 0,
						ig::Colors::White, ig::TextureDrawStyle::BlendDisabled);

					float spriteAnimationFPS = 24.0f;
					uint32_t numSpriteAnimationFrames = 40;
					uint32_t spriteIndex = uint32_t((float)tick * spriteAnimationFPS) % numSpriteAnimationFrames;
					uint32_t spriteSize = 32;
					uint32_t spriteX = (spriteSize * spriteIndex) % spriteTexture->GetWidth();
					uint32_t spriteY = (spriteIndex / (spriteTexture->GetWidth() / spriteSize)) * spriteSize;
					r.DrawSprite(*spriteTexture, 580, 220, ig::IntRect(spriteX, spriteY, spriteX + spriteSize, spriteY + spriteSize));
					r.DrawString(470, 225, "Sprite animation:", *defaultFont, ig::Colors::White);

					// This will draw many sprites. BatchRenderer will automatically batch them together into the same draw call,
					// as long as the same texture is used for all sprites.
					// Note that in most cases, if two BatchRenderer drawing function have different names, they most
					// likely use different pipelines and thus can't be batched together into the same draw call.
					for (uint32_t i = 0; i < demoSpriteCount; i++)
					{
						r.DrawSprite(*buttonTexture, demoSpritePos[i].x, demoSpritePos[i].y,
							ig::IntRect(0, 0, buttonTexture->GetWidth(), buttonTexture->GetHeight()));
					}
					r.DrawString(470, 280, ig::ToString(demoSpriteCount, " sprites in 1 draw call:"), *defaultFont, ig::Colors::White);

					r.DrawString(470, 540, "Different samplers:", *defaultFont, ig::Colors::White);
					r.DrawScaledSprite(*buttonTexture, 470, 560, 64, 64, ig::FloatRect(0, 0, 16, 16));
					r.SetSamplerToPixelatedTextures();
					r.DrawScaledSprite(*buttonTexture, 540, 560, 64, 64, ig::FloatRect(0, 0, 16, 16));
				}
				else if (currentDemo == 2)
				{
					// Draw point primitives
					r.DrawPoint(50, 130, ig::Colors::Red);
					r.DrawPoint(50, 135, ig::Colors::Green);
					r.DrawPoint(50, 140, ig::Colors::Blue);

					// Draw line primitives, with and without anti alias
					r.DrawLine(100, 130, 190, 180, ig::Colors::Green, ig::LineRasterizationMode::Pixelated);
					r.DrawLine(100, 150, 190, 200, ig::Colors::Green, ig::LineRasterizationMode::Smooth);

					// The rounded rectangle border function draws with line primitives as well
					r.DrawRoundedRectangleBorder(100, 200, 70, 50, ig::Colors::Yellow, 17, 32);

					// Draw triangle primitives
					r.DrawTriangle(ig::Vector2(250, 130), ig::Vector2(320, 200), ig::Vector2(250, 200),
						ig::Colors::Red, ig::Colors::Green, ig::Colors::Blue);
					r.DrawTriangle(ig::Vector2(250, 230), ig::Vector2(350, 300), ig::Vector2(250, 300),
						ig::Colors::White, ig::Colors::White, ig::Colors::White);
					r.DrawTriangle(ig::Vector2(270, 260), ig::Vector2(370, 330), ig::Vector2(270, 330), ig::Color32(0, 0, 255, 128));
					r.DrawTexturedTriangle(*woodTexture, ig::Vector2(290, 290), ig::Vector2(390, 360), ig::Vector2(290, 360),
						ig::Colors::White, ig::Colors::White, ig::Colors::White, ig::Vector2(0, 0), ig::Vector2(0, 1), ig::Vector2(1, 0));

					// Draw a rectangle
					r.DrawRectangle(400 + (sinf((float)tick) * 5.0f), 150, 50, 50, ig::Colors::Green);

					// Draw various things
					r.DrawRectangleBorder(500, 150, 50, 50, ig::Colors::Green, (sinf((float)tick) + 1.0f) * 5.0f);
					r.DrawTexturedRectangularLine(*woodTexture, 600, 220, 650, 270, (sinf((float)tick) + 1.0f) * 10.0f, ig::Colors::White);
					r.DrawRectangularLine(600, 150, 650, 200, (sinf((float)tick) + 1.0f) * 10.0f, ig::Colors::Red);
					r.DrawRoundedRectangle(700, 150 + (sinf((float)tick) * 5.0f), 70, 50, ig::Colors::Cyan, 17, 32);
					std::string str = "Hold down left mouse button to use MSAA X8 anti alias.";
					if (useMSAA) str += " MSAA Enabled!";
					r.DrawString(400, 100, str, *defaultFont, ig::Colors::White);

					// Draw cake shapes (they consist of triangle and line primitives)
					r.DrawCake(470, 550, 50, demoCakeSides, 0, (float)context->GetMouseX(),
						ig::Colors::Yellow, ig::Colors::Red);
					r.DrawCakeBorder(590, 550, 50, demoCakeSides, 0, (float)context->GetMouseX(),
						ig::Colors::Yellow, ig::Colors::Red);
					r.DrawString(390, 450, ig::ToString("Cake shape sides: ", demoCakeSides, "\nPress [K] and [L] to increase/decrease sides."),
						*defaultFont, ig::Colors::White);

					// Draw circles (BatchRenderer internally draws circles using circle pixel shaders, not polygons)
					r.DrawCircle(80, 450, 20, ig::Colors::Green);
					ig::CircleParams circle =
					{
						.position = ig::Vector2(110 + sinf((float)tick) * 20.0f, 450),
						.radius = 20,
						.smoothing = 1.0f,
						.borderThickness = 1.0f,
						.innerColor = ig::Colors::Red,
						.outerColor = ig::Color32(255, 0, 0, 0),
						.borderColor = ig::Colors::Red,
					};
					r.DrawCircle(circle);
					circle = {};
					circle =
					{
						.position = ig::Vector2(180, 450),
						.radius = 20,
						.innerColor = ig::Colors::Green,
						.outerColor = ig::Colors::Black,
					};
					r.DrawCircle(circle);
					circle = {};
					circle =
					{
						.position = ig::Vector2(230, 450),
						.radius = 20,
						.smoothing = 2.0f,
						.borderThickness = 7.0f,
						.innerColor = ig::Colors::Transparent,
						.outerColor = ig::Colors::Transparent,
						.borderColor = ig::Colors::Red,
					};
					r.DrawCircle(circle);
					circle = {};
					circle =
					{
						.position = ig::Vector2(80, 350),
						.radius = (sinf((float)tick) + 1.0f) * 15.0f,
						.innerColor = ig::Colors::White,
						.outerColor = ig::Colors::White,
					};
					r.DrawCircle(circle);
					circle.position = ig::Vector2(150, 350);
					circle.smoothing = 2.0f;
					r.DrawCircle(circle);
					ig::Color32 transparentWhite = ig::Color32(255, 255, 255, 0);
					circle = {};
					circle =
					{
						.position = ig::Vector2(80, 520),
						.radius = 18,
						.borderThickness = 1.0f,
						.innerColor = transparentWhite,
						.outerColor = transparentWhite,
						.borderColor = ig::Colors::White,
					};
					r.DrawCircle(circle);
					circle.position = ig::Vector2(80 + 50, 520);
					circle.borderThickness = 2.0f;
					r.DrawCircle(circle);
					circle.position = ig::Vector2(80 + 100, 520);
					circle.borderThickness = 4.0f;
					r.DrawCircle(circle);
					circle.position = ig::Vector2(80 + 150, 520);
					circle.borderThickness = 8.0f;
					r.DrawCircle(circle);
					circle.position = ig::Vector2(80, 580);
					circle.smoothing = 0.0f;
					circle.borderThickness = 8.0f;
					r.DrawCircle(circle);
					circle.position = ig::Vector2(80 + 50, 580);
					circle.smoothing = 1.0f;
					circle.borderThickness = 8.0f;
					r.DrawCircle(circle);
					circle.position = ig::Vector2(80 + 100, 580);
					circle.smoothing = 2.0f;
					circle.borderThickness = 8.0f;
					r.DrawCircle(circle);
					circle.position = ig::Vector2(80 + 150, 580);
					circle.smoothing = 4.0f;
					circle.borderThickness = 8.0f;
					r.DrawCircle(circle);

					// Draw a circle where the cursor is
					circle = {};
					circle =
					{
						.position = ig::Vector2((float)context->GetMouseX(), (float)context->GetMouseY()),
						.radius = 29,
						.smoothing = 2,
						.borderThickness = 2,
						.innerColor = ig::Color32(0, 0, 255, 255),
						.outerColor = ig::Color32(0, 0, 255, 0),
						.borderColor = ig::Colors::White,
					};
					r.DrawCircle(circle);

				}
				else if (currentDemo == 3)
				{
					if (context->IsKeyDown(ig::Key::W)) demoScaleA += 1.0f * (float)mainloop.GetElapsedSeconds();
					if (context->IsKeyDown(ig::Key::S)) demoScaleA -= 1.0f * (float)mainloop.GetElapsedSeconds();
					if (context->IsKeyDown(ig::Key::A)) demoScaleB -= 2.0f * (float)mainloop.GetElapsedSeconds();
					if (context->IsKeyDown(ig::Key::D)) demoScaleB += 2.0f * (float)mainloop.GetElapsedSeconds();

					std::string str = "Hold down [WASD] keys to change scale.";
					ig::Vector2 strSize = r.MeasureString(str, *vegur);
					float x = roundf(float(context->GetWidth() / 2) - (strSize.x / 2.0f));
					float y = 50;
					r.DrawString(x + 1, y + 1, str, *vegur, ig::Colors::Black);
					r.DrawString(x, y, str, *vegur, ig::Colors::Yellow);

					r.SetWorldMatrix(
						ig::Vector3((float)context->GetMouseX(), (float)context->GetMouseY(), 0),
						ig::Quaternion::Identity,
						ig::Vector3(demoScaleA, demoScaleA, 1));
					r.DrawString(0, 0, "(1) Anchored in the top left.", *trimSDF, ig::Colors::White);

					float rotation = context->GetMouseX() * 0.01f;
					r.SetWorldMatrix(
						ig::Vector3((float)context->GetMouseX(), (float)context->GetMouseY(), 0),
						ig::Quaternion::Euler(ig::Vector3(0, 0, rotation)),
						ig::Vector3(demoScaleB, demoScaleB, 1.0f));
					str = "(2) Anchored in the center. Rotating.";
					strSize = r.MeasureString(str, *trimSDF);
					r.DrawString(-(strSize.x / 2.0f), -(strSize.y / 2.0f), str, *trimSDF, ig::Colors::White);
					r.DrawCircle(0, 0, 2, ig::Colors::Green);

					r.RestoreWorldMatrix();
				}
				else if (currentDemo == 4)
				{
					cmd->ClearColor(context->GetBackBuffer(), ig::Color32(70, 70, 70));

					std::string str = "Move mouse to rotate text.\n"
						"Hold left or right mouse buttons to change SDF style.\n"
						"Hold middle mouse button to show measured string area.";
					ig::Vector2 strSize = r.MeasureString(str, *vegur);
					float x = roundf(float(context->GetWidth() / 2) - (strSize.x / 2.0f));
					float y = 20;
					r.DrawString(x + 1, y + 1, str, *vegur, ig::Colors::Black);
					r.DrawString(x, y, str, *vegur, ig::Colors::Yellow);

					ig::Matrix4x4 oldView = r.GetViewMatrix();
					ig::Matrix4x4 oldProj = r.GetProjectionMatrix();

					ig::SDFEffect sdfEffect;
					sdfEffect.sdfEffectFlags = (uint32_t)ig::SDFEffectFlags::Outline | (uint32_t)ig::SDFEffectFlags::Glow;
					if (context->IsMouseButtonDown(ig::MouseButton::Right)) // Hard drop shadow
					{
						sdfEffect.sdfEffectFlags = (uint32_t)ig::SDFEffectFlags::Glow;
						sdfEffect.glowSize = 0.1f;
						sdfEffect.glowOffset = ig::Vector2(2.5f, 2.5f);
						sdfEffect.glowColor = ig::Colors::Black;
					}
					if (context->IsMouseButtonDown(ig::MouseButton::Left))
					{
						sdfEffect.sdfEffectFlags = (uint32_t)ig::SDFEffectFlags::Glow;
					}
					if (context->IsMouseButtonDown(ig::MouseButton::Left) &&
						context->IsMouseButtonDown(ig::MouseButton::Right))
					{
						sdfEffect.sdfEffectFlags = (uint32_t)ig::SDFEffectFlags::NoEffects;
					}
					r.SetSDFEffect(sdfEffect);


					bool drawRect = context->IsMouseButtonDown(ig::MouseButton::Middle);
					str = "A";
					strSize = r.MeasureString(str, *trimSDF);
					r.SetWorldMatrix(ig::Vector3(900, 0, 0), ig::Quaternion::Identity, ig::Vector3(4, 4, 1));
					x = 0;
					y = 0;
					if (drawRect) r.DrawRectangle(x, y, strSize.x, strSize.y, ig::Colors::Gray);
					r.DrawString(x, y, str, *trimSDF, ig::Colors::Green);

					str = "ABC1234567890.";
					strSize = r.MeasureString(str, *trimSDF);
					r.SetViewAndProjection3D(ig::Vector3(0, 0, 80), ig::Vector3(0, 0, -1), ig::Vector3(0, -1, 0),
						(float)context->GetWidth() / (float)context->GetHeight(), 90.0f);
					r.SetWorldMatrix(
						ig::Vector3(0, (-strSize.y * 0.5f) - 4, 0),
						ig::Quaternion::Euler(0, context->GetMouseX() * 0.007f, 0),
						ig::Vector3(1, 1, 1));
					x = -strSize.x / 2;
					y = 0;
					if (drawRect) r.DrawRectangle(x, y, strSize.x, strSize.y, ig::Colors::Gray);
					r.DrawString(x, y, str, *trimSDF, ig::Colors::Green);
					r.RestoreWorldMatrix();
					r.SetViewAndProjection(oldView, oldProj);
				}
				else if (currentDemo == 5) // Exotic texture formats
				{
					float x = 0;
					float y = 0;

					// 16-bit monochrome PNG
					{
						const char* formatName = ig::GetFormatName(mono16bitTexture->GetFormat());
						x = 470;
						y = 20;
						r.DrawString(x, y + (float)mono16bitTexture->GetHeight() + 2, ig::ToString(
							"Format: ", formatName,
							"\n[Left mouse button] = double brightness",
							"\n[Right mouse button] = draw 0-255 gradient rectangle for comparison"),
							*defaultFont, ig::Colors::Yellow);

						// The 16-bit PNG is drawn with twice as bright color to see if there is still any banding visible.
						ig::Color color = ig::Colors::White;
						if (context->IsMouseButtonDown(ig::MouseButton::Left)) color = ig::Color(2, 2, 2, 1);
						r.UsingBatch(batchFloatColorMono);
						r.UsingTexture(*mono16bitTexture);
						Vertex_ScaledSpriteFloatColor V[] =
						{
							ig::Vector2(x, y), // XY
							(float)mono16bitTexture->GetWidth(), // Width
							(float)mono16bitTexture->GetHeight(), // Height
							ig::FloatRect(0, 0, (float)mono16bitTexture->GetWidth(), (float)mono16bitTexture->GetHeight()), // UV
							color // Color
						};
						r.AddPrimitive(V);

						// Compare gradient of 16-bit grayscale PNG with gradient of rectangle to see if they match.
						// This is to confirm that the 16-bit PNG really is 16-bit.
						if (context->IsMouseButtonDown(ig::MouseButton::Right))
						{
							ig::Color32 colors2[] = { ig::Colors::Black, ig::Colors::White, ig::Colors::Black, ig::Colors::White };
							ig::FloatRect realGradientRect = ig::FloatRect(x, y - 8, x + 256, y + 8);
							ig::Vector2 p0 = ig::Vector2(realGradientRect.left, realGradientRect.top);
							ig::Vector2 p1 = ig::Vector2(realGradientRect.right, realGradientRect.top);
							ig::Vector2 p2 = ig::Vector2(realGradientRect.left, realGradientRect.bottom);
							ig::Vector2 p3 = ig::Vector2(realGradientRect.right, realGradientRect.bottom);
							r.DrawTriangle(p0, p1, p2, colors2[0], colors2[1], colors2[2]);
							r.DrawTriangle(p2, p1, p3, colors2[2], colors2[1], colors2[3]);
						}
					}

					// 16-bit dual channel PNG (Grayscale + Alpha)
					{
						const char* formatName = ig::GetFormatName(duo16bitTexture->GetFormat());
						y += 330;
						r.DrawString(x, y + (float)duo16bitTexture->GetHeight() + 2,
							ig::ToString("Format: ", formatName), *defaultFont, ig::Colors::Yellow);
						r.DrawTexture(*duo16bitTexture, x, y);

						// PNG with grayscale channel + alpha channel shows up as a 2 channel texture when drawn (Red and green).
						// This is expected and OK.
						// User can always make a custom batch type with custom pixel shader
						// to interpret the texture channels however they want.
					}

					// 8-bit monochrome
					{
						const char* formatName = ig::GetFormatName(defaultFont->GetTexture()->GetFormat());
						y += 150;
						r.DrawString(x, y + defaultFont->GetTexture()->GetHeight() + 2,
							ig::ToString("Format: ", formatName), *defaultFont, ig::Colors::Yellow);
						r.DrawTexture(*defaultFont->GetTexture(), x, y);
					}

					// HDR texture
					{
						float brightness = 1.0f;
						if (context->IsMouseButtonDown(ig::MouseButton::Left)) brightness = context->GetMouseX() * 0.001f;
						if (context->IsMouseButtonDown(ig::MouseButton::Right)) brightness = context->GetMouseX() * 0.1f;
						const char* formatName = ig::GetFormatName(hdrTexture->GetFormat());
						x = 10;
						y = 350;
						float width = 512 - 100;
						float height = 256 - 50;
						r.DrawString(x, y + height + 2, ig::ToString(
							"Format: ", formatName,
							"\n[Left/right mouse button & mouse position] = change brightness"),
							*defaultFont, ig::Colors::Yellow);
						ig::Color color = ig::Color(brightness, brightness, brightness);
						r.UsingBatch(batchFloatColor);
						r.UsingTexture(*hdrTexture);
						Vertex_ScaledSpriteFloatColor V =
						{
							ig::Vector2(x, y), // XY
							width, // Width
							height, // Height
							ig::FloatRect(0, 0, (float)hdrTexture->GetWidth(), (float)hdrTexture->GetHeight()), // UV
							color // Color
						};
						r.AddPrimitive(&V);
					}

					// 16-bit RGBA PNG
					{
						const char* formatName = ig::GetFormatName(rgba16bitTexture->GetFormat());
						x = 10;
						y = 130;
						float width = 128;
						float height = 128;
						r.DrawString(x, y + height + 2, ig::ToString(
							"Format: ", formatName,
							"\n[Middle mouse button] = Move to cursor"),
							*defaultFont, ig::Colors::Yellow);
						if (context->IsMouseButtonDown(ig::MouseButton::Middle))
						{
							x = (float)context->GetMouseX();
							y = (float)context->GetMouseY();
						}
						r.DrawTexture(*rgba16bitTexture, x, y, width, height);
					}
				}
				else if (currentDemo == 6)
				{
					const float spacing = 8;
					float quadSize = 32;
					float x = 570;
					float y = 140;

					r.DrawString(x, y - 50, "Drawing rectangles:", *trimSDF, ig::Colors::White);

					{
						r.UsingBatch(batchInstancedRects);
						ig::Vertex_Rect V = { ig::Vector2(x, y), quadSize, quadSize, ig::Colors::Green };
						r.AddPrimitive(&V);
						r.DrawString(x + 38, y + 7, "<-- Using instancing", *defaultFont, ig::Colors::White);
					}

					{
						y += quadSize + spacing;
						r.UsingBatch(batchRawRects);
						ig::Vertex_Rect V = { ig::Vector2(x, y), quadSize, quadSize, ig::Colors::Green };
						r.AddPrimitive(&V);
						r.DrawString(x + 38, y + 7, "<-- Using vertex pulling (Raw buffer)", *defaultFont, ig::Colors::White);
					}

					{
						y += quadSize + spacing;
						r.UsingBatch(batchStructuredRects);
						Vertex_Rect_Structured V = { ig::Vector2(x, y), quadSize, quadSize, ig::Colors::Green, 0 };
						r.AddPrimitive(&V);
						r.DrawString(x + 38, y + 7, "<-- Using vertex pulling (Structured buffer)", *defaultFont, ig::Colors::White);
					}

					{
						y += quadSize + spacing;
						r.UsingBatch(batchIndexedRects);
						ig::Vertex_XYC V[] =
						{
							// The four corners of the quad.
							ig::Vector2(x, y), ig::Colors::Green,
							ig::Vector2(x + quadSize, y), ig::Colors::Green,
							ig::Vector2(x, y + quadSize), ig::Colors::Green,
							ig::Vector2(x + quadSize, y + quadSize), ig::Colors::Green
						};
						r.AddPrimitive(&V);
						r.DrawString(x + 38, y + 7, "<-- Using an index buffer", *defaultFont, ig::Colors::White);
					}

					x = 100;
					y = 140;
					quadSize = 64;
					r.DrawString(x, y - 50, "Drawing sprites:", *trimSDF, ig::Colors::White);

					{
						r.UsingBatch(batchInstancedSprites);
						r.UsingTexture(*appleTexture);
						ig::Vertex_Sprite V = { ig::Vector2(x, y), (uint16_t)quadSize, (uint16_t)quadSize, 0, 0, ig::Colors::White };
						r.AddPrimitive(&V);
						r.DrawString(x + 72, y + 20, "<-- Using instancing", *defaultFont, ig::Colors::White);
					}
					{
						y += quadSize + spacing;
						r.UsingBatch(batchRawSprites);
						r.UsingTexture(*appleTexture);
						ig::Vertex_Sprite V = { ig::Vector2(x, y), (uint16_t)quadSize, (uint16_t)quadSize, 0, 0, ig::Colors::White };
						r.AddPrimitive(&V);
						r.DrawString(x + 72, y + 20, "<-- Using vertex pulling (Raw buffer)", *defaultFont, ig::Colors::White);
					}
					{
						y += quadSize + spacing;
						r.UsingBatch(batchStructuredSprites);
						r.UsingTexture(*appleTexture);
						Vertex_Sprite_Structured V = { ig::Vector2(x, y), (uint16_t)quadSize, (uint16_t)quadSize, 0, 0, ig::Colors::White, 0 };
						r.AddPrimitive(&V);
						r.DrawString(x + 72, y + 20, "<-- Using vertex pulling (Structured buffer)", *defaultFont, ig::Colors::White);
					}
					{
						y += quadSize + spacing;
						r.UsingBatch(batchIndexedSprites);
						r.UsingTexture(*appleTexture);
						ig::Vertex_XYCUV V[] =
						{
							ig::Vector2(x, y), ig::Colors::White, ig::Vector2(0, 0),
							ig::Vector2(x + quadSize, y), ig::Colors::White, ig::Vector2(1, 0),
							ig::Vector2(x, y + quadSize), ig::Colors::White, ig::Vector2(0, 1),
							ig::Vector2(x + quadSize, y + quadSize), ig::Colors::White, ig::Vector2(1, 1)
						};
						r.AddPrimitive(&V);
						r.DrawString(x + 72, y + 20, "<-- Using an index buffer", *defaultFont, ig::Colors::White);
					}
					{
						y += quadSize + spacing;
						struct Wobble
						{
							float animation;
							float intensityX;
							float intensityY;
						};
						Wobble wobble;
						wobble.animation = (float)tick * 0.55f;
						wobble.intensityX = 15.0f;
						wobble.intensityY = 0;
						if (context->IsMouseButtonDown(ig::MouseButton::Left))
						{
							wobble.intensityX = (float)context->GetMouseX() * 0.1f;
							wobble.intensityY = (float)context->GetMouseY() * 0.1f;
						}
						ig::Descriptor wobbleConstants = context->CreateTempConstant(&wobble, sizeof(wobble));
						r.UsingRenderConstants(wobbleConstants);
						r.UsingBatch(batchCustomPixelShaderSprites);
						r.UsingTexture(*appleTexture);
						ig::Vertex_Sprite V = { ig::Vector2(x, y), (uint16_t)quadSize, (uint16_t)quadSize, 0, 0, ig::Colors::White };
						r.AddPrimitive(&V);
						r.DrawString(x + 72, y + 20, "<-- Using custom pixel shader (Left mouse button to adjust wobble)",
							*defaultFont, ig::Colors::White);
					}
					{
						y += quadSize + spacing;
						if (context->IsMouseButtonDown(ig::MouseButton::Right))
						{
							x = (float)context->GetMouseX();
							y = (float)context->GetMouseY();
						}
						r.UsingBatch(batchCustomBlendSprites);
						r.UsingTexture(*appleTexture);
						ig::Vertex_Sprite V = { ig::Vector2(x, y), (uint16_t)quadSize, (uint16_t)quadSize, 0, 0, ig::Colors::White };
						r.AddPrimitive(&V);
						r.DrawString(x + 72, y + 20, "<-- Using custom blend state (Right mouse button to move)", *defaultFont, ig::Colors::White);
					}

				}

				buttonNext.Draw(r, *buttonTexture, *vegur);
				buttonPrev.Draw(r, *buttonTexture, *vegur);

				std::string topLeftStr = ig::ToString
				(
					"iglo v" IGLO_VERSION_STRING, " ", IGLO_GRAPHICS_API_STRING, ", FPS: ", mainloop.GetAverageFPS(), "\n",
					"BatchRenderer draw calls: ", r.GetDrawCallCount()
				);
				ig::Vector2 strPos(7, 2);
				ig::Vector2 shadowOffset = ig::Vector2(1, 1);
				r.DrawString(strPos + shadowOffset, topLeftStr, *vegur, ig::Colors::Black);
				r.DrawString(strPos, topLeftStr, *vegur, ig::Colors::White);

				std::string demoStr = ig::ToString
				(
					"Showing demo ", currentDemo + 1, "/", numDemos, "\n", demos[currentDemo]
				);
				strPos = ig::Vector2((float)buttonPrev.GetRect().left, (float)buttonPrev.GetRect().top - 55);
				r.DrawString(strPos + shadowOffset, demoStr, *vegur, ig::Colors::Black);
				r.DrawString(strPos, demoStr, *vegur, ig::Colors::White);
			}
			r.End();

			if (useMSAA)
			{
				cmd->AddTextureBarrier(*rendertarget, ig::SimpleBarrier::RenderTarget, ig::SimpleBarrier::ResolveSource);
				cmd->AddTextureBarrier(context->GetBackBuffer(), ig::SimpleBarrier::Discard, ig::SimpleBarrier::ResolveDest);
				cmd->FlushBarriers();

				cmd->ResolveTexture(*rendertarget, context->GetBackBuffer());

				cmd->AddTextureBarrier(context->GetBackBuffer(), ig::SimpleBarrier::ResolveDest, ig::SimpleBarrier::Present);
				cmd->FlushBarriers();
			}
			else
			{
				cmd->AddTextureBarrier(context->GetBackBuffer(), ig::SimpleBarrier::RenderTarget, ig::SimpleBarrier::Present);
				cmd->FlushBarriers();
			}
		}
		cmd->End();

		context->Submit(*cmd);
		context->Present();
	}

};

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShow)
#else
int main(int argc, char** argv)
#endif
{
	std::unique_ptr<App> app = std::make_unique<App>();
	app->Run();
	app = nullptr;
	return 0;
}
