﻿#include "iglo.h"
#include "igloFont.h"
#include "igloBatchRenderer.h"
#include "igloMainLoop.h"

// Agility SDK path and version
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 715; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }

const std::string sampleName = "Lighting and shadows";
const std::string resourceFolder = "resources/";
const std::string shaderFolder = "shaders/";

ig::IGLOContext context;
ig::CommandList cmd;
ig::MainLoop mainloop;
ig::BatchRenderer r;

// Shader constants
ig::Buffer lightConstants;
ig::Buffer cameraConstants;

// Pipelines
ig::Pipeline pipelineLightAndShadow;
ig::Pipeline pipelineLightAndShadowMSAA;
ig::Pipeline pipelineDepth;
ig::Sampler colorSampler;
ig::Sampler depthSampler;

// Rendertargets
ig::Texture sceneRender;
ig::Texture sceneDepth;
ig::Texture sceneResolved;
ig::Texture shadowMap;
ig::RenderTargetDesc sceneDesc;
ig::RenderTargetDesc sceneDescMSAA;
ig::RenderTargetDesc shadowMapDesc;

struct Mesh
{
	ig::Buffer vertexBuffer;
	ig::Buffer indexBuffer;
};

struct ModelConstants
{
	ig::Matrix4x4 world;
};

struct Model
{
	Mesh mesh;
	ig::Texture texture;
	ModelConstants constantData;
	ig::Buffer constantBuffer;
};

// Models
Model ground;
Model cube;
Model sphere;

// Fonts
ig::Font defaultFont;
ig::Font trimSDF;

// Parameters
const float cameraFOV = 50.0f;
const float cameraZNear = 0.5f;
const float cameraZFar = 20.0f;
const float lightFOV = 90.0f;
const float lightZNear = 0.25f;
const float lightZFar = 40.0f;
const uint32_t shadowMapResolution = 1024 * 4;
const float slopedScaleDepthBias = 1.0f; // Prevents stripes from forming in the shadow at low angles
const ig::Color backgroundColor = ig::Color(0.1f, 0.1f, 0.1f);
const ig::Color lightAmbientColor = ig::Color(0.15f, 0.15f, 0.15f);
const ig::Color lightDiffuseColor = ig::Colors::White;
const bool useStencil = false;

// User settings
float lightDistance = 10.5f; //10.5f;
float lightElevation = 0.5f; //0.5f;
bool lightRotates = true;
bool enableMSAA = true;

struct PushConstants
{
	uint32_t colorTextureIndex = IGLO_UINT32_MAX;
	uint32_t depthTextureIndex = IGLO_UINT32_MAX;
	uint32_t colorSamplerIndex = IGLO_UINT32_MAX;
	uint32_t depthSamplerIndex = IGLO_UINT32_MAX;
	uint32_t lightConstantsIndex = IGLO_UINT32_MAX;
	uint32_t cameraConstantsIndex = IGLO_UINT32_MAX;
	uint32_t modelConstantsIndex = IGLO_UINT32_MAX;
};

struct LightConstants
{
	ig::Matrix4x4 viewAndProj;
	ig::Color diffuseColor;
	ig::Color ambientColor;
	ig::Vector3 position;
};
LightConstants light;

struct CameraConstants
{
	ig::Matrix4x4 viewAndProj;
};
CameraConstants camera;

struct Vertex
{
	ig::Vector3 position;
	ig::Vector2 uv;
	ig::Vector3 normal;
};

