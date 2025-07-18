#include "iglo.h"
#include "iglo_font.h"
#include "iglo_batch_renderer.h"
#include "iglo_screen_renderer.h"
#include "iglo_main_loop.h"
#include "iglo_camera.h"

#include "DemoCamera.h"
#include "LoadingScreenProgressBar.h"

#include <thread>

#ifdef IGLO_D3D12
// Agility SDK path and version
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 715; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
#endif

class App
{
public:
	void Run()
	{
		if (context.Load(
			ig::WindowSettings
			{
				.title = sampleName,
				.width = 1280,
				.height = 720,
				.resizable = true,
				.centered = true,
			},
			ig::RenderSettings
			{
				.presentMode = ig::PresentMode::Immediate,
			}))
		{
			mainloop.Run(context,
				std::bind(&App::Start, this),
				std::bind(&App::OnLoopExited, this),
				std::bind(&App::Draw, this),
				std::bind(&App::Update, this, std::placeholders::_1),
				std::bind(&App::FixedUpdate, this),
				std::bind(&App::OnEvent, this, std::placeholders::_1));
		}
	}

private:

	const std::string sampleName = "Camera and skybox";
	const std::string resourceFolder = "resources/";
	const std::string shaderFolder = "shaders/";

	ig::IGLOContext context;
	ig::CommandList cmd;
	ig::BatchRenderer r;
	ig::ScreenRenderer screenRenderer;
	ig::MainLoop mainloop;

	struct Mesh
	{
		ig::Buffer vertexBuffer;
		ig::Buffer indexBuffer;
	};

	struct Vertex
	{
		ig::Vector3 position;
		ig::Color32 color;
	};

	enum class SceneDrawMode
	{
		InstancedCubes = 0,
		Points,
	};

	static constexpr uint32_t MaxObjectCount = 10'000'000;
	std::vector<ig::Vector3> instancePositions;
	std::vector<Vertex> pointVertices;
	uint32_t objectDrawCount = 1000;
	bool msaaEnabled = true;
	bool skyboxEnabled = true;
	SceneDrawMode sceneDrawMode = SceneDrawMode::InstancedCubes;

	Mesh cube;
	Mesh skybox;
	ig::Buffer instancePosBuffer;
	ig::Buffer pointVertexBuffer;
	ig::Buffer XYZLines;
	ig::Texture skyboxCubemapTexture;

	ig::Font vegur;
	ig::Font trimSDF;

	ig::Sampler skyboxSampler;
	ig::Pipeline skyboxPipeline;
	ig::Pipeline skyboxPipelineMSAA;
	ig::Pipeline instancingPipeline;
	ig::Pipeline instancingPipelineMSAA;
	ig::Pipeline pointPipeline;
	ig::Pipeline pointPipelineMSAA;
	ig::Pipeline linePipeline;
	ig::Pipeline linePipelineMSAA;

	ig::Texture renderColor;
	ig::Texture renderDepth;
	ig::Texture resolved;
	ig::RenderTargetDesc renderDesc;
	ig::RenderTargetDesc renderDescMSAA;

	std::thread worker;
	ig::LoadingScreenProgressBar bar;
	bool generationIsComplete = false;
	std::atomic<uint32_t> generationProgress = 0;
	std::atomic<bool> threadShouldRun = true;

	ig::DemoCamera camera;

