#include "iglo.h"
#include "igloFont.h"
#include "igloBatchRenderer.h"
#include "igloMainLoop.h"

// Agility SDK path and version
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 715; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }

const std::string sampleName = "Hello cube";
const std::string resourceFolder = "resources/";
const std::string shaderFolder = "shaders/";

ig::IGLOContext context;
ig::CommandList cmd;
ig::MainLoop mainloop;
ig::BatchRenderer r;

ig::Font defaultFont;
ig::Pipeline pipeline;
ig::Texture renderTexture;
ig::Texture depthBuffer;
ig::Buffer matrixConstants;
ig::Texture stoneTexture;
ig::Sampler sampler;

struct Mesh
{
	ig::Buffer vertexBuffer;
	ig::Buffer indexBuffer;
};
Mesh cube;

ig::RenderTargetDesc renderTargetDesc = ig::RenderTargetDesc(
	{ ig::Format::BYTE_BYTE_BYTE_BYTE },
	ig::Format::DEPTHFORMAT_UINT24_BYTE,
	ig::MSAA::X16);

struct PushConstants
{
	uint32_t textureIndex = IGLO_UINT32_MAX;
	uint32_t samplerIndex = IGLO_UINT32_MAX;
	uint32_t constantsIndex = IGLO_UINT32_MAX;
};

struct MatrixConstants
{
	ig::Matrix4x4 viewProj;
	ig::Matrix4x4 world;
};

struct Vertex
{
	ig::Vector3 position;
	ig::Vector2 uv;
};