// Origin is in the center.
Mesh GenerateCubeMesh(const ig::IGLOContext& context, ig::CommandList& commandList, float cubeSize)
{
	float d = cubeSize * 0.5f;
	const uint32_t vertexCount = 48;
	Vertex vertices[vertexCount] =
	{
		ig::Vector3(d,-d,-d),  ig::Vector2(1,1), ig::Vector3(0,0,-1), // -Z side (0)
		ig::Vector3(-d,-d,-d), ig::Vector2(0,1), ig::Vector3(0,0,-1), // -Z side (1)
		ig::Vector3(d,d,-d),   ig::Vector2(1,0), ig::Vector3(0,0,-1), // -Z side (2)
		ig::Vector3(-d,d,-d),  ig::Vector2(0,0), ig::Vector3(0,0,-1), // -Z side (3)

		ig::Vector3(-d,-d,d),  ig::Vector2(1,1), ig::Vector3(0,0,1), // +Z side (0)
		ig::Vector3(d,-d,d),   ig::Vector2(0,1), ig::Vector3(0,0,1), // +Z side (1)
		ig::Vector3(-d,d,d),   ig::Vector2(1,0), ig::Vector3(0,0,1), // +Z side (2)
		ig::Vector3(d,d,d),    ig::Vector2(0,0), ig::Vector3(0,0,1), // +Z side (3)

		ig::Vector3(d,-d,d),   ig::Vector2(0,0), ig::Vector3(0,-1,0), // -Y side (0)
		ig::Vector3(-d,-d,d),  ig::Vector2(1,0), ig::Vector3(0,-1,0), // -Y side (1)
		ig::Vector3(d,-d,-d),  ig::Vector2(0,1), ig::Vector3(0,-1,0), // -Y side (2)
		ig::Vector3(-d,-d,-d), ig::Vector2(1,1), ig::Vector3(0,-1,0), // -Y side (3)

		ig::Vector3(-d,d,d),   ig::Vector2(0,0), ig::Vector3(0,1,0), // +Y side (0)
		ig::Vector3(d,d,d),    ig::Vector2(1,0), ig::Vector3(0,1,0), // +Y side (1)
		ig::Vector3(-d,d,-d),  ig::Vector2(0,1), ig::Vector3(0,1,0), // +Y side (2)
		ig::Vector3(d,d,-d),   ig::Vector2(1,1), ig::Vector3(0,1,0), // +Y side (3)

		ig::Vector3(-d,-d,-d), ig::Vector2(1,1), ig::Vector3(-1,0,0), // -X side (0)
		ig::Vector3(-d,-d,d),  ig::Vector2(0,1), ig::Vector3(-1,0,0), // -X side (1)
		ig::Vector3(-d,d,-d),  ig::Vector2(1,0), ig::Vector3(-1,0,0), // -X side (2)
		ig::Vector3(-d,d,d),   ig::Vector2(0,0), ig::Vector3(-1,0,0), // -X side (3)

		ig::Vector3(d,d,-d),   ig::Vector2(0,0), ig::Vector3(1,0,0), // +X side (0)
		ig::Vector3(d,d,d),    ig::Vector2(1,0), ig::Vector3(1,0,0), // +X side (1)
		ig::Vector3(d,-d,-d),  ig::Vector2(0,1), ig::Vector3(1,0,0), // +X side (2)
		ig::Vector3(d,-d,d),   ig::Vector2(1,1), ig::Vector3(1,0,0), // +X side (3)
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

// Origin is in the center.
Mesh GeneratePlaneMesh(const ig::IGLOContext& context, ig::CommandList& commandList, float width, float height)
{
	// Clockwise culling is used
	float w = width / 2;
	float h = height / 2;
	float W = width; // Using capital letters for texture coordinates
	float H = height;

	const uint32_t vertexCount = 4;
	Vertex vertices[vertexCount] =
	{
		ig::Vector3(-w,0,h),  ig::Vector2(W,0), ig::Vector3(0,1,0), // +Y side (0)
		ig::Vector3(w,0,h),   ig::Vector2(0,0), ig::Vector3(0,1,0), // +Y side (1)
		ig::Vector3(-w,0,-h), ig::Vector2(W,H), ig::Vector3(0,1,0), // +Y side (2)
		ig::Vector3(w,0,-h),  ig::Vector2(0,H), ig::Vector3(0,1,0), // +Y side (3)
	};

	const uint32_t indexCount = 6;
	uint16_t indices[indexCount] = { 0, 1, 2, 2, 1, 3 };

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
		ig::Print("Failed generating plane mesh.\n");
		return Mesh();
	}
}

// Origin is in the center.
Mesh GenerateSphereMesh(const ig::IGLOContext& context, ig::CommandList& commandList, float radius, uint32_t latitudeSegments, uint32_t longitudeSegments)
{
	if ((size_t)latitudeSegments * longitudeSegments >= (size_t)IGLO_UINT16_MAX) throw std::invalid_argument("Too many vertices!");

	// Step 1: Generate vertices
	std::vector<Vertex> vertices;
	vertices.reserve((size_t)latitudeSegments * longitudeSegments);
	for (uint32_t i = 0; i <= latitudeSegments; i++)
	{
		float theta = (float)i * (float)IGLO_PI / (float)latitudeSegments;
		float sinTheta = sin(theta);
		float cosTheta = cos(theta);

		for (uint32_t j = 0; j <= longitudeSegments; j++)
		{
			float phi = (float)j * 2.0f * (float)IGLO_PI / (float)longitudeSegments;
			float sinPhi = sin(phi);
			float cosPhi = cos(phi);

			ig::Vector3 position(
				radius * sinTheta * cosPhi,
				radius * cosTheta,
				radius * sinTheta * sinPhi);

			ig::Vector3 normal(
				sinTheta * cosPhi,
				cosTheta,
				sinTheta * sinPhi);

			ig::Vector2 uv(
				(float)j / (float)longitudeSegments,
				(float)i / (float)latitudeSegments);

			vertices.push_back({ position, uv, normal });
		}
	}

	// Step 2: Generate indices that form quads (each quad is 2 triangles)
	std::vector<uint16_t> indices;
	for (uint32_t i = 0; i <= latitudeSegments; i++)
	{
		for (uint32_t j = 0; j <= longitudeSegments; j++)
		{
			if (i < latitudeSegments && j < longitudeSegments)
			{
				uint32_t first = i * ((uint32_t)longitudeSegments + 1) + j;
				uint32_t second = first + longitudeSegments + 1;

				indices.push_back((uint16_t)first);
				indices.push_back((uint16_t)second + 1);
				indices.push_back((uint16_t)second);

				indices.push_back((uint16_t)first);
				indices.push_back((uint16_t)first + 1);
				indices.push_back((uint16_t)second + 1);
			}
		}
	}

	Mesh out;
	out.vertexBuffer.LoadAsVertexBuffer(context, sizeof(Vertex), (uint32_t)vertices.size(), ig::BufferUsage::Default);
	out.indexBuffer.LoadAsIndexBuffer(context, ig::IndexFormat::UINT16, (uint32_t)indices.size(), ig::BufferUsage::Default);
	if (out.vertexBuffer.IsLoaded() && out.indexBuffer.IsLoaded())
	{
		out.vertexBuffer.SetData(commandList, vertices.data());
		out.indexBuffer.SetData(commandList, indices.data());
		return out;
	}
	else
	{
		ig::Print("Failed generating sphere mesh.\n");
		return Mesh();
	}
}

void InitRenderTargetDescriptions()
{
	// The main scene
	{
		ig::Format colorFormat = ig::Format::FLOAT_FLOAT_FLOAT_FLOAT;
		ig::Format depthFormat = useStencil ? ig::Format::DEPTHFORMAT_FLOAT_BYTE : ig::Format::DEPTHFORMAT_FLOAT;
		ig::MSAA msaaDisabled = ig::MSAA::Disabled;
		ig::MSAA msaaEnabled = context.GetMaxMultiSampleCount(colorFormat);
		sceneDesc = ig::RenderTargetDesc({ colorFormat }, depthFormat, msaaDisabled);
		sceneDescMSAA = ig::RenderTargetDesc({ colorFormat }, depthFormat, msaaEnabled);
	}

	// Shadow map
	shadowMapDesc = ig::RenderTargetDesc({}, ig::Format::DEPTHFORMAT_FLOAT, ig::MSAA::Disabled);
}

void LoadRenderTargets()
{
	// The main scene
	{
		ig::RenderTargetDesc& desc = enableMSAA ? sceneDescMSAA : sceneDesc;

		uint32_t width = context.GetWidth();
		uint32_t height = context.GetHeight();

		ig::ClearValue clearValue;
		clearValue.color = backgroundColor;

		sceneRender.Load(context, width, height, desc.colorFormats.at(0), ig::TextureUsage::RenderTexture, desc.msaa, 1, 1, false, clearValue);
		sceneDepth.Load(context, width, height, desc.depthFormat, ig::TextureUsage::DepthBuffer, desc.msaa);

		// The rendering code expects this texture to start with a certain barrier layout.
		ig::BarrierLayout initLayout = ig::BarrierLayout::_GraphicsQueue_ShaderResource;
		sceneResolved.Load(context, width, height, desc.colorFormats.at(0), ig::TextureUsage::Default, ig::MSAA::Disabled,
			1, 1, false, ig::ClearValue(), &initLayout);
	}

	// Shadow map
	shadowMap.Load(context, shadowMapResolution, shadowMapResolution, shadowMapDesc.depthFormat, ig::TextureUsage::DepthBuffer, shadowMapDesc.msaa);
}

void Start()
{
	cmd.Load(context, ig::CommandListType::Graphics);
	InitRenderTargetDescriptions();
	LoadRenderTargets();

	// Load shader constant buffers
	lightConstants.LoadAsShaderConstant(context, sizeof(LightConstants), ig::BufferUsage::Dynamic);
	cameraConstants.LoadAsShaderConstant(context, sizeof(CameraConstants), ig::BufferUsage::Dynamic);
	ground.constantBuffer.LoadAsShaderConstant(context, sizeof(ModelConstants), ig::BufferUsage::Dynamic);
	cube.constantBuffer.LoadAsShaderConstant(context, sizeof(ModelConstants), ig::BufferUsage::Dynamic);
	sphere.constantBuffer.LoadAsShaderConstant(context, sizeof(ModelConstants), ig::BufferUsage::Dynamic);

	// Load pipelines
	{
		ig::ReadFileResult sceneVS = ig::ReadFile(shaderFolder + "VS_LightingAndShadows.cso");
		ig::ReadFileResult scenePS = ig::ReadFile(shaderFolder + "PS_LightingAndShadows.cso");
		ig::ReadFileResult depthVS = ig::ReadFile(shaderFolder + "VS_Depth.cso");

		if (!sceneVS.success || !scenePS.success || !depthVS.success)
		{
			PopupMessage("Failed to read shader binary files from folder: " + shaderFolder, "Error", &context);
			mainloop.Quit();
			return;
		}

		ig::VertexElement position(ig::Format::FLOAT_FLOAT_FLOAT, "POSITION");
		ig::VertexElement texcoord(ig::Format::FLOAT_FLOAT, "TEXCOORD");
		ig::VertexElement normal(ig::Format::FLOAT_FLOAT_FLOAT, "NORMAL");

		ig::PipelineDesc desc;
		desc.VS = ig::Shader(sceneVS.fileContent, "LightingAndShadowsVS");
		desc.PS = ig::Shader(scenePS.fileContent, "LightingAndShadowsPS");
		desc.vertexLayout = { position, texcoord, normal };
		desc.primitiveTopology = ig::Primitive::TriangleList;
		desc.blendStates = { ig::BlendDesc::StraightAlpha };
		desc.depthState = useStencil ? ig::DepthDesc::DepthAndStencilEnabled : ig::DepthDesc::DepthEnabled;
		desc.rasterizerState = ig::RasterizerDesc::BackCull;
		desc.rasterizerState.slopeScaledDepthBias = slopedScaleDepthBias;
		desc.renderTargetDesc = sceneDesc;
		pipelineLightAndShadow.Load(context, desc);

		desc.renderTargetDesc = sceneDescMSAA;
		pipelineLightAndShadowMSAA.Load(context, desc);

		desc.renderTargetDesc = shadowMapDesc;
		desc.PS = ig::Shader();
		desc.VS = ig::Shader(depthVS.fileContent, "DepthVS");
		desc.depthState = ig::DepthDesc::DepthEnabled;
		pipelineDepth.Load(context, desc);
	}

	// Load samplers
	{
		colorSampler.Load(context, ig::SamplerDesc::SmoothRepeatSampler);

		ig::SamplerDesc desc;
		desc.filter = ig::TextureFilter::_Comparison_Bilinear;
		desc.wrapU = ig::TextureWrapMode::BorderColor;
		desc.wrapV = ig::TextureWrapMode::BorderColor;
		desc.wrapW = ig::TextureWrapMode::BorderColor;
		desc.comparisonFunc = ig::ComparisonFunc::Less;
		desc.borderColor = ig::Colors::Black;
		depthSampler.Load(context, desc);

	}

	// Load resources
	{
		if (!trimSDF.LoadFromFile(context, resourceFolder + "trim.ttf", 20, ig::FontSettings(ig::FontType::SDF)))
		{
			ig::PopupMessage("Failed to load one or more fonts from: " + resourceFolder, "Error", &context);
			mainloop.Quit();
			return;
		}

		cmd.Begin();
		{
			r.Load(context, cmd, context.GetBackBufferRenderTargetDesc());
			defaultFont.LoadAsPrebaked(context, cmd, ig::GetDefaultFont());

			if (!ground.texture.LoadFromFile(context, cmd, resourceFolder + "grassy-cobblestone.jpg") ||
				!cube.texture.LoadFromFile(context, cmd, resourceFolder + "paving-stones.jpg") ||
				!sphere.texture.LoadFromFile(context, cmd, resourceFolder + "green-metal-rust.jpg"))
			{
				ig::PopupMessage("Failed to load one or more textures from: " + resourceFolder, "Error", &context);
				mainloop.Quit();
				return;
			}

			ground.mesh = GeneratePlaneMesh(context, cmd, 1.0f, 1.0f);
			cube.mesh = GenerateCubeMesh(context, cmd, 1.0f);
			sphere.mesh = GenerateSphereMesh(context, cmd, 0.5f, 40, 80);

		}
		cmd.End();
		context.WaitForCompletion(context.Submit(cmd));
	}

	// Generate a world matrix for each model
	ground.constantData.world = ig::Matrix4x4::WorldTRS(ig::Vector3(0, 0, 0), ig::Quaternion::Identity, ig::Vector3(10, 1, 10));
	cube.constantData.world = ig::Matrix4x4::WorldTRS(ig::Vector3(0, 0.5f, 1.0f), ig::Quaternion::Identity, ig::Vector3(1, 1, 1));
	sphere.constantData.world = ig::Matrix4x4::WorldTRS(ig::Vector3(0, 0.5f, -1.0f), ig::Quaternion::Identity, ig::Vector3(1, 1, 1));
}

void OnLoopExited()
{
	context.WaitForIdleDevice(); // Wait for GPU to finish all remaining work before resources get released
}

double tick = 0;
void Update(double elapsedSeconds)
{
	if (lightRotates) tick += elapsedSeconds;

	if (context.IsKeyDown(ig::Key::K)) lightElevation -= 0.5f * (float)elapsedSeconds;
	if (context.IsKeyDown(ig::Key::L)) lightElevation += 0.5f * (float)elapsedSeconds;
	if (lightElevation < 0) lightElevation = 0;
	if (lightElevation > 1) lightElevation = 1;

	if (context.IsKeyDown(ig::Key::I)) lightDistance -= 10.0f * (float)elapsedSeconds;
	if (context.IsKeyDown(ig::Key::O)) lightDistance += 10.0f * (float)elapsedSeconds;
	if (lightDistance < lightZNear) lightDistance = lightZNear;
	if (lightDistance > lightZFar) lightDistance = lightZFar;
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
		LoadRenderTargets(); // Update render target size
	}
	else if (e.type == ig::EventType::KeyDown)
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

		case ig::Key::M:
			context.WaitForIdleDevice();
			enableMSAA = !enableMSAA;
			LoadRenderTargets(); // Update render target MSAA
			break;

		case ig::Key::P:
			if (context.GetPresentMode() == ig::PresentMode::ImmediateWithTearing) context.SetPresentMode(ig::PresentMode::Immediate);
			else if (context.GetPresentMode() == ig::PresentMode::Immediate) context.SetPresentMode(ig::PresentMode::Vsync);
			else if (context.GetPresentMode() == ig::PresentMode::Vsync) context.SetPresentMode(ig::PresentMode::VsyncHalf);
			else context.SetPresentMode(ig::PresentMode::ImmediateWithTearing);
			break;

		case ig::Key::R:
			lightRotates = !lightRotates;
			break;

		}
	}
}