	// Origin is in the center.
	Mesh GenerateCubeMesh(const ig::IGLOContext& context, ig::CommandList& commandList, float cubeSize)
	{
		float d = cubeSize * 0.5f;
		const uint32_t vertexCount = 8;
		Vertex vertices[vertexCount] =
		{
			ig::Vector3(-d,-d,-d), ig::Color32(255, 255, 255),
			ig::Vector3(d,-d,-d), ig::Color32(0, 255, 255),
			ig::Vector3(-d,d,-d), ig::Color32(255, 0, 255),
			ig::Vector3(d,d,-d), ig::Color32(0, 0, 255),

			ig::Vector3(-d,-d, d), ig::Color32(255, 255, 0),
			ig::Vector3(d,-d, d), ig::Color32(0, 255, 0),
			ig::Vector3(-d,d, d), ig::Color32(255, 0, 0),
			ig::Vector3(d,d, d), ig::Color32(0, 0, 0),
		};

		const uint32_t indexCount = 48;
		uint16_t indices[indexCount] =
		{
			3, 1, 2, 2, 1, 0,
			4, 5, 6, 6, 5, 7,
			0, 1, 4, 4, 1, 5,
			6, 7, 2, 2, 7, 3,
			7, 5, 3, 3, 5, 1,
			4, 6, 0, 0, 6, 2,
		};

		Mesh out;
		out.vertexBuffer.LoadAsVertexBuffer(context, sizeof(Vertex), vertexCount, ig::BufferUsage::Default);
		out.indexBuffer.LoadAsIndexBuffer(context, ig::IndexFormat::UINT16, indexCount, ig::BufferUsage::Default);
		if (out.vertexBuffer.IsLoaded() && out.indexBuffer.IsLoaded())
		{
			out.vertexBuffer.SetData(commandList, vertices);
			out.indexBuffer.SetData(commandList, indices);
			return out;
		}
		else
		{
			ig::Print("Failed generating cube mesh.\n");
			return Mesh();
		}
	}

	// Origin is in the center.
	Mesh GenerateSkyboxMesh(const ig::IGLOContext& context, ig::CommandList& commandList, float cubeSize)
	{
		float d = cubeSize * 0.5f;
		const uint32_t vertexCount = 8;
		ig::Vector3 vertices[vertexCount] =
		{
			ig::Vector3(-d,-d,-d),
			ig::Vector3(d,-d,-d),
			ig::Vector3(-d,d,-d),
			ig::Vector3(d,d,-d),

			ig::Vector3(-d,-d, d),
			ig::Vector3(d,-d, d),
			ig::Vector3(-d,d, d),
			ig::Vector3(d,d, d),
		};

		const uint32_t indexCount = 48;
		uint16_t indices[indexCount] =
		{
			3, 1, 2, 2, 1, 0,
			4, 5, 6, 6, 5, 7,
			0, 1, 4, 4, 1, 5,
			6, 7, 2, 2, 7, 3,
			7, 5, 3, 3, 5, 1,
			4, 6, 0, 0, 6, 2,
		};

		Mesh out;
		out.vertexBuffer.LoadAsVertexBuffer(context, sizeof(ig::Vector3), vertexCount, ig::BufferUsage::Default);
		out.indexBuffer.LoadAsIndexBuffer(context, ig::IndexFormat::UINT16, indexCount, ig::BufferUsage::Default);
		if (out.vertexBuffer.IsLoaded() && out.indexBuffer.IsLoaded())
		{
			out.vertexBuffer.SetData(commandList, vertices);
			out.indexBuffer.SetData(commandList, indices);
			return out;
		}
		else
		{
			ig::Print("Failed generating skybox mesh.\n");
			return Mesh();
		}
	}

	void LoadRenderTargets()
	{
		ig::RenderTargetDesc& desc = msaaEnabled ? renderDescMSAA : renderDesc;
		uint32_t width = context.GetWidth();
		uint32_t height = context.GetHeight();
		renderColor.Load(context, width, height, desc.colorFormats[0], ig::TextureUsage::RenderTexture, desc.msaa);
		renderDepth.Load(context, width, height, desc.depthFormat, ig::TextureUsage::DepthBuffer, desc.msaa);
		resolved.Load(context, width, height, desc.colorFormats[0], ig::TextureUsage::Default);

		// Update camera aspect ratio
		camera.SetAspectRatio((float)renderColor.GetWidth() / (float)renderColor.GetHeight());
	}

	void OnEvent(ig::Event e)
	{
		if (e.type == ig::EventType::KeyPress)
		{
			switch (e.key)
			{
			default:
				break;

			case ig::Key::Comma:
				if (objectDrawCount >= 10) objectDrawCount /= 10;
				break;
			case ig::Key::Period:
				objectDrawCount *= 10;
				if (objectDrawCount > MaxObjectCount) objectDrawCount = MaxObjectCount;
				break;
			case ig::Key::F11:
				context.ToggleFullscreen();
				break;
			case ig::Key::Enter:
				if (context.IsKeyDown(ig::Key::LeftAlt)) context.ToggleFullscreen();
				break;
			case ig::Key::Escape:
				mainloop.Quit();
				return;
			case ig::Key::M:
				context.WaitForIdleDevice();
				msaaEnabled = !msaaEnabled;
				LoadRenderTargets();
				break;
			case ig::Key::N:
				sceneDrawMode = (SceneDrawMode)(((int)sceneDrawMode + 1) % 2);
				break;
			case ig::Key::B:
				skyboxEnabled = !skyboxEnabled;
				break;
			}
		}
		else if (e.type == ig::EventType::Resize)
		{
			LoadRenderTargets();
		}
		else if (e.type == ig::EventType::CloseRequest)
		{
			mainloop.Quit();
			return;
		}
	}