// Origin is in the center.
Mesh GenerateCubeMesh(const ig::IGLOContext& context, ig::CommandList& commandList, float cubeSize)
{
	float d = cubeSize * 0.5f;
	const uint32_t vertexCount = 48;
	Vertex vertices[vertexCount] =
	{
		ig::Vector3(d,-d,-d),  ig::Vector2(1,1), // -Z side (0)
		ig::Vector3(-d,-d,-d), ig::Vector2(0,1), // -Z side (1)
		ig::Vector3(d,d,-d),   ig::Vector2(1,0), // -Z side (2)
		ig::Vector3(-d,d,-d),  ig::Vector2(0,0), // -Z side (3)

		ig::Vector3(-d,-d,d),  ig::Vector2(1,1), // +Z side (0)
		ig::Vector3(d,-d,d),   ig::Vector2(0,1), // +Z side (1)
		ig::Vector3(-d,d,d),   ig::Vector2(1,0), // +Z side (2)
		ig::Vector3(d,d,d),    ig::Vector2(0,0), // +Z side (3)

		ig::Vector3(d,-d,d),   ig::Vector2(0,0), // -Y side (0)
		ig::Vector3(-d,-d,d),  ig::Vector2(1,0), // -Y side (1)
		ig::Vector3(d,-d,-d),  ig::Vector2(0,1), // -Y side (2)
		ig::Vector3(-d,-d,-d), ig::Vector2(1,1), // -Y side (3)

		ig::Vector3(-d,d,d),   ig::Vector2(0,0), // +Y side (0)
		ig::Vector3(d,d,d),    ig::Vector2(1,0), // +Y side (1)
		ig::Vector3(-d,d,-d),  ig::Vector2(0,1), // +Y side (2)
		ig::Vector3(d,d,-d),   ig::Vector2(1,1), // +Y side (3)

		ig::Vector3(-d,-d,-d), ig::Vector2(1,1), // -X side (0)
		ig::Vector3(-d,-d,d),  ig::Vector2(0,1), // -X side (1)
		ig::Vector3(-d,d,-d),  ig::Vector2(1,0), // -X side (2)
		ig::Vector3(-d,d,d),   ig::Vector2(0,0), // -X side (3)

		ig::Vector3(d,d,-d),   ig::Vector2(0,0), // +X side (0)
		ig::Vector3(d,d,d),    ig::Vector2(1,0), // +X side (1)
		ig::Vector3(d,-d,-d),  ig::Vector2(0,1), // +X side (2)
		ig::Vector3(d,-d,d),   ig::Vector2(1,1), // +X side (3)
	};

	const uint32_t indexCount = 48;
	uint16_t indices[indexCount] =
	{
		0, 1, 2, 2, 1, 3,
		0, 1, 2, 2, 1, 3,
		0, 1, 2, 2, 1, 3,
		0, 1, 2, 2, 1, 3,
		0, 1, 2, 2, 1, 3,
		0, 1, 2, 2, 1, 3,
	};
	for (uint32_t i = 0; i < 6; i++)
	{
		indices[i + (6 * 1)] += (4 * 1);
		indices[i + (6 * 2)] += (4 * 2);
		indices[i + (6 * 3)] += (4 * 3);
		indices[i + (6 * 4)] += (4 * 4);
		indices[i + (6 * 5)] += (4 * 5);
	}

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

void LoadSizeDependentResources()
{
	renderTexture.Load(context, context.GetWidth(), context.GetHeight(), renderTargetDesc.colorFormats.at(0),
		ig::TextureUsage::RenderTexture, renderTargetDesc.msaa);
	depthBuffer.Load(context, context.GetWidth(), context.GetHeight(), renderTargetDesc.depthFormat,
		ig::TextureUsage::DepthBuffer, renderTargetDesc.msaa);
}

void Start()
{
	// Cap MSAA to highest possible value
	renderTargetDesc.msaa = context.GetMaxMultiSampleCount(renderTargetDesc.colorFormats.at(0));

	cmd.Load(context, ig::CommandListType::Graphics);

	ig::VertexElement position(ig::Format::FLOAT_FLOAT_FLOAT, "POSITION");
	ig::VertexElement texcoord(ig::Format::FLOAT_FLOAT, "TEXCOORD");
	std::vector<ig::VertexElement> vertexLayout = { position, texcoord };

	if (!pipeline.LoadFromFile(context, shaderFolder + "VS_HelloCube.cso", shaderFolder + "PS_HelloCube.cso", "VSMain", "PSMain",
		vertexLayout, ig::Primitive::TriangleList,
		ig::DepthDesc::DepthAndStencilEnabled, ig::RasterizerDesc::BackCull, { ig::BlendDesc::StraightAlpha },
		renderTargetDesc))
	{
		PopupMessage("Failed to create pipeline state.", "Error", &context);
		mainloop.Quit();
		return;
	}

	LoadSizeDependentResources();

	sampler.Load(context, ig::SamplerDesc::SmoothRepeatSampler);

	matrixConstants.LoadAsShaderConstant(context, sizeof(MatrixConstants), ig::BufferUsage::Dynamic);

	cmd.Begin();
	{
		if (!stoneTexture.LoadFromFile(context, cmd, resourceFolder + "wood-planks.jpg"))
		{
			PopupMessage("Failed to load texture.", "Error", &context);
			mainloop.Quit();
			return;
		}

		cube = GenerateCubeMesh(context, cmd, 1.0f);

		r.Load(context, cmd, renderTargetDesc);
		defaultFont.LoadAsPrebaked(context, cmd, ig::GetDefaultFont());
	}
	cmd.End();
	context.WaitForCompletion(context.Submit(cmd));
}

void OnLoopExited()
{
	context.WaitForIdleDevice(); // Wait for GPU to finish all remaining work before resources get released
}

double tick = 0;
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
	else if (e.type == ig::EventType::Resize)
	{
		LoadSizeDependentResources();
	}
	else if (e.type == ig::EventType::KeyPress)
	{
		switch (e.key)
		{
		case ig::Key::Escape:
			mainloop.Quit();
			return;

		case ig::Key::Enter:
			if (context.IsKeyDown(ig::Key::LeftAlt)) context.ToggleFullscreen();
			break;

		case ig::Key::F11:
			context.ToggleFullscreen();
			break;
		}
	}
}