void RenderModel(const Model& model, bool bindTextures)
{
	if (bindTextures)
	{
		uint32_t textureIndex = model.texture.GetDescriptor()->heapIndex;
		cmd.SetPushConstants(&textureIndex, sizeof(textureIndex), 0); // First push constant
	}
	uint32_t modelConstantsIndex = model.constantBuffer.GetDescriptor()->heapIndex;
	cmd.SetPushConstants(&modelConstantsIndex, sizeof(modelConstantsIndex), sizeof(uint32_t) * 6); // Last push constant
	cmd.SetVertexBuffer(model.mesh.vertexBuffer);
	cmd.SetIndexBuffer(model.mesh.indexBuffer);
	cmd.DrawIndexed(model.mesh.indexBuffer.GetNumElements());
}

void RenderAllModels(bool bindTextures)
{
	RenderModel(cube, bindTextures);
	RenderModel(sphere, bindTextures);
	RenderModel(ground, bindTextures);
}

void RenderScene()
{
	// Update model constants (world matrices)
	{
		ModelConstants transposed;

		transposed.world = cube.constantData.world.GetTransposed();
		cube.constantBuffer.SetDynamicData(&transposed);

		transposed.world = ground.constantData.world.GetTransposed();
		ground.constantBuffer.SetDynamicData(&transposed);

		transposed.world = sphere.constantData.world.GetTransposed();
		sphere.constantBuffer.SetDynamicData(&transposed);
	}

	// Update camera
	{
		const float cameraAspectRatio = (float)context.GetWidth() / (float)context.GetHeight();
		ig::Vector3 cameraPos = ig::Vector3(-4.0f, 1.5f, 0);
		ig::Vector3 cameraLookAt = ig::Vector3(0, 0.5f, 0);
		ig::Vector3 cameraUp = ig::Vector3(0, 1, 0);
		ig::Matrix4x4 cameraView = ig::Matrix4x4::LookToLH(cameraPos, cameraLookAt - cameraPos, cameraUp);
		ig::Matrix4x4 cameraProjection = ig::Matrix4x4::PerspectiveFovLH(cameraAspectRatio, cameraFOV, cameraZNear, cameraZFar);
		camera.viewAndProj = cameraProjection * cameraView;

		CameraConstants transposed;
		transposed.viewAndProj = camera.viewAndProj.GetTransposed();
		cameraConstants.SetDynamicData(&transposed);
	}

	// Update light
	{
		const float lightAspectRatio = 1.0f;
		light.position = ig::Vector3(0, sinf(lightElevation * (float)IGLO_PI * 0.5f), -cosf(lightElevation * (float)IGLO_PI * 0.5f)) * lightDistance;
		if (light.position.y < 0) light.position.y = -light.position.y;
		light.position = ig::Vector3::TransformCoord(light.position, ig::Matrix4x4::RotationY((float)tick * 0.4f));
		light.ambientColor = lightAmbientColor;
		light.diffuseColor = lightDiffuseColor;
		ig::Vector3 lightLookAt = ig::Vector3(0, 0, 0);
		ig::Vector3 lightUp = ig::Vector3(0, 1, 0);
		ig::Matrix4x4 lightView = ig::Matrix4x4::LookToLH(light.position, lightLookAt - light.position, lightUp);
		ig::Matrix4x4 lightProjection = ig::Matrix4x4::PerspectiveFovLH(lightAspectRatio, lightFOV, lightZNear, lightZFar);
		light.viewAndProj = lightProjection * lightView;

		LightConstants transposed = light;
		transposed.viewAndProj = light.viewAndProj.GetTransposed();
		lightConstants.SetDynamicData(&transposed);
	}

	// Prepare the shadow map pipeline
	cmd.SetRenderTarget(nullptr, &shadowMap);
	cmd.SetViewport((float)shadowMapResolution, (float)shadowMapResolution);
	cmd.SetScissorRectangle(shadowMapResolution, shadowMapResolution);
	cmd.ClearDepth(shadowMap);
	cmd.SetPipeline(pipelineDepth);
	cmd.SetPrimitiveTopology(ig::Primitive::TriangleList);

	// Bind constants and samplers
	PushConstants pushConstants;
	pushConstants.colorTextureIndex = IGLO_UINT32_MAX; // Per model
	pushConstants.depthTextureIndex = shadowMap.GetDescriptor()->heapIndex;
	pushConstants.colorSamplerIndex = colorSampler.GetDescriptor()->heapIndex;
	pushConstants.depthSamplerIndex = depthSampler.GetDescriptor()->heapIndex;
	pushConstants.lightConstantsIndex = lightConstants.GetDescriptor()->heapIndex;
	pushConstants.cameraConstantsIndex = cameraConstants.GetDescriptor()->heapIndex;
	pushConstants.modelConstantsIndex = IGLO_UINT32_MAX; // Per model
	cmd.SetPushConstants(&pushConstants, sizeof(pushConstants));

	// Render the scene from the lights perspective
	RenderAllModels(false);

	// Prepare the lighting and shadows pipeline
	cmd.SetRenderTarget(&sceneRender, &sceneDepth);
	cmd.SetViewport((float)context.GetWidth(), (float)context.GetHeight());
	cmd.SetScissorRectangle(context.GetWidth(), context.GetHeight());
	cmd.SetPipeline(enableMSAA ? pipelineLightAndShadowMSAA : pipelineLightAndShadow);
	cmd.SetPushConstants(&pushConstants.depthTextureIndex, sizeof(uint32_t) * 3, sizeof(uint32_t) * 1);
	cmd.ClearColor(sceneRender, backgroundColor);
	cmd.ClearDepth(sceneDepth);

	cmd.AddTextureBarrier(shadowMap, ig::SimpleBarrier::DepthStencil, ig::SimpleBarrier::PixelShaderResource);
	cmd.FlushBarriers();

	// Render the scene from the cameras perspective
	RenderAllModels(true);
}