	void GenerateObjects()
	{
		for (generationProgress = 0; generationProgress < MaxObjectCount; generationProgress++)
		{
			uint32_t i = generationProgress;
			const float areaSize = -100;

			// Instance position
			instancePositions[i] = ig::Vector3(
				ig::Random::NextFloat(-areaSize, areaSize),
				ig::Random::NextFloat(-areaSize, areaSize),
				ig::Random::NextFloat(-areaSize, areaSize));

			// Point vertex
			const float cylinderRadius = 200;
			const float cylinderHeight = 200;
			ig::Vector2 pos2D = ig::Vector2(ig::Random::NextFloat(0, cylinderRadius), 0).GetRotated(ig::Random::NextFloat(0, 2.0f * (float)IGLO_PI));
			pointVertices[i].position = ig::Vector3(pos2D.x, ig::Random::NextFloat(-cylinderHeight / 2, cylinderHeight / 2), pos2D.y);
			pointVertices[i].color = ig::Random::NextUInt32();

			if (!threadShouldRun) return;
		}
	}

	void Start()
	{
		cmd.Load(context, ig::CommandListType::Graphics);
		screenRenderer.Load(context, context.GetBackBufferRenderTargetDesc());

		// Render target descriptions
		renderDesc =
		{
			.colorFormats = { ig::Format::BYTE_BYTE_BYTE_BYTE },
			.depthFormat = ig::Format::DEPTHFORMAT_FLOAT,
		};
		renderDescMSAA = renderDesc;
		renderDescMSAA.msaa = context.GetMaxMultiSampleCount(renderDesc.colorFormats[0]);

		// Load pipelines
		{
			skyboxSampler.Load(context, ig::SamplerDesc::SmoothRepeatSampler);

#ifdef IGLO_D3D12
			std::string fileExt = ".cso";
#else
			std::string fileExt = "_SPIRV.cso";
#endif

			ig::ReadFileResult skyboxVS = ig::ReadFile(shaderFolder + "VS_SkyboxShader" + fileExt);
			ig::ReadFileResult skyboxPS = ig::ReadFile(shaderFolder + "PS_SkyboxShader" + fileExt);
			ig::ReadFileResult regularVS = ig::ReadFile(shaderFolder + "VS_RegularShader" + fileExt);
			ig::ReadFileResult regularPS = ig::ReadFile(shaderFolder + "PS_RegularShader" + fileExt);
			ig::ReadFileResult instancingVS = ig::ReadFile(shaderFolder + "VS_InstancingShader" + fileExt);
			ig::ReadFileResult instancingPS = ig::ReadFile(shaderFolder + "PS_InstancingShader" + fileExt);

			if (!skyboxVS.success || !skyboxPS.success ||
				!regularVS.success || !regularPS.success ||
				!instancingVS.success || !instancingPS.success)
			{
				PopupMessage("Failed to read shader binary files from folder: " + shaderFolder, "Error", &context);
				mainloop.Quit();
				return;
			}

			ig::VertexElement position(ig::Format::FLOAT_FLOAT_FLOAT, "POSITION", 0, 0, ig::InputClass::PerVertex);
			ig::VertexElement color(ig::Format::BYTE_BYTE_BYTE_BYTE, "COLOR", 0, 0, ig::InputClass::PerVertex);
			ig::VertexElement instancePos(ig::Format::FLOAT_FLOAT_FLOAT, "INSTANCEPOS", 0, 1, ig::InputClass::PerInstance, 1);

			ig::PipelineDesc skyboxDesc;
			skyboxDesc.VS = ig::Shader(skyboxVS.fileContent, "VSMain");
			skyboxDesc.PS = ig::Shader(skyboxPS.fileContent, "PSMain");
			skyboxDesc.blendStates = { ig::BlendDesc::BlendDisabled };
			skyboxDesc.depthState = ig::DepthDesc::DepthDisabled;
			skyboxDesc.rasterizerState = ig::RasterizerDesc::NoCull;
			skyboxDesc.primitiveTopology = ig::PrimitiveTopology::TriangleList;
			skyboxDesc.vertexLayout = { position };
			skyboxDesc.renderTargetDesc = renderDesc;
			skyboxPipeline.Load(context, skyboxDesc);
			skyboxDesc.renderTargetDesc = renderDescMSAA;
			skyboxPipelineMSAA.Load(context, skyboxDesc);

			ig::PipelineDesc instancingDesc;
			instancingDesc.VS = ig::Shader(instancingVS.fileContent, "VSMain");
			instancingDesc.PS = ig::Shader(instancingPS.fileContent, "PSMain");
			instancingDesc.blendStates = { ig::BlendDesc::StraightAlpha };
			instancingDesc.depthState = ig::DepthDesc::DepthEnabled;
			instancingDesc.rasterizerState = ig::RasterizerDesc::BackCull;
			instancingDesc.primitiveTopology = ig::PrimitiveTopology::TriangleList;
			instancingDesc.vertexLayout = { position, color, instancePos };
			instancingDesc.renderTargetDesc = renderDesc;
			instancingPipeline.Load(context, instancingDesc);
			instancingDesc.renderTargetDesc = renderDescMSAA;
			instancingPipelineMSAA.Load(context, instancingDesc);

			ig::PipelineDesc regularDesc = instancingDesc;
			regularDesc.VS = ig::Shader(regularVS.fileContent, "VSMain");
			regularDesc.PS = ig::Shader(regularPS.fileContent, "PSMain");
			regularDesc.primitiveTopology = ig::PrimitiveTopology::PointList;
			regularDesc.vertexLayout = { position, color };
			regularDesc.renderTargetDesc = renderDesc;
			pointPipeline.Load(context, regularDesc);
			regularDesc.primitiveTopology = ig::PrimitiveTopology::LineList;
			linePipeline.Load(context, regularDesc);
			regularDesc.primitiveTopology = ig::PrimitiveTopology::PointList;
			regularDesc.renderTargetDesc = renderDescMSAA;
			pointPipelineMSAA.Load(context, regularDesc);
			regularDesc.primitiveTopology = ig::PrimitiveTopology::LineList;
			linePipelineMSAA.Load(context, regularDesc);
		}

		// Load resources
		{
			instancePosBuffer.LoadAsVertexBuffer(context, sizeof(ig::Vector3), MaxObjectCount, ig::BufferUsage::Default);
			pointVertexBuffer.LoadAsVertexBuffer(context, sizeof(Vertex), MaxObjectCount, ig::BufferUsage::Default);

			if (!vegur.LoadFromFile(context, resourceFolder + "Vegur-Regular.otf", 18) ||
				!trimSDF.LoadFromFile(context, resourceFolder + "trim.ttf", 17, ig::FontSettings(ig::FontType::SDF)))
			{
				ig::PopupMessage("Failed to load one or more fonts from: " + resourceFolder, "Error", &context);
				mainloop.Quit();
				return;
			}

			cmd.Begin();
			{
				r.Load(context, cmd, context.GetBackBufferRenderTargetDesc());

				cube = GenerateCubeMesh(context, cmd, 1.0f);
				skybox = GenerateSkyboxMesh(context, cmd, 10.0f);

				{
					const uint32_t numVertices = 6;
					Vertex vertices[numVertices] =
					{
						ig::Vector3(0, 0, 0), ig::Colors::Red, ig::Vector3(10000, 0, 0), ig::Colors::Red,
						ig::Vector3(0, 0, 0), ig::Colors::Green, ig::Vector3(0, 10000, 0), ig::Colors::Green,
						ig::Vector3(0, 0, 0), ig::Colors::Blue, ig::Vector3(0, 0, 10000), ig::Colors::Blue,
					};
					XYZLines.LoadAsVertexBuffer(context, sizeof(Vertex), numVertices, ig::BufferUsage::Default);
					XYZLines.SetData(cmd, vertices);
				}

				skyboxCubemapTexture.LoadFromFile(context, cmd, resourceFolder + "cubemap.dds", false);
			}
			cmd.End();
			context.WaitForCompletion(context.Submit(cmd));
		}

		// Begin generating objects
		instancePositions.resize(MaxObjectCount, {});
		pointVertices.resize(MaxObjectCount, {});
		worker = std::thread(std::bind(&App::GenerateObjects, this));
		bar.SetCurrentTask("Generating objects...");

		LoadRenderTargets();
	}