void Draw()
{
	cmd.Begin();
	{
		cmd.SetRenderTarget(&renderTexture, &depthBuffer);
		cmd.SetViewport((float)renderTexture.GetWidth(), (float)renderTexture.GetHeight());
		cmd.SetScissorRectangle(renderTexture.GetWidth(), renderTexture.GetHeight());
		cmd.ClearColor(renderTexture);
		cmd.ClearDepth(depthBuffer);
		cmd.SetPipeline(pipeline);

		// Generate matrices
		ig::Vector3 cameraPos = ig::Vector3(-1.5f, 1.25f, 0);
		ig::Vector3 cameraLookAt = ig::Vector3(0, 0, 0);
		ig::Vector3 cameraLookUp = ig::Vector3(0, 1, 0);
		float cameraAspectRatio = (float)context.GetWidth() / (float)context.GetHeight();
		float cameraFOV = 70.0f;
		float zNear = 0.5f;
		float zFar = 20.0f;
		ig::Matrix4x4 view = ig::Matrix4x4::LookToLH(cameraPos, cameraLookAt - cameraPos, cameraLookUp);
		ig::Matrix4x4 proj = ig::Matrix4x4::PerspectiveFovLH(cameraAspectRatio, cameraFOV, zNear, zFar);
		ig::Matrix4x4 world = ig::Matrix4x4::WorldTRS(
			ig::Vector3(0, 0, 0), // Translation
			ig::Quaternion::Euler(0, (float)tick * 0.4f, 0), // Rotation
			ig::Vector3(1, 1, 1)); // Scale

		// Update constants
		MatrixConstants data;
		data.viewProj = (proj * view).GetTransposed();
		data.world = world.GetTransposed();
		matrixConstants.SetDynamicData(&data);

		// This is how you bind textures and constants in SM 6.6 bindless
		PushConstants pushConstants;
		pushConstants.textureIndex = stoneTexture.GetDescriptor()->heapIndex;
		pushConstants.samplerIndex = sampler.GetDescriptor()->heapIndex;
		pushConstants.constantsIndex = matrixConstants.GetDescriptor()->heapIndex;
		cmd.SetPushConstants(&pushConstants, sizeof(pushConstants));

		// Draw cube
		cmd.SetVertexBuffer(cube.vertexBuffer);
		cmd.SetIndexBuffer(cube.indexBuffer);
		cmd.SetPrimitiveTopology(ig::Primitive::TriangleList);
		cmd.DrawIndexed(cube.indexBuffer.GetNumElements());

		r.Begin(cmd);
		{
			r.DrawString(4, 4, ig::ToString(sampleName, "\n", ig::igloVersion, " ", context.GetGraphicsAPIShortName(), "\n",
				"FPS: ", mainloop.GetAvarageFPS()), defaultFont, ig::Colors::Yellow);
		}
		r.End();

		cmd.AddTextureBarrier(context.GetBackBuffer(), ig::SimpleBarrier::Common, ig::SimpleBarrier::ResolveDest, false);
		cmd.AddTextureBarrier(renderTexture, ig::SimpleBarrier::RenderTarget, ig::SimpleBarrier::ResolveSource);
		cmd.FlushBarriers();

		cmd.ResolveTexture(renderTexture, context.GetBackBuffer());

		cmd.AddTextureBarrier(context.GetBackBuffer(), ig::SimpleBarrier::ResolveDest, ig::SimpleBarrier::Common, false);
		cmd.AddTextureBarrier(renderTexture, ig::SimpleBarrier::ResolveSource, ig::SimpleBarrier::RenderTarget);
		cmd.FlushBarriers();
	}
	cmd.End();

	context.Submit(cmd);
	context.Present();
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShow)
#endif
#ifdef __linux__
int main(int argc, char** argv)
#endif
{
	if (context.Load(
		ig::WindowSettings(sampleName, 640, 480, true, true),
		ig::RenderSettings(ig::PresentMode::Immediate, ig::Format::BYTE_BYTE_BYTE_BYTE)))
	{
		mainloop.Run(context, Start, OnLoopExited, Draw, Update, FixedUpdate, OnEvent);
	}
	return 0;
}