void RenderUI()
{
	r.Begin(cmd);
	{
		// Use point sampling for textures
		r.SetSamplerToPixelatedTextures();

		// Draw light source as a circle
		{
			ig::Vector4 lightPos = ig::Vector4(light.position.x, light.position.y, light.position.z, 1.0f);
			lightPos = ig::Vector4::Transform(lightPos, camera.viewAndProj);
			float scaleFactor = 1.0f / lightPos.w;
			lightPos /= lightPos.w;
			float screenX = (lightPos.x + 1.0f) * 0.5f * (float)context.GetWidth();
			float screenY = (1.0f - lightPos.y) * 0.5f * (float)context.GetHeight();
			ig::Color32 innerColor = light.ambientColor.ToColor32();
			r.DrawCircle(screenX, screenY, 70.0f * scaleFactor, 2, innerColor, innerColor, light.diffuseColor.ToColor32(), 1.0f);
		}

		// Draw depth buffers
		if (context.IsKeyDown(ig::Key::D))
		{
			r.SetDepthBufferDrawStyle(cameraZNear, cameraZFar, false);
			r.DrawTexture(sceneDepth, 0, 0, (float)context.GetWidth(), (float)context.GetHeight());
		}
		else if (context.IsKeyDown(ig::Key::E) && useStencil)
		{
			r.SetDepthBufferDrawStyle(0, 0, true); // Draw stencil component
			r.DrawTexture(sceneDepth, 0, 0, (float)context.GetWidth(), (float)context.GetHeight());
		}
		else if (context.IsKeyDown(ig::Key::S))
		{
			r.SetDepthBufferDrawStyle(lightZNear, lightZFar, false);
			r.DrawTexture(shadowMap, 0, 0, (float)context.GetWidth(), (float)context.GetHeight());
		}
		else
		{
			float textureSize = 128;
			float textureX = (float)context.GetWidth() - textureSize - 14;
			float textureY = 32;

			// Draw scene depth
			r.SetDepthBufferDrawStyle(cameraZNear, cameraZFar, false);
			r.DrawTexture(sceneDepth, textureX, textureY, textureSize, textureSize);
			r.DrawString(textureX, textureY - 20, "Depth [D]", defaultFont, ig::Colors::White);

			// Draw scene stencil (if used)
			if (useStencil)
			{
				textureY += textureSize + 42;
				r.SetDepthBufferDrawStyle(0, 0, true); // Draw stencil component
				r.DrawTexture(sceneDepth, textureX, textureY, textureSize, textureSize);
				r.DrawString(textureX, textureY - 20, "Stencil [E]", defaultFont, ig::Colors::White);
			}

			// Draw shadow map
			textureY += textureSize + 42;
			r.SetDepthBufferDrawStyle(lightZNear, lightZFar, false);
			r.DrawTexture(shadowMap, textureX, textureY, textureSize, textureSize);
			r.DrawString(textureX, textureY - 20, "Shadow map [S]", defaultFont, ig::Colors::White);
		}

		// Draw text
		{
			std::string strTop = ig::ToString(
				sampleName, "\n",
				ig::igloVersion, " ", context.GetGraphicsAPIShortName(), "\n",
				"FPS: ", mainloop.GetAvarageFPS());

			std::string presentModeStr = "";
			switch (context.GetPresentMode())
			{
			default: throw std::exception("Unexpected present mode");
			case ig::PresentMode::Immediate: presentModeStr = "Immediate"; break;
			case ig::PresentMode::ImmediateWithTearing: presentModeStr = "Immediate with tearing"; break;
			case ig::PresentMode::Vsync: presentModeStr = "Vsync"; break;
			case ig::PresentMode::VsyncHalf: presentModeStr = "Vsync Half"; break;
			}

			std::string strBottom = ig::ToString(
				"MSAA X", (int)sceneRender.GetMSAA(), " [M]\n",
				"Present mode [P]: ", presentModeStr, "\n",
				"Light distance [I, O]: ", lightDistance, "\n",
				"Light elevation [K, L]: ", lightElevation, "\n",
				"Light rotates [R]: ", lightRotates ? "True" : "False");

			ig::SDFEffect sdfEffect;
			sdfEffect.sdfEffectFlags = (uint32_t)ig::SDFEffectFlags::Outline | (uint32_t)ig::SDFEffectFlags::Glow;
			sdfEffect.outlineThickness = 0.21f;
			r.SetSDFEffect(sdfEffect);

			float bottomStrHeight = r.MeasureString(strBottom, trimSDF).y;

			r.DrawString(5, 0, strTop, trimSDF, ig::Colors::White);
			r.DrawString(5, (float)context.GetHeight() - bottomStrHeight - 5, strBottom, trimSDF, ig::Colors::White);
		}
	}
	r.End();
}