	void OnLoopExited()
	{
		threadShouldRun = false;
		worker.join();
		context.WaitForIdleDevice();
	}

	void Update(double elapsedSeconds)
	{
		camera.Update(context, elapsedSeconds);
	}

	void FixedUpdate()
	{
	}

	void Draw()
	{
		if (generationProgress < MaxObjectCount)
		{
			float floatProgress = (float)generationProgress / (float)MaxObjectCount;
			std::string strProgress = ig::ToString(generationProgress, "/", MaxObjectCount);
			bar.DrawAndPresent(context, cmd, r, vegur, strProgress, floatProgress);
			ig::BasicSleep(10); // Lower GPU and CPU usage on this thread while the other thread generates instance data.
			return;
		}
		else
		{
			// Finished generating object data
			if (!generationIsComplete)
			{
				generationIsComplete = true;
				cmd.Begin();
				{
					// Upload data to GPU
					instancePosBuffer.SetData(cmd, instancePositions.data());
					pointVertexBuffer.SetData(cmd, pointVertices.data());
				}
				cmd.End();
				context.WaitForCompletion(context.Submit(cmd));
			}
		}

		cmd.Begin();
		{
			cmd.AddTextureBarrier(renderColor, ig::SimpleBarrier::Discard, ig::SimpleBarrier::RenderTarget);
			cmd.AddTextureBarrier(renderDepth, ig::SimpleBarrier::Discard, ig::SimpleBarrier::DepthStencil);
			cmd.FlushBarriers();

			cmd.SetRenderTarget(&renderColor, &renderDepth);
			cmd.SetViewport((float)renderColor.GetWidth(), (float)renderColor.GetHeight());
			cmd.SetScissorRectangle(renderColor.GetWidth(), renderColor.GetHeight());
			if (!skyboxEnabled) cmd.ClearColor(renderColor);
			cmd.ClearDepth(renderDepth);

			struct PushConstants
			{
				ig::Matrix4x4 viewProj; // For all shaders
				ig::Vector3 cameraPos; // For skybox
				float padding = 0;
				uint32_t textureIndex = IGLO_UINT32_MAX; // For skybox
				uint32_t samplerIndex = IGLO_UINT32_MAX; // For skybox
			};
			PushConstants pushConstants;
			pushConstants.viewProj = camera.GetViewProjMatrix().GetTransposed();
			pushConstants.cameraPos = camera.GetPosition();
			pushConstants.textureIndex = skyboxCubemapTexture.GetDescriptor()->heapIndex;
			pushConstants.samplerIndex = skyboxSampler.GetDescriptor()->heapIndex;
			cmd.SetPushConstants(&pushConstants, sizeof(pushConstants));

			// Draw skybox
			if (skyboxEnabled)
			{
				cmd.SetPipeline(msaaEnabled ? skyboxPipelineMSAA : skyboxPipeline);
				cmd.SetVertexBuffer(skybox.vertexBuffer);
				cmd.SetIndexBuffer(skybox.indexBuffer);
				cmd.DrawIndexed(skybox.indexBuffer.GetNumElements());
			}

			// Draw the XYZ lines
			cmd.SetPipeline(msaaEnabled ? linePipelineMSAA : linePipeline);
			cmd.SetVertexBuffer(XYZLines);
			cmd.Draw(XYZLines.GetNumElements());

			if (sceneDrawMode == SceneDrawMode::InstancedCubes)
			{
				// Draw instanced cubes
				cmd.SetPipeline(msaaEnabled ? instancingPipelineMSAA : instancingPipeline);
				cmd.SetVertexBuffer(cube.vertexBuffer, 0);
				cmd.SetVertexBuffer(instancePosBuffer, 1);
				cmd.SetIndexBuffer(cube.indexBuffer);
				cmd.DrawIndexedInstanced(cube.indexBuffer.GetNumElements(), objectDrawCount);
			}
			else if (sceneDrawMode == SceneDrawMode::Points)
			{
				// Draw points
				cmd.SetPipeline(msaaEnabled ? pointPipelineMSAA : pointPipeline);
				cmd.SetVertexBuffer(pointVertexBuffer);
				cmd.Draw(objectDrawCount);
			}

			if (msaaEnabled)
			{
				cmd.AddTextureBarrier(renderColor, ig::SimpleBarrier::RenderTarget, ig::SimpleBarrier::ResolveSource);
				cmd.AddTextureBarrier(resolved, ig::SimpleBarrier::Discard, ig::SimpleBarrier::ResolveDest);
				cmd.FlushBarriers();

				cmd.ResolveTexture(renderColor, resolved);

				cmd.AddTextureBarrier(resolved, ig::SimpleBarrier::ResolveDest, ig::SimpleBarrier::PixelShaderResource);
				cmd.FlushBarriers();
			}
			else
			{
				cmd.AddTextureBarrier(renderColor, ig::SimpleBarrier::RenderTarget, ig::SimpleBarrier::PixelShaderResource);
				cmd.FlushBarriers();
			}

			cmd.AddTextureBarrier(context.GetBackBuffer(), ig::SimpleBarrier::Discard, ig::SimpleBarrier::RenderTarget);
			cmd.FlushBarriers();

			cmd.SetRenderTarget(&context.GetBackBuffer());
			cmd.SetViewport((float)context.GetWidth(), (float)context.GetHeight());
			cmd.SetScissorRectangle(context.GetWidth(), context.GetHeight());

			// Draw the render texture onto the backbuffer
			screenRenderer.DrawFullscreenQuad(cmd, msaaEnabled ? resolved : renderColor, context.GetBackBuffer());

			r.Begin(cmd);
			{
				r.SetSamplerToPixelatedTextures();

				// Draw text
				std::string str = ig::ToString
				(
					sampleName, "\n",
					"iglo v" IGLO_VERSION_STRING " " IGLO_GRAPHICS_API_STRING "\n",
					"FPS: ", mainloop.GetAvarageFPS(), "\n",
					"Yaw: ", camera.GetYaw(), "\n",
					"Pitch: ", camera.GetPitch(), "\n",
					"Roll: ", camera.GetRoll(), "\n",
					"MSAA X", (int)renderColor.GetMSAA(), " [M]\n",
					"Objects: ", objectDrawCount, " [,][.]\n",
					"Drawing ", (sceneDrawMode == SceneDrawMode::InstancedCubes ? "instanced cubes" : "points"), " [N]\n"
					"Skybox ", (skyboxEnabled ? "enabled" : "disabled"), " [B]\n"
				);

				ig::SDFEffect sdfEffect;
				sdfEffect.sdfEffectFlags = (uint32_t)ig::SDFEffectFlags::Glow | (uint32_t)ig::SDFEffectFlags::Outline;
				sdfEffect.outlineThickness = 0.25f;
				r.SetSDFEffect(sdfEffect);

				ig::Vector2 strPos = ig::Vector2(7, 2);
				r.DrawString(strPos, str, trimSDF, ig::Colors::Yellow);

				if (camera.GetPosition() == ig::Vector3(0, 0, 0))
				{
					const std::string helperText =
						"Hold and drag mouse: rotate camera\n"
						"[Arrow keys][WASD][Q, E][Shift][Space]: move camera\n"
						"[Middle mouse]: reset camera\n"
						"[Right mouse]: rotate camera around scene";

					ig::Vector2 strSize = r.MeasureString(helperText, trimSDF);
					float x = floorf(((float)context.GetWidth() / 2.0f) - (strSize.x / 2.0f));
					float y = floorf((float)context.GetHeight() - (strSize.y + 15.0f));
					r.DrawString(x, y, helperText, trimSDF, ig::Colors::White);
				}

			}
			r.End();

			cmd.AddTextureBarrier(context.GetBackBuffer(), ig::SimpleBarrier::RenderTarget, ig::SimpleBarrier::Present);
			cmd.FlushBarriers();
		}
		cmd.End();

		context.Submit(cmd);
		context.Present();
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