void Draw()
{
	cmd.Begin();
	{
		// Render all 3D things
		RenderScene();

		if (enableMSAA)
		{
			// Resolve scene if using MSAA
			cmd.AddTextureBarrier(sceneRender, ig::SimpleBarrier::RenderTarget, ig::SimpleBarrier::ResolveSource);
			cmd.AddTextureBarrier(sceneResolved, ig::SimpleBarrier::PixelShaderResource, ig::SimpleBarrier::ResolveDest);
			cmd.FlushBarriers();

			cmd.ResolveTexture(sceneRender, sceneResolved);

			cmd.AddTextureBarrier(sceneRender, ig::SimpleBarrier::ResolveSource, ig::SimpleBarrier::RenderTarget);
			cmd.AddTextureBarrier(sceneResolved, ig::SimpleBarrier::ResolveDest, ig::SimpleBarrier::PixelShaderResource);
			cmd.FlushBarriers();
		}
		else
		{
			// If not using MSAA, prepare to draw the render texture directly
			cmd.AddTextureBarrier(sceneRender, ig::SimpleBarrier::RenderTarget, ig::SimpleBarrier::PixelShaderResource);
			cmd.FlushBarriers();
		}

		// Prepare to render onto back buffer and to draw depth buffer
		cmd.AddTextureBarrier(context.GetBackBuffer(), ig::SimpleBarrier::Common, ig::SimpleBarrier::RenderTarget, false);
		cmd.AddTextureBarrier(sceneDepth, ig::SimpleBarrier::DepthStencil, ig::SimpleBarrier::PixelShaderResource);
		cmd.FlushBarriers();

		// Draw the resulting scene texture onto the back buffer
		cmd.SetRenderTarget(&context.GetBackBuffer());
		cmd.SetViewport((float)context.GetWidth(), (float)context.GetHeight());
		cmd.SetScissorRectangle(context.GetWidth(), context.GetHeight());
		r.Begin(cmd);
		{
			if (sceneRender.GetWidth() != context.GetWidth() ||
				sceneRender.GetHeight() != context.GetHeight())
			{
				// Use anisotropic filtering with clamping if source and dest texture are different sizes.
				// Anisotropic filtering makes the texture look smooth when stretched.
				// Clamping is used to prevent visual artifacts on the edges of the texture.
				//
				// In this example project, the render target texture will always have the same size as the back buffer.
				// But for more serious projects, render targets can sometimes be lower resolution than the back buffer.
				// An example of this is when borderless fullscreen mode is active, but user has chosen
				// an internal rendering resolution lower than the monitor resolution.
				// Remember, borderless fullscreen mode will force the back buffer to be the same size as the monitor resolution.
				r.SetSamplerToSmoothClampedTextures();
			}
			else
			{
				// If source and dest texture are the same size, use point sampling (fastest).
				// The performance difference between anisotropic filtering and point sampling isn't huge,
				// but there are no visual differences between the two in this case, so might as well get that extra +1% FPS.
				r.SetSamplerToPixelatedTextures();
			}

			// Either draw the resolved texture or the render texture, depending on whether MSAA is enabled or not
			r.DrawTexture(enableMSAA ? sceneResolved : sceneRender, 0, 0, (float)context.GetWidth(), (float)context.GetHeight(),
				ig::Colors::White, ig::TextureDrawStyle::BlendDisabled);
		}
		r.End();

		// Render all 2D things
		RenderUI();

		cmd.AddTextureBarrier(sceneDepth, ig::SimpleBarrier::PixelShaderResource, ig::SimpleBarrier::DepthStencil);
		cmd.AddTextureBarrier(shadowMap, ig::SimpleBarrier::PixelShaderResource, ig::SimpleBarrier::DepthStencil);
		cmd.AddTextureBarrier(context.GetBackBuffer(), ig::SimpleBarrier::RenderTarget, ig::SimpleBarrier::Common, false);
		if (!enableMSAA) cmd.AddTextureBarrier(sceneRender, ig::SimpleBarrier::PixelShaderResource, ig::SimpleBarrier::RenderTarget);
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
		ig::WindowSettings(sampleName, 1280, 720, true, true),
		ig::RenderSettings(ig::PresentMode::Vsync, ig::Format::BYTE_BYTE_BYTE_BYTE)))
	{
		mainloop.Run(context, Start, OnLoopExited, Draw, Update, FixedUpdate, OnEvent);
	}
	return 0;
}
