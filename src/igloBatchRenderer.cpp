#pragma once

#include "iglo.h"
#include "igloFont.h"
#include "igloBatchRenderer.h"

#include <cassert>

#ifdef __linux__
#include <cstring>
#endif

#include "shaders/PS_C.h"
#include "shaders/PS_CUV.h"
#include "shaders/PS_CUV_DepthBuffer.h"
#include "shaders/PS_CUV_MonoOpaque.h"
#include "shaders/PS_CUV_MonoTransparent.h"
#include "shaders/PS_CUV_SDF.h"
#include "shaders/PS_Circle.h"
#include "shaders/VS_InstancedCircle.h"
#include "shaders/VS_InstancedRect.h"
#include "shaders/VS_InstancedScaledSprite.h"
#include "shaders/VS_InstancedSprite.h"
#include "shaders/VS_InstancedTransformedSprite.h"
#include "shaders/VS_XYC.h"
#include "shaders/VS_XYCUV.h"

#define SHADER_VS(a) Shader(a, sizeof(a), "VSMain")
#define SHADER_PS(a) Shader(a, sizeof(a), "PSMain")

namespace ig
{
	void BatchRenderer::UsingBatch(BatchType batchType)
	{
		if (state.batchType == batchType) return;

		FlushPrimitives();

		if (batchType >= batchPipelines.size())
		{
			state.batchType = 0;
			state.batchDesc = batchPipelines.at(0).batchDesc;
			Log(LogType::Error, "Failed to set batch type. Reason: Invalid batch type.");
			return;
		}

		state.batchType = batchType;
		state.batchDesc = batchPipelines.at((uint32_t)batchType).batchDesc;
		state.bytesPerPrimitive = state.batchDesc.bytesPerVertex * state.batchDesc.inputVerticesPerPrimitive;
		if (state.bytesPerPrimitive == 0)
		{
			state.maxPrimitives = IGLO_UINT32_MAX;
		}
		else
		{
			state.maxPrimitives = MaxBatchSizeInBytes / state.bytesPerPrimitive;
		}
		if (state.batchDesc.vertGenMethod == BatchDesc::VertexGenerationMethod::Indexing)
		{
			uint32_t cappedMaxPrimitives = state.batchDesc.indexBuffer->GetNumElements() / state.batchDesc.outputVerticesPerPrimitive;
			if (state.maxPrimitives > cappedMaxPrimitives)
			{
				state.maxPrimitives = cappedMaxPrimitives;
			}
		}
	}

	void BatchRenderer::UsingTexture(const Texture& texture)
	{
		if (state.texture == &texture) return;

		FlushPrimitives();

		state.texture = &texture;

		// Use null texture if given texture doesn't have a descriptor
		const Texture* validTexture = &texture;
		if (!texture.IsLoaded() || !texture.GetDescriptor())
		{
			static bool hasWarned = false;
			if (!hasWarned)
			{
				hasWarned = true;
				Log(LogType::Warning, "You are attempting to draw an invalid texture with BatchRenderer."
					" A placeholder texture will be used instead.");
			}
			validTexture = &nullTexture;
		}
		if (validTexture->GetDescriptor()->IsNull()) throw std::runtime_error("This should be impossible.");

		TextureConstants textureConstants = {};
		textureConstants.textureSize = Vector2((float)validTexture->GetWidth(), (float)validTexture->GetHeight());
		textureConstants.inverseTextureSize = Vector2(1.0f / (float)validTexture->GetWidth(), 1.0f / (float)validTexture->GetHeight());
		textureConstants.msaa = (uint32_t)validTexture->GetMSAA();

		Descriptor tempConstant = context->CreateTempConstant(&textureConstants, sizeof(textureConstants));
		if (tempConstant.IsNull())
		{
			// Better to throw an exception than crash the GPU with an invalid resource heap index.
			throw std::runtime_error("This shouldn't happen unless you run out of VRAM.");
		}
		state.pushConstants.textureConstantsIndex = tempConstant.heapIndex;
		state.pushConstants.textureIndex = validTexture->GetDescriptor()->heapIndex;
		if (validTexture->GetStencilDescriptor())
		{
			state.pushConstants.stencilComponentTextureIndex = validTexture->GetStencilDescriptor()->heapIndex;
		}
		else
		{
			state.pushConstants.stencilComponentTextureIndex = validTexture->GetDescriptor()->heapIndex;
		}
	}

	void BatchRenderer::UsingRenderConstants(const Descriptor& descriptor)
	{
		if (descriptor.IsNull())
		{
			// Better to throw an exception than crash the GPU with an invalid resource heap index.
			throw std::invalid_argument("Descriptor is null.");
		}

		if (state.pushConstants.renderConstantsIndex == descriptor.heapIndex) return;

		FlushPrimitives();

		state.pushConstants.renderConstantsIndex = descriptor.heapIndex;
	}

	bool BatchRenderer::Load(IGLOContext& context, CommandList& cmd, RenderTargetDesc renderTargetDesc)
	{
		Unload();

		const char* errStr = "Failed to initialize batch renderer. Reason: ";

		// Create null texture
		{
			Image image;
			image.Load(8, 8, Format::BYTE_BYTE_BYTE_BYTE);
			for (uint32_t i = 0; i < image.GetWidth() * image.GetHeight(); i++)
			{
				uint32_t step = i;
				if (i >= (image.GetWidth() * image.GetHeight()) / 2) step += image.GetWidth() / 2;
				uint32_t filled = (step / (image.GetWidth() / 2)) % 2;
				((Color32*)image.GetPixels())[i] = filled ? Color32(Colors::Black) : Color32(Colors::Pink);
			}
			if (!this->nullTexture.LoadFromMemory(context, cmd, image))
			{
				Log(LogType::Error, ToString(errStr, "Failed to create null texture."));
				Unload();
				return false;
			}
		}

		// Create samplers
		if (!this->samplerSmoothTextures.Load(context, SamplerDesc::SmoothRepeatSampler) ||
			!this->samplerSmoothClampedTextures.Load(context, SamplerDesc::SmoothClampSampler) ||
			!this->samplerPixelatedTextures.Load(context, SamplerDesc::PixelatedRepeatSampler))
		{
			Log(LogType::Error, ToString(errStr, "Sampler state creation failed."));
			Unload();
			return false;
		}

		this->isLoaded = true;
		this->context = &context;
		this->renderTargetDesc = renderTargetDesc;

		this->vertices.clear();
		this->vertices.shrink_to_fit();
		this->vertices.resize(MaxBatchSizeInBytes);

		// Create standard batch types
		this->batchPipelines.clear();
		this->batchPipelines.shrink_to_fit();
		this->batchPipelines.push_back(BatchPipeline()); // Add the 'None' batch first, which is a batch type that draws nothing
		for (uint32_t i = 1; i < (uint32_t)StandardBatchType::NumStandardBatchTypes; i++)
		{
			CreateBatchType(GetStandardBatchParams((StandardBatchType)i, renderTargetDesc));
		}

		ResetRenderStates();
		return true;
	}

	void BatchRenderer::Unload()
	{
		isLoaded = false;
		context = nullptr;
		cmd = nullptr;
		renderTargetDesc = RenderTargetDesc();

		nullTexture.Unload();
		samplerSmoothTextures.Unload();
		samplerSmoothClampedTextures.Unload();
		samplerPixelatedTextures.Unload();

		tempConstantDepthBufferDrawStyle.SetToNull();
		tempConstantSDFEffect.SetToNull();

		batchPipelines.clear();
		batchPipelines.shrink_to_fit();

		vertices.clear();

		hasBegunDrawing = false;
		nextPrimitive = 0;
		drawCallCounter = 0;
		prevDrawCallCounter = 0;

		state = BatchState();
	}

	void BatchRenderer::Begin(CommandList& commandList)
	{
		if (!isLoaded)
		{
			Log(LogType::Error, "Failed to begin drawing with BatchRenderer. Reason: BatchRenderer isn't loaded!");
			return;
		}
		if (hasBegunDrawing)
		{
			Log(LogType::Error, "You must call BatchRenderer::End() at some point after BatchRenderer::Begin().");
			FlushPrimitives();
		}
		cmd = &commandList;
		drawCallCounter = 0;
		ResetRenderStates();
		hasBegunDrawing = true;
	}

	void BatchRenderer::End()
	{
		if (!isLoaded) return;
		FlushPrimitives();
		hasBegunDrawing = false;

		prevDrawCallCounter = drawCallCounter;
	}

	void BatchRenderer::ResetRenderStates()
	{
		FlushPrimitives();

		state = BatchState(); // Reset states

		// ----- These renderstates use redundancy checks ----- //
		state.batchType = IGLO_UINT32_MAX; // To bypass redundancy checker for UsingBatch()
		UsingBatch(0); // Use the 'None' batch type by default.
		UsingTexture(nullTexture);
		SetSampler(samplerSmoothTextures);

		// ----- These renderstates don't use redundancy checks ----- //
		SetDepthBufferDrawStyle(0.5f, 50.0f, false);
		SetSDFEffect(SDFEffect());
		SetViewAndProjection2D((float)context->GetWidth(), (float)context->GetHeight());
		RestoreWorldMatrix();
	}

	void BatchRenderer::SetSampler(const Sampler& samplerState)
	{
		if (!samplerState.IsLoaded())
		{
			Log(LogType::Error, "Failed to set BatchRenderer sampler. Reason: Sampler isn't loaded.");
			return;
		}
		if (state.sampler == &samplerState) return;

		FlushPrimitives();

		state.sampler = &samplerState;
		state.pushConstants.samplerIndex = samplerState.GetDescriptor()->heapIndex;

		if (samplerState.GetDescriptor()->IsNull()) throw std::runtime_error("This should be impossible.");
	}

	void BatchRenderer::SetSamplerToSmoothTextures()
	{
		SetSampler(samplerSmoothTextures);
	}

	void BatchRenderer::SetSamplerToSmoothClampedTextures()
	{
		SetSampler(samplerSmoothClampedTextures);
	}

	void BatchRenderer::SetSamplerToPixelatedTextures()
	{
		SetSampler(samplerPixelatedTextures);
	}

	void BatchRenderer::SetViewAndProjection3D(Vector3 position, Vector3 lookToDirection, Vector3 up, float aspectRatio,
		float fovInDegrees, float zNear, float zFar)
	{
		Matrix4x4 view = Matrix4x4::LookToLH(position, lookToDirection, up);
		Matrix4x4 proj = Matrix4x4::PerspectiveFovLH(aspectRatio, fovInDegrees, zNear, zFar);
		SetViewAndProjection(view, proj);
	}

	void BatchRenderer::SetViewAndProjection2D(float viewWidth, float viewHeight)
	{
		float originX = viewWidth / 2;
		float originY = viewHeight / 2;
		SetViewAndProjection2D(originX, originY, viewWidth, viewHeight);
	}

	void BatchRenderer::SetViewAndProjection2D(FloatRect viewArea)
	{
		float originX = viewArea.left + (viewArea.GetWidth() / 2);
		float originY = viewArea.top + (viewArea.GetHeight() / 2);
		SetViewAndProjection2D(originX, originY, viewArea.GetWidth(), viewArea.GetHeight());
	}

	void BatchRenderer::SetViewAndProjection2D(float viewX, float viewY, float viewWidth, float viewHeight, float zNear, float zFar)
	{
		Vector3 position = Vector3(viewX, viewY, 1);
		Vector3 direction = Vector3(0, 0, -1);
		Vector3 up = Vector3(0, -1, 0);

		Matrix4x4 view = Matrix4x4::LookToLH(position, direction, up);
		Matrix4x4 proj = Matrix4x4::OrthoLH(viewWidth, viewHeight, zNear, zFar);

		SetViewAndProjection(view, proj);
	}

	void BatchRenderer::SetViewAndProjection(Matrix4x4 view, Matrix4x4 projection)
	{
		FlushPrimitives();

		state.view = view;
		state.proj = projection;

		Matrix4x4 viewProjConstant = (projection * view).GetTransposed();
		Descriptor tempConstant = context->CreateTempConstant(&viewProjConstant, sizeof(viewProjConstant));

		// Better to throw an exception than crash the GPU with an invalid resource heap index.
		if (tempConstant.IsNull()) throw std::runtime_error("Failed to create temp constant.");

		state.pushConstants.viewProjMatrixIndex = tempConstant.heapIndex;
	}

	void BatchRenderer::SetWorldMatrix(Vector3 translation, Quaternion quaternionRotation, Vector3 scale)
	{
		Matrix4x4 world = Matrix4x4::WorldTRS(translation, quaternionRotation, scale);

		SetWorldMatrix(world);
	}

	void BatchRenderer::RestoreWorldMatrix()
	{
		SetWorldMatrix(Matrix4x4::Identity);
	}

	void BatchRenderer::SetWorldMatrix(Matrix4x4 world)
	{
		FlushPrimitives();

		state.world = world;

		Matrix4x4 worldConstant = world.GetTransposed();
		Descriptor tempConstant = context->CreateTempConstant(&worldConstant, sizeof(worldConstant));

		// Better to throw an exception than crash the GPU with an invalid resource heap index.
		if (tempConstant.IsNull()) throw std::runtime_error("Failed to create temp constant.");

		state.pushConstants.worldMatrixIndex = tempConstant.heapIndex;
	}

	BatchParams GetStandardBatchParams(StandardBatchType type, RenderTargetDesc renderTargetDesc)
	{
		static const std::vector<VertexElement> layout_XYC =
		{
			VertexElement(Format::FLOAT_FLOAT, "POSITION"),
			VertexElement(Format::BYTE_BYTE_BYTE_BYTE, "COLOR"),
		};
		static const std::vector<VertexElement> layout_XYCUV =
		{
			VertexElement(Format::FLOAT_FLOAT, "POSITION"),
			VertexElement(Format::BYTE_BYTE_BYTE_BYTE, "COLOR"),
			VertexElement(Format::FLOAT_FLOAT, "TEXCOORD"),
		};
		static const std::vector<VertexElement> layout_instanced_Rect =
		{
			VertexElement(Format::FLOAT_FLOAT, "POSITION", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::FLOAT, "WIDTH", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::FLOAT, "HEIGHT", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::BYTE_BYTE_BYTE_BYTE, "COLOR", 0, 0, InputClass::PerInstance, 1),
		};
		static const std::vector<VertexElement> layout_instanced_Sprite =
		{
			VertexElement(Format::FLOAT_FLOAT, "POSITION", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::UINT16_NotNormalized, "WIDTH", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::UINT16_NotNormalized, "HEIGHT", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::UINT16_UINT16_NotNormalized, "TEXCOORD", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::BYTE_BYTE_BYTE_BYTE, "COLOR", 0, 0, InputClass::PerInstance, 1),
		};
		static const std::vector<VertexElement> layout_instanced_ScaledSprite =
		{
			VertexElement(Format::FLOAT_FLOAT, "POSITION", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::FLOAT, "WIDTH", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::FLOAT, "HEIGHT", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::FLOAT_FLOAT_FLOAT_FLOAT, "TEXCOORD", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::BYTE_BYTE_BYTE_BYTE, "COLOR", 0, 0, InputClass::PerInstance, 1),
		};
		static const std::vector<VertexElement> layout_instanced_TransformedSprite =
		{
			VertexElement(Format::FLOAT_FLOAT, "POSITION", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::FLOAT, "WIDTH", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::FLOAT, "HEIGHT", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::FLOAT_FLOAT_FLOAT_FLOAT, "TEXCOORD", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::FLOAT_FLOAT, "ROTORIGIN", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::FLOAT, "ROTATION", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::BYTE_BYTE_BYTE_BYTE, "COLOR", 0, 0, InputClass::PerInstance, 1),
		};
		static const std::vector<VertexElement> layout_instanced_Circle =
		{
			VertexElement(Format::FLOAT_FLOAT, "POSITION", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::FLOAT, "RADIUS", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::FLOAT, "SMOOTHING", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::FLOAT, "BORDERTHICKNESS", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::BYTE_BYTE_BYTE_BYTE, "INNERCOLOR", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::BYTE_BYTE_BYTE_BYTE, "OUTERCOLOR", 0, 0, InputClass::PerInstance, 1),
			VertexElement(Format::BYTE_BYTE_BYTE_BYTE, "BORDERCOLOR", 0, 0, InputClass::PerInstance, 1),
		};

		static const std::vector<BlendDesc> blend_straightAlpha = { BlendDesc::StraightAlpha };
		static const std::vector<BlendDesc> blend_premultipliedAlpha = { BlendDesc::PremultipliedAlpha };
		static const std::vector<BlendDesc> blend_disabled = { BlendDesc::BlendDisabled };


		BatchParams out;
		out.pipelineDesc.renderTargetDesc = renderTargetDesc;
		out.pipelineDesc.blendStates = blend_straightAlpha;
		out.pipelineDesc.depthState = DepthDesc::DepthDisabled;

		// Use smooth lines by default
		out.pipelineDesc.rasterizerState = RasterizerDesc::NoCull;
		out.pipelineDesc.rasterizerState.lineRasterizationMode = LineRasterizationMode::Smooth;

		switch (type)
		{
		default:
			Log(LogType::Error, "Invalid standard batch type.");
			return BatchParams();

		case StandardBatchType::Points_XYC:
			out.batchDesc.primitive = Primitive::PointList;
			out.batchDesc.inputVerticesPerPrimitive = 1;
			out.batchDesc.bytesPerVertex = sizeof(Vertex_XYC);
			out.pipelineDesc.vertexLayout = layout_XYC;
			out.pipelineDesc.VS = SHADER_VS(g_VS_XYC);
			out.pipelineDesc.PS = SHADER_PS(g_PS_C);
			break;

		case StandardBatchType::Lines_XYC:
		case StandardBatchType::Lines_XYC_Pixelated:
			out.batchDesc.primitive = Primitive::LineList;
			out.batchDesc.inputVerticesPerPrimitive = 2;
			out.batchDesc.bytesPerVertex = sizeof(Vertex_XYC);
			out.pipelineDesc.vertexLayout = layout_XYC;
			out.pipelineDesc.VS = SHADER_VS(g_VS_XYC);
			out.pipelineDesc.PS = SHADER_PS(g_PS_C);
			if (type == StandardBatchType::Lines_XYC_Pixelated)
			{
				out.pipelineDesc.rasterizerState.lineRasterizationMode = LineRasterizationMode::Pixelated;
			}
			break;

		case StandardBatchType::Triangles_XYC:
			out.batchDesc.primitive = Primitive::TriangleList;
			out.batchDesc.inputVerticesPerPrimitive = 3;
			out.batchDesc.bytesPerVertex = sizeof(Vertex_XYC);
			out.pipelineDesc.vertexLayout = layout_XYC;
			out.pipelineDesc.VS = SHADER_VS(g_VS_XYC);
			out.pipelineDesc.PS = SHADER_PS(g_PS_C);
			break;

		case StandardBatchType::Triangles_XYCUV:
			out.batchDesc.primitive = Primitive::TriangleList;
			out.batchDesc.inputVerticesPerPrimitive = 3;
			out.batchDesc.bytesPerVertex = sizeof(Vertex_XYCUV);
			out.pipelineDesc.vertexLayout = layout_XYCUV;
			out.pipelineDesc.VS = SHADER_VS(g_VS_XYCUV);
			out.pipelineDesc.PS = SHADER_PS(g_PS_CUV);
			break;

		case StandardBatchType::Rects:
			out.batchDesc.vertGenMethod = BatchDesc::VertexGenerationMethod::Instancing;
			out.batchDesc.primitive = Primitive::TriangleStrip;
			out.batchDesc.inputVerticesPerPrimitive = 1;
			out.batchDesc.outputVerticesPerPrimitive = 4;
			out.batchDesc.bytesPerVertex = sizeof(Vertex_Rect);
			out.pipelineDesc.vertexLayout = layout_instanced_Rect;
			out.pipelineDesc.VS = SHADER_VS(g_VS_InstancedRect);
			out.pipelineDesc.PS = SHADER_PS(g_PS_C);
			break;

		case StandardBatchType::Sprites:
		case StandardBatchType::Sprites_MonoOpaque:
		case StandardBatchType::Sprites_MonoTransparent:
		case StandardBatchType::Sprites_SDF:
			out.batchDesc.vertGenMethod = BatchDesc::VertexGenerationMethod::Instancing;
			out.batchDesc.primitive = Primitive::TriangleStrip;
			out.batchDesc.inputVerticesPerPrimitive = 1;
			out.batchDesc.outputVerticesPerPrimitive = 4;
			out.batchDesc.bytesPerVertex = sizeof(Vertex_Sprite);
			out.pipelineDesc.vertexLayout = layout_instanced_Sprite;
			out.pipelineDesc.VS = SHADER_VS(g_VS_InstancedSprite);
			out.pipelineDesc.PS = SHADER_PS(g_PS_CUV);
			if (type == StandardBatchType::Sprites_MonoOpaque) out.pipelineDesc.PS = SHADER_PS(g_PS_CUV_MonoOpaque);
			if (type == StandardBatchType::Sprites_MonoTransparent) out.pipelineDesc.PS = SHADER_PS(g_PS_CUV_MonoTransparent);
			if (type == StandardBatchType::Sprites_SDF) out.pipelineDesc.PS = SHADER_PS(g_PS_CUV_SDF);
			break;

		case StandardBatchType::ScaledSprites:
		case StandardBatchType::ScaledSprites_MonoOpaque:
		case StandardBatchType::ScaledSprites_MonoTransparent:
		case StandardBatchType::ScaledSprites_SDF:
		case StandardBatchType::ScaledSprites_DepthBuffer:
		case StandardBatchType::ScaledSprites_PremultipliedAlpha:
		case StandardBatchType::ScaledSprites_BlendDisabled:
			out.batchDesc.vertGenMethod = BatchDesc::VertexGenerationMethod::Instancing;
			out.batchDesc.primitive = Primitive::TriangleStrip;
			out.batchDesc.inputVerticesPerPrimitive = 1;
			out.batchDesc.outputVerticesPerPrimitive = 4;
			out.batchDesc.bytesPerVertex = sizeof(Vertex_ScaledSprite);
			out.pipelineDesc.vertexLayout = layout_instanced_ScaledSprite;
			out.pipelineDesc.VS = SHADER_VS(g_VS_InstancedScaledSprite);
			out.pipelineDesc.PS = SHADER_PS(g_PS_CUV);
			if (type == StandardBatchType::ScaledSprites_MonoOpaque) out.pipelineDesc.PS = SHADER_PS(g_PS_CUV_MonoOpaque);
			if (type == StandardBatchType::ScaledSprites_MonoTransparent) out.pipelineDesc.PS = SHADER_PS(g_PS_CUV_MonoTransparent);
			if (type == StandardBatchType::ScaledSprites_SDF) out.pipelineDesc.PS = SHADER_PS(g_PS_CUV_SDF);
			if (type == StandardBatchType::ScaledSprites_DepthBuffer) out.pipelineDesc.PS = SHADER_PS(g_PS_CUV_DepthBuffer);
			if (type == StandardBatchType::ScaledSprites_PremultipliedAlpha) out.pipelineDesc.blendStates = blend_premultipliedAlpha;
			if (type == StandardBatchType::ScaledSprites_BlendDisabled) out.pipelineDesc.blendStates = blend_disabled;
			break;

		case StandardBatchType::TransformedSprites:
		case StandardBatchType::TransformedSprites_MonoOpaque:
		case StandardBatchType::TransformedSprites_MonoTransparent:
		case StandardBatchType::TransformedSprites_SDF:
			out.batchDesc.vertGenMethod = BatchDesc::VertexGenerationMethod::Instancing;
			out.batchDesc.primitive = Primitive::TriangleStrip;
			out.batchDesc.inputVerticesPerPrimitive = 1;
			out.batchDesc.outputVerticesPerPrimitive = 4;
			out.batchDesc.bytesPerVertex = sizeof(Vertex_TransformedSprite);
			out.pipelineDesc.vertexLayout = layout_instanced_TransformedSprite;
			out.pipelineDesc.VS = SHADER_VS(g_VS_InstancedTransformedSprite);
			out.pipelineDesc.PS = SHADER_PS(g_PS_CUV);
			if (type == StandardBatchType::TransformedSprites_MonoOpaque) out.pipelineDesc.PS = SHADER_PS(g_PS_CUV_MonoOpaque);
			if (type == StandardBatchType::TransformedSprites_MonoTransparent) out.pipelineDesc.PS = SHADER_PS(g_PS_CUV_MonoTransparent);
			if (type == StandardBatchType::TransformedSprites_SDF) out.pipelineDesc.PS = SHADER_PS(g_PS_CUV_SDF);
			break;

		case StandardBatchType::Circles:
			out.batchDesc.vertGenMethod = BatchDesc::VertexGenerationMethod::Instancing;
			out.batchDesc.primitive = Primitive::TriangleStrip;
			out.batchDesc.inputVerticesPerPrimitive = 1;
			out.batchDesc.outputVerticesPerPrimitive = 4;
			out.batchDesc.bytesPerVertex = sizeof(Vertex_Circle);
			out.pipelineDesc.vertexLayout = layout_instanced_Circle;
			out.pipelineDesc.VS = SHADER_VS(g_VS_InstancedCircle);
			out.pipelineDesc.PS = SHADER_PS(g_PS_Circle);
			break;
		}

		out.pipelineDesc.primitiveTopology = out.batchDesc.primitive;

		return out;
	}

	BatchType BatchRenderer::CreateBatchType(BatchParams batchParams)
	{
		const char* errStr = "Failed to create batch type. Reason: ";

		if (!isLoaded)
		{
			Log(LogType::Error, ToString(errStr, "BatchRenderer isn't loaded."));
			return 0;
		}

		if (batchParams.batchDesc.vertGenMethod == BatchDesc::VertexGenerationMethod::None)
		{
			if (batchParams.batchDesc.outputVerticesPerPrimitive > 0)
			{
				Log(LogType::Warning, "For batch types that don't use any vertex generation method, 'outputVerticesPerPrimitive' is expected to be zero.");
			}
		}
		else
		{
			if (batchParams.batchDesc.outputVerticesPerPrimitive == 0)
			{
				Log(LogType::Error, "For batch types that use a vertex generation method, 'outputVerticesPerPrimitive' must be non-zero.");
				return 0;
			}
		}

		if (batchParams.batchDesc.vertGenMethod == BatchDesc::VertexGenerationMethod::Instancing ||
			batchParams.batchDesc.vertGenMethod == BatchDesc::VertexGenerationMethod::VertexPullingStructured ||
			batchParams.batchDesc.vertGenMethod == BatchDesc::VertexGenerationMethod::VertexPullingRaw)
		{
			if (batchParams.batchDesc.inputVerticesPerPrimitive > 1)
			{
				Log(LogType::Error, "'inputVerticesPerPrimitive' can't be higher than 1 for batch types that use instancing or vertex pulling.");
				return 0;
			}
		}

		if (batchParams.batchDesc.vertGenMethod == BatchDesc::VertexGenerationMethod::Indexing)
		{
			bool indexBufferIsValid = false;
			if (batchParams.batchDesc.indexBuffer)
			{
				if (batchParams.batchDesc.indexBuffer->IsLoaded())
				{
					if (batchParams.batchDesc.indexBuffer->GetNumElements() % batchParams.batchDesc.outputVerticesPerPrimitive == 0)
					{
						indexBufferIsValid = true;
					}
				}
			}
			if (!indexBufferIsValid)
			{
				Log(LogType::Error, "For batch types that use indexing,"
					" you must provide an index buffer whose element count is a multiple of 'outputVerticesPerPrimitive'.");
				return 0;
			}
		}
		else
		{
			if (batchParams.batchDesc.indexBuffer)
			{
				Log(LogType::Warning, "For batch types that don't use indexing, the given index buffer is expected to be nullptr.");
			}
		}

		BatchPipeline batch;
		batch.batchDesc = batchParams.batchDesc;
		batch.pipeline = std::make_unique<Pipeline>();
		if (!batch.pipeline->Load(*context, batchParams.pipelineDesc))
		{
			Log(LogType::Error, ToString(errStr, "Failed to create pipeline state."));
			return 0;
		}

		BatchType out = (uint32_t)batchPipelines.size();
		batchPipelines.push_back(std::move(batch));
		return out;
	}

	void BatchRenderer::SetDepthBufferDrawStyle(float zNear, float zFar, bool drawStencilComponent)
	{
		FlushPrimitives();
		DepthBufferDrawStyle style = {};
		style.depthOrStencilComponent = drawStencilComponent ? 1 : 0;
		style.zNear = zNear;
		style.zFar = zFar;
		tempConstantDepthBufferDrawStyle = context->CreateTempConstant(&style, sizeof(style));
	}
	void BatchRenderer::SetSDFEffect(const SDFEffect& sdf)
	{
		FlushPrimitives();
		tempConstantSDFEffect = context->CreateTempConstant(&sdf, sizeof(sdf));
	}

	void BatchRenderer::AddPrimitive(const void* data)
	{
		if (nextPrimitive + 1 > state.maxPrimitives)
		{
			FlushPrimitives();
		}
		memcpy(vertices.data() + ((size_t)nextPrimitive * state.bytesPerPrimitive), data, (size_t)state.bytesPerPrimitive);
		nextPrimitive++;
	}

	void BatchRenderer::AddPrimitives(const void* data, uint32_t numPrimitives)
	{
		uint32_t primIndex = 0;
		while (primIndex < numPrimitives)
		{
			// If no more primitives can fit
			if (nextPrimitive + 1 > state.maxPrimitives)
			{
				FlushPrimitives();
			}

			// We can't copy all primitives at once if there is not enough space available
			uint32_t primsToCopy = numPrimitives - primIndex;
			if (primsToCopy > state.maxPrimitives - nextPrimitive)
			{
				primsToCopy = state.maxPrimitives - nextPrimitive;
			}

			memcpy(vertices.data() + ((size_t)nextPrimitive * state.bytesPerPrimitive),
				((byte*)data) + ((size_t)primIndex * state.bytesPerPrimitive),
				(size_t)primsToCopy * state.bytesPerPrimitive);

			primIndex += primsToCopy;
			nextPrimitive += primsToCopy;
		}
	}

	void BatchRenderer::FlushPrimitives()
	{
		if (nextPrimitive == 0) return;
		if (batchPipelines.at(state.batchType).pipeline == nullptr)
		{
			Log(LogType::Error, "BatchRenderer failed to draw pending primitives. Reason: Invalid batch type.");
			nextPrimitive = 0;
			return;
		}

		uint32_t numVertices = nextPrimitive * state.batchDesc.inputVerticesPerPrimitive;

		cmd->SetPipeline(*batchPipelines.at(state.batchType).pipeline.get());
		cmd->SetPrimitiveTopology(state.batchDesc.primitive);

		if (state.batchDesc.vertGenMethod == BatchDesc::VertexGenerationMethod::None ||
			state.batchDesc.vertGenMethod == BatchDesc::VertexGenerationMethod::Instancing ||
			state.batchDesc.vertGenMethod == BatchDesc::VertexGenerationMethod::Indexing)
		{
			cmd->SetTempVertexBuffer(vertices.data(), (uint64_t)nextPrimitive * state.bytesPerPrimitive, state.batchDesc.bytesPerVertex);
		}

		if (state.batchDesc.vertGenMethod == BatchDesc::VertexGenerationMethod::Indexing)
		{
			cmd->SetIndexBuffer(*state.batchDesc.indexBuffer);
		}

		if (state.batchDesc.vertGenMethod == BatchDesc::VertexGenerationMethod::VertexPullingStructured)
		{
			state.pushConstants.rawOrStructuredBufferIndex = context->CreateTempStructuredBuffer(vertices.data(),
				state.batchDesc.bytesPerVertex, numVertices).heapIndex;
		}
		else if (state.batchDesc.vertGenMethod == BatchDesc::VertexGenerationMethod::VertexPullingRaw)
		{
			state.pushConstants.rawOrStructuredBufferIndex = context->CreateTempRawBuffer(vertices.data(),
				(uint64_t)state.batchDesc.bytesPerVertex * numVertices).heapIndex;
		}

		cmd->SetPushConstants(&state.pushConstants, sizeof(state.pushConstants));

		switch (state.batchDesc.vertGenMethod)
		{
		case BatchDesc::VertexGenerationMethod::None:
			cmd->Draw(numVertices);
			break;

		case BatchDesc::VertexGenerationMethod::Instancing:
			cmd->DrawInstanced(state.batchDesc.outputVerticesPerPrimitive, numVertices);
			break;

		case BatchDesc::VertexGenerationMethod::Indexing:
			cmd->DrawIndexed(nextPrimitive * state.batchDesc.outputVerticesPerPrimitive);
			break;

		case BatchDesc::VertexGenerationMethod::VertexPullingStructured:
		case BatchDesc::VertexGenerationMethod::VertexPullingRaw:
			cmd->Draw(numVertices * state.batchDesc.outputVerticesPerPrimitive);
			break;

		default:
			throw std::runtime_error("impossible");
		}

		drawCallCounter++;
		nextPrimitive = 0;
	}

	void BatchRenderer::DrawString(float x, float y, const std::string& utf8string, Font& font, Color32 color)
	{
		DrawString(x, y, utf8string, font, color, 0, utf8string.size());
	}

	void BatchRenderer::DrawString(Vector2 position, const std::string& utf8string, Font& font, Color32 color)
	{
		DrawString(position.x, position.y, utf8string, font, color, 0, utf8string.size());
	}

	void BatchRenderer::DrawString(Vector2 position, const std::string& utf8string, Font& font, Color32 color, size_t startIndex, size_t endIndex)
	{
		DrawString(position.x, position.y, utf8string, font, color, 0, utf8string.size());
	}

	void BatchRenderer::DrawString(float x, float y, const std::string& utf8string, Font& font, Color32 color, size_t startIndex, size_t endIndex)
	{
		if (!font.IsLoaded()) return;
		if (utf8string.length() == 0) return;

		// Must select batch type atleast once
		bool hasSelectedBatch = false;

		float charX = x;
		float charY = y;
		uint32_t codepoint;
		uint32_t prevCodepoint = 0xffffffff;
		size_t i = startIndex;
		while (utf8_next_codepoint(utf8string, i, endIndex, &i, &codepoint)) // Iterate each codepoint in the text
		{
			switch (codepoint)
			{
				//TODO: How should tab characters be handled?
			case '\r':
				continue;

			case '\n': // Newline
				prevCodepoint = 0xffffffff;
				charY += float(font.GetFontDesc().lineHeight + font.GetFontDesc().lineGap);
				charX = x;
				continue;

			default:
				Glyph glyph = font.GetGlyph(codepoint);
				charX += font.GetKerning(prevCodepoint, codepoint); // Add kerning

				if (font.TextureIsDirty())
				{
					// Give all glyphs in the string a chance to be rasterized.
					font.PreloadGlyphs(utf8string);

					// Update the font texture.
					font.ApplyChangesToTexture(*cmd);

					// There is a possibility that the old font texture got replaced with a larger one to fit more glyphs.
					// This is why we must call UsingTexture() here to ensure the latest texture is still used.
					UsingTexture(*font.GetTexture());
				}

				// At this point, the font is guaranteed to have a texture.
				if (!hasSelectedBatch)
				{
					hasSelectedBatch = true;
					if (font.GetFontType() == FontType::SDF)
					{
						UsingBatch((BatchType)StandardBatchType::Sprites_SDF);
						UsingRenderConstants(GetSDFEffectRenderConstant());
					}
					else
					{
						if (font.GetTexture()->GetColorChannelCount() == 1)
						{
							UsingBatch((BatchType)StandardBatchType::Sprites_MonoTransparent);
						}
						else
						{
							UsingBatch((BatchType)StandardBatchType::Sprites);
						}
					}
					UsingTexture(*font.GetTexture());
				}

				float quadX = charX + float(glyph.glyphOffsetX);
				float quadY = charY + float(glyph.glyphOffsetY);
				Vertex_Sprite V = { Vector2(quadX, quadY), glyph.glyphWidth, glyph.glyphHeight, glyph.texturePosX, glyph.texturePosY, color };
				AddPrimitive(&V);

				charX += float(glyph.advanceX);
				prevCodepoint = codepoint;
				continue;
			}
		}
	}

	Vector2 BatchRenderer::MeasureString(const std::string& utf8string, Font& font)
	{
		return MeasureString(utf8string, font, 0, utf8string.size());
	}
	Vector2 BatchRenderer::MeasureString(const std::string& utf8string, Font& font, size_t startIndex, size_t endIndex)
	{
		if (!font.IsLoaded()) return Vector2(0, 0);

		float charX = 0;
		Vector2 boundingBox = Vector2(0, (float)font.GetFontDesc().lineHeight);
		uint32_t codepoint;
		uint32_t prevCodepoint = 0xffffffff;
		size_t i = startIndex;
		while (utf8_next_codepoint(utf8string, i, endIndex, &i, &codepoint)) // Iterate each codepoint in the text
		{
			switch (codepoint)
			{
			case '\r':
				continue;

			case '\n': // Newline
				prevCodepoint = 0xffffffff;
				boundingBox.y += float(font.GetFontDesc().lineHeight + font.GetFontDesc().lineGap);
				charX = 0;
				continue;

			default:
				Glyph glyph = font.GetGlyph(codepoint);
				charX += font.GetKerning(prevCodepoint, codepoint); // Add kerning
				charX += float(glyph.advanceX);
				prevCodepoint = codepoint;

				if (charX > boundingBox.x) boundingBox.x = charX;
				continue;
			}
		}
		return boundingBox;
	}

	void BatchRenderer::DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, Color32 color)
	{
		DrawTriangle(Vector2(x0, y0), Vector2(x1, y1), Vector2(x2, y2), color, color, color);
	}
	void BatchRenderer::DrawTriangle(Vector2 pos0, Vector2 pos1, Vector2 pos2, Color32 color)
	{
		DrawTriangle(pos0, pos1, pos2, color, color, color);
	}
	void BatchRenderer::DrawTriangle(Vector2 pos0, Vector2 pos1, Vector2 pos2, Color32 color0, Color32 color1, Color32 color2)
	{
		UsingBatch((BatchType)StandardBatchType::Triangles_XYC);
		Vertex_XYC V[] = { pos0, color0, pos1, color1, pos2, color2 };
		AddPrimitive(V);
	}

	void BatchRenderer::DrawTexturedTriangle(const Texture& texture, Vector2 pos0, Vector2 pos1, Vector2 pos2,
		Color32 color0, Color32 color1, Color32 color2, Vector2 texCoord0, Vector2 texCoord1, Vector2 texCoord2)
	{
		UsingBatch((BatchType)StandardBatchType::Triangles_XYCUV);
		UsingTexture(texture);
		Vertex_XYCUV V[] = { pos0, color0, texCoord0, pos1, color1, texCoord1, pos2, color2, texCoord2 };
		AddPrimitive(V);
	}

	void BatchRenderer::DrawTexture(const Texture& texture, float x, float y, Color32 color, TextureDrawStyle style)
	{
		DrawTexture(texture, x, y, (float)texture.GetWidth(), (float)texture.GetHeight(), color, style);
	}
	void BatchRenderer::DrawTexture(const Texture& texture, float x, float y, float width, float height, Color32 color, TextureDrawStyle style)
	{
		DrawTexture(texture, x, y, width, height, FloatRect(0, 0, (float)texture.GetWidth(), (float)texture.GetHeight()), color, style);
	}
	void BatchRenderer::DrawTexture(const Texture& texture, float x, float y, float width, float height, FloatRect UV, Color32 color,
		TextureDrawStyle style)
	{
		TextureDrawStyle chosenStyle = style;
		if (chosenStyle == TextureDrawStyle::Auto)
		{
			switch (texture.GetFormat())
			{
			default:
				chosenStyle = TextureDrawStyle::RGBA;
				break;

			case Format::DEPTHFORMAT_UINT16:
			case Format::DEPTHFORMAT_UINT24_BYTE:
			case Format::DEPTHFORMAT_FLOAT:
			case Format::DEPTHFORMAT_FLOAT_BYTE:
				chosenStyle = TextureDrawStyle::DepthBuffer;
				break;

			case Format::BYTE:
			case Format::FLOAT16:
			case Format::FLOAT:
			case Format::BC4_UNSIGNED:
			case Format::BC4_SIGNED:
				chosenStyle = TextureDrawStyle::MonochromeOpaque;
				break;
			}
		}

		switch (chosenStyle)
		{
		default:
			break;

		case TextureDrawStyle::DepthBuffer:
			UsingRenderConstants(GetDepthBufferDrawStyleRenderConstant());
			break;

		case TextureDrawStyle::SDF:
			UsingRenderConstants(GetSDFEffectRenderConstant());
			break;
		}

		UsingBatch((BatchType)chosenStyle);
		UsingTexture(texture);
		Vertex_ScaledSprite V[] = { Vector2(x, y), width, height, UV, color };
		AddPrimitive(V);
	}

	void BatchRenderer::DrawRectangle(float x, float y, float width, float height, Color32 color)
	{
		UsingBatch((BatchType)StandardBatchType::Rects);
		Vertex_Rect V[] = { Vector2(x, y), width, height, color };
		AddPrimitive(V);
	}
	void BatchRenderer::DrawRectangle(FloatRect rect, Color32 color)
	{
		DrawRectangle(rect.left, rect.top, rect.GetWidth(), rect.GetHeight(), color);
	}

	void BatchRenderer::DrawRectangleBorder(FloatRect rect, Color32 color, float borderThickness)
	{
		UsingBatch((BatchType)StandardBatchType::Rects);
		FloatRect norm = rect.GetNormalized();
		float x = norm.left;
		float y = norm.top;
		float width = norm.GetWidth();
		float height = norm.GetHeight();
		Vertex_Rect V = {};
		if (height <= borderThickness * 2)
		{
			// Cover entire rectangle
			V = { Vector2(x, y), width, height, color };
			AddPrimitive(&V);
			return;
		}

		// Top
		V = { Vector2(x, y), width, borderThickness, color };
		AddPrimitive(&V);

		// Bottom
		V = { Vector2(x, y + height - borderThickness), width, borderThickness, color };
		AddPrimitive(&V);

		if (width > borderThickness * 2)
		{
			// Left
			V = { Vector2(x, y + borderThickness), borderThickness, height - (borderThickness * 2), color };
			AddPrimitive(&V);

			// Right
			V = { Vector2(x + width - borderThickness, y + borderThickness), borderThickness, height - (borderThickness * 2), color };
			AddPrimitive(&V);
		}
		else
		{
			// From left to right
			V = { Vector2(x, y + borderThickness), width, height - (borderThickness * 2), color };
			AddPrimitive(&V);
		}
	}
	void BatchRenderer::DrawRectangleBorder(float x, float y, float width, float height, Color32 color, float borderThickness)
	{
		DrawRectangleBorder(FloatRect(x, y, x + width, y + height), color, borderThickness);
	}

	void BatchRenderer::DrawLine(float x0, float y0, float x1, float y1, Color32 color, LineRasterizationMode rasterization)
	{
		DrawLine(Vector2(x0, y0), Vector2(x1, y1), color, color, rasterization);
	}
	void BatchRenderer::DrawLine(float x0, float y0, float x1, float y1, Color32 color0, Color32 color1, LineRasterizationMode rasterization)
	{
		DrawLine(Vector2(x0, y0), Vector2(x1, y1), color0, color1, rasterization);
	}
	void BatchRenderer::DrawLine(Vector2 pos0, Vector2 pos1, Color32 color0, Color32 color1, LineRasterizationMode rasterization)
	{
		switch (rasterization)
		{
		default:
			UsingBatch((BatchType)StandardBatchType::Lines_XYC);
			break;
		case LineRasterizationMode::Pixelated:
			UsingBatch((BatchType)StandardBatchType::Lines_XYC_Pixelated);
			break;
		}

		Vertex_XYC V[] = { pos0, color0, pos1, color1 };
		AddPrimitive(V);
	}

	void BatchRenderer::DrawRectangularLine(float x0, float y0, float x1, float y1, float thickness, Color32 color)
	{
		float r = -atan2(x1 - x0, y1 - y0);
		Vector2 p, p0, p1, p2, p3;
		p = Vector2(thickness / 2, 0).GetRotated(r);
		p0 = Vector2(x0, y0) - p;
		p1 = Vector2(x0, y0) + p;
		p2 = Vector2(x1, y1) - p;
		p3 = Vector2(x1, y1) + p;
		UsingBatch((BatchType)StandardBatchType::Triangles_XYC);
		Vertex_XYC V0[] = { Vector2(p0.x, p0.y), color, Vector2(p1.x, p1.y), color, Vector2(p2.x, p2.y), color };
		Vertex_XYC V1[] = { Vector2(p2.x, p2.y), color, Vector2(p1.x, p1.y), color, Vector2(p3.x, p3.y), color };
		AddPrimitive(V0);
		AddPrimitive(V1);
	}

	void BatchRenderer::DrawTexturedRectangularLine(const Texture& texture, float x0, float y0, float x1, float y1, float thickness, Color32 color)
	{
		float r = -atan2(x1 - x0, y1 - y0);
		Vector2 p, p0, p1, p2, p3;
		p = Vector2(thickness / 2, 0).GetRotated(r);
		p0 = Vector2(x0, y0) - p;
		p1 = Vector2(x0, y0) + p;
		p2 = Vector2(x1, y1) - p;
		p3 = Vector2(x1, y1) + p;
		UsingBatch((BatchType)StandardBatchType::Triangles_XYCUV);
		UsingTexture(texture);
		Vertex_XYCUV V0[] = { Vector2(p0.x, p0.y), color, Vector2(0, 0), Vector2(p1.x, p1.y), color, Vector2(1, 0), Vector2(p2.x, p2.y), color, Vector2(0, 1) };
		Vertex_XYCUV V1[] = { Vector2(p2.x, p2.y), color, Vector2(0, 1), Vector2(p1.x, p1.y), color, Vector2(1, 0), Vector2(p3.x, p3.y), color, Vector2(1, 1) };
		AddPrimitive(V0);
		AddPrimitive(V1);
	}

	void BatchRenderer::DrawCircle(float x, float y, float radius, float borderThickness,
		Color32 innerColor, Color32 outerColor, Color32 borderColor, float smoothing)
	{
		UsingBatch((BatchType)StandardBatchType::Circles);
		Vertex_Circle V[] = { Vector2(x, y), radius, smoothing, borderThickness, innerColor, outerColor, borderColor };
		AddPrimitive(V);
	}
	void BatchRenderer::DrawCircle(float x, float y, float radius, float borderThickness, Color32 fillColor, Color32 borderColor)
	{
		DrawCircle(x, y, radius, borderThickness, fillColor, fillColor, borderColor);
	}
	void BatchRenderer::DrawCircle(float x, float y, float radius, Color32 innerColor, Color32 outerColor)
	{
		DrawCircle(x, y, radius, 0, innerColor, outerColor, Colors::Transparent);
	}
	void BatchRenderer::DrawCircle(float x, float y, float radius, Color32 color)
	{
		DrawCircle(x, y, radius, 0, color, color, Colors::Transparent);
	}

	void BatchRenderer::DrawCake(float x, float y, float radius, uint32_t sides, float degreesStart, float degreesLength,
		Color32 colorCenter, Color32 colorBorder)
	{
		const uint32_t maxCircleSides = 360;
		if (sides > maxCircleSides)sides = maxCircleSides;
		if (sides < 3)sides = 3;

		UsingBatch((BatchType)StandardBatchType::Triangles_XYC);
		for (uint32_t i = 0; i < sides; i++)
		{
			float rotA = degreesLength * (float(i) / (float)sides);
			float rotB = degreesLength * (float(i + 1) / (float)sides);
			Vector2 edgeA;
			edgeA.x = cosf((float)IGLO_ToRadian((rotA - 90) + degreesStart)) * radius;
			edgeA.y = sinf((float)IGLO_ToRadian((rotA - 90) + degreesStart)) * radius;
			Vector2 edgeB;
			edgeB.x = cosf((float)IGLO_ToRadian((rotB - 90) + degreesStart)) * radius;
			edgeB.y = sinf((float)IGLO_ToRadian((rotB - 90) + degreesStart)) * radius;
			Vertex_XYC V[] = {
				Vector2(x, y), colorCenter,
				Vector2(x + edgeA.x, y + edgeA.y), colorBorder,
				Vector2(x + edgeB.x, y + edgeB.y), colorBorder };
			AddPrimitive(V);
		}
	}

	void BatchRenderer::DrawCakeBorder(float x, float y, float radius, uint32_t sides, float degreesStart, float degreesLength,
		Color32 colorCenter, Color32 colorBorder)
	{
		const uint32_t maxCircleSides = 360;
		if (sides > maxCircleSides)sides = maxCircleSides;
		if (sides < 3)sides = 3;

		UsingBatch((BatchType)StandardBatchType::Lines_XYC);
		for (uint32_t i = 0; i < sides; i++)
		{
			float rotA = degreesLength * (float(i) / (float)sides);
			float rotB = degreesLength * (float(i + 1) / (float)sides);
			Vector2 edgeA;
			edgeA.x = cosf((float)IGLO_ToRadian((rotA - 90) + degreesStart)) * radius;
			edgeA.y = sinf((float)IGLO_ToRadian((rotA - 90) + degreesStart)) * radius;
			Vector2 edgeB;
			edgeB.x = cosf((float)IGLO_ToRadian((rotB - 90) + degreesStart)) * radius;
			edgeB.y = sinf((float)IGLO_ToRadian((rotB - 90) + degreesStart)) * radius;
			if (i == 0)
			{
				Vertex_XYC V[] = { Vector2(x, y), colorCenter, Vector2(x + edgeA.x, y + edgeA.y), colorBorder };
				AddPrimitive(V);
			}
			if (i == sides - 1)
			{
				Vertex_XYC V[] = { Vector2(x, y), colorCenter, Vector2(x + edgeB.x, y + edgeB.y), colorBorder };
				AddPrimitive(V);
			}
			Vertex_XYC V[] = { Vector2(x + edgeA.x, y + edgeA.y), colorBorder, Vector2(x + edgeB.x, y + edgeB.y), colorBorder };
			AddPrimitive(V);
		}
	}

	void BatchRenderer::DrawPoint(float x, float y, Color32 color)
	{
		UsingBatch((BatchType)StandardBatchType::Points_XYC);
		Vertex_XYC V[] = { Vector2(x, y), color };
		AddPrimitive(V);
	}

	void BatchRenderer::DrawRoundedRectangle(float x, float y, float width, float height, Color32 color, float circleRadius, uint32_t circleSides)
	{
		const uint32_t maxCircleSides = 16; // Amount of triangles in each quarter circle
		const uint32_t minCircleSides = 1;
		if (circleSides > maxCircleSides) circleSides = maxCircleSides;
		if (circleSides < minCircleSides) circleSides = minCircleSides;
		if (circleRadius > width / 2)circleRadius = width / 2;
		if (circleRadius > height / 2)circleRadius = height / 2;
		FloatRect centerBox = FloatRect(x + circleRadius, y + circleRadius, x + width - circleRadius, y + height - circleRadius);
		FloatRect topBox = FloatRect(x + circleRadius, y, x + width - circleRadius, y + circleRadius);
		FloatRect bottomBox = topBox;
		bottomBox.top = centerBox.bottom;
		bottomBox.bottom = y + height;
		FloatRect leftBox = FloatRect(x, y + circleRadius, x + circleRadius, y + height - circleRadius);
		FloatRect rightBox = leftBox;
		rightBox.left = centerBox.right;
		rightBox.right = x + width;

		// Circle center locations
		Vector2 topLeft = Vector2(centerBox.left, centerBox.top);
		Vector2 topRight = Vector2(centerBox.right, centerBox.top);
		Vector2 bottomLeft = Vector2(centerBox.left, centerBox.bottom);
		Vector2 bottomRight = Vector2(centerBox.right, centerBox.bottom);

		UsingBatch((BatchType)StandardBatchType::Triangles_XYC);

		Vertex_XYC V[] =
		{
			// The center rectangle
			Vector2(centerBox.left, centerBox.top), color,
			Vector2(centerBox.right, centerBox.top), color,
			Vector2(centerBox.left, centerBox.bottom), color,

			Vector2(centerBox.left, centerBox.bottom), color,
			Vector2(centerBox.right, centerBox.top), color,
			Vector2(centerBox.right, centerBox.bottom), color ,

			// The top rectangle
			Vector2(topBox.left, topBox.top), color,
			Vector2(topBox.right, topBox.top), color,
			Vector2(topBox.left, topBox.bottom), color,

			Vector2(topBox.left, topBox.bottom), color,
			Vector2(topBox.right, topBox.top), color,
			Vector2(topBox.right, topBox.bottom), color,

			// The bottom rectangle
			Vector2(bottomBox.left, bottomBox.top), color,
			Vector2(bottomBox.right, bottomBox.top), color,
			Vector2(bottomBox.left, bottomBox.bottom), color,

			Vector2(bottomBox.left, bottomBox.bottom), color,
			Vector2(bottomBox.right, bottomBox.top), color,
			Vector2(bottomBox.right, bottomBox.bottom), color,

			// The left rectangle
			Vector2(leftBox.left, leftBox.top), color,
			Vector2(leftBox.right, leftBox.top), color,
			Vector2(leftBox.left, leftBox.bottom), color,

			Vector2(leftBox.left, leftBox.bottom), color,
			Vector2(leftBox.right, leftBox.top), color,
			Vector2(leftBox.right, leftBox.bottom), color,

			// The right rectangle
			Vector2(rightBox.left, rightBox.top), color,
			Vector2(rightBox.right, rightBox.top), color,
			Vector2(rightBox.left, rightBox.bottom), color,

			Vector2(rightBox.left, rightBox.bottom), color,
			Vector2(rightBox.right, rightBox.top), color,
			Vector2(rightBox.right, rightBox.bottom), color
		};
		AddPrimitives(V, 10);

		float degreePerTriangle = (float)IGLO_ToRadian(90); // each circle is a quarter size
		degreePerTriangle /= circleSides;
		for (unsigned int i = 0; i < circleSides; i++)
		{
			// Top left circle triangle
			Vector2 edgeA = Vector2(-circleRadius, 0).GetRotated(degreePerTriangle * i);
			Vector2 edgeB = edgeA.GetRotated(degreePerTriangle);
			Vertex_XYC V0[] = {
				Vector2(topLeft.x, topLeft.y), color,
				Vector2(topLeft.x + edgeA.x, topLeft.y + edgeA.y), color,
				Vector2(topLeft.x + edgeB.x, topLeft.y + edgeB.y), color };
			AddPrimitive(V0);

			// Top right circle triangle
			edgeA = Vector2(0, -circleRadius).GetRotated(degreePerTriangle * i);
			edgeB = edgeA.GetRotated(degreePerTriangle);
			Vertex_XYC V1[] = {
				Vector2(topRight.x, topRight.y), color,
				Vector2(topRight.x + edgeA.x, topRight.y + edgeA.y), color,
				Vector2(topRight.x + edgeB.x, topRight.y + edgeB.y), color };
			AddPrimitive(V1);

			// Bottom left circle triangle
			edgeA = Vector2(0, circleRadius).GetRotated(degreePerTriangle * i);
			edgeB = edgeA.GetRotated(degreePerTriangle);
			Vertex_XYC V2[] = {
				Vector2(bottomLeft.x, bottomLeft.y), color,
				Vector2(bottomLeft.x + edgeA.x, bottomLeft.y + edgeA.y), color,
				Vector2(bottomLeft.x + edgeB.x, bottomLeft.y + edgeB.y), color };
			AddPrimitive(V2);

			// Bottom right circle triangle
			edgeA = Vector2(circleRadius, 0).GetRotated(degreePerTriangle * i);
			edgeB = edgeA.GetRotated(degreePerTriangle);
			Vertex_XYC V3[] = {
				Vector2(bottomRight.x, bottomRight.y), color,
				Vector2(bottomRight.x + edgeA.x, bottomRight.y + edgeA.y), color,
				Vector2(bottomRight.x + edgeB.x, bottomRight.y + edgeB.y), color };
			AddPrimitive(V3);
		}
	}

	void BatchRenderer::DrawRoundedRectangleBorder(float x, float y, float width, float height, Color32 color, float circleRadius, uint32_t circleSides)
	{
		const uint32_t maxCircleSides = 16; // Amount of triangles in each quarter circle
		const uint32_t minCircleSides = 1;
		if (circleSides > maxCircleSides) circleSides = maxCircleSides;
		if (circleSides < minCircleSides) circleSides = minCircleSides;
		if (circleRadius > width / 2)circleRadius = width / 2;
		if (circleRadius > height / 2)circleRadius = height / 2;
		FloatRect centerBox = FloatRect(x + circleRadius, y + circleRadius, x + width - circleRadius, y + height - circleRadius);
		FloatRect topBox = FloatRect(x + circleRadius, y, x + width - circleRadius, y + circleRadius);
		FloatRect bottomBox = topBox;
		bottomBox.top = centerBox.bottom;
		bottomBox.bottom = y + height;
		FloatRect leftBox;
		leftBox.left = x;
		leftBox.top = y + circleRadius;
		leftBox.bottom = y + height - circleRadius;
		leftBox.right = x + circleRadius;
		FloatRect rightBox = leftBox;
		rightBox.left = centerBox.right;
		rightBox.right = x + width;

		// Circle center locations
		Vector2 topLeft = Vector2(centerBox.left, centerBox.top);
		Vector2 topRight = Vector2(centerBox.right, centerBox.top);
		Vector2 bottomLeft = Vector2(centerBox.left, centerBox.bottom);
		Vector2 bottomRight = Vector2(centerBox.right, centerBox.bottom);

		UsingBatch((BatchType)StandardBatchType::Lines_XYC);

		Vertex_XYC V[] =
		{
			Vector2(topBox.left, topBox.top), color,
			Vector2(topBox.right, topBox.top), color,

			Vector2(bottomBox.left, bottomBox.bottom), color,
			Vector2(bottomBox.right, bottomBox.bottom), color,

			Vector2(leftBox.left, leftBox.top), color,
			Vector2(leftBox.left, leftBox.bottom), color,

			Vector2(rightBox.right, rightBox.top), color,
			Vector2(rightBox.right, rightBox.bottom), color
		};
		AddPrimitives(V, 4);

		float degreePerTriangle = (float)IGLO_ToRadian(90); // each circle is a quarter size
		degreePerTriangle /= circleSides;
		for (unsigned int i = 0; i < circleSides; i++)
		{
			// Top left
			Vector2 edgeA = Vector2(-circleRadius, 0).GetRotated(degreePerTriangle * i);
			Vector2 edgeB = edgeA.GetRotated(degreePerTriangle);
			Vertex_XYC V0[] = {
				Vector2(topLeft.x + edgeA.x, topLeft.y + edgeA.y), color,
				Vector2(topLeft.x + edgeB.x, topLeft.y + edgeB.y), color };
			AddPrimitive(V0);

			// Top right
			edgeA = Vector2(0, -circleRadius).GetRotated(degreePerTriangle * i);
			edgeB = edgeA.GetRotated(degreePerTriangle);
			Vertex_XYC V1[] = {
				Vector2(topRight.x + edgeA.x, topRight.y + edgeA.y), color,
				Vector2(topRight.x + edgeB.x, topRight.y + edgeB.y), color };
			AddPrimitive(V1);

			// Bottom left
			edgeA = Vector2(0, circleRadius).GetRotated(degreePerTriangle * i);
			edgeB = edgeA.GetRotated(degreePerTriangle);
			Vertex_XYC V2[] = {
				Vector2(bottomLeft.x + edgeA.x, bottomLeft.y + edgeA.y), color,
				Vector2(bottomLeft.x + edgeB.x, bottomLeft.y + edgeB.y), color };
			AddPrimitive(V2);

			// Bottom right
			edgeA = Vector2(circleRadius, 0).GetRotated(degreePerTriangle * i);
			edgeB = edgeA.GetRotated(degreePerTriangle);
			Vertex_XYC V3[] = {
				Vector2(bottomRight.x + edgeA.x, bottomRight.y + edgeA.y), color,
				Vector2(bottomRight.x + edgeB.x, bottomRight.y + edgeB.y), color };
			AddPrimitive(V3);
		}
	}

	void BatchRenderer::DrawCenterStretchedTexture(const Texture& texture, float x, float y, float width, float height, Color32 color, TextureDrawStyle style)
	{
		/*
			center stretched texture		not stretched (normal)
			 ___________________			 _________
			|A	|H1			|  B|			|A	 |   B|
			|___|___________|___|			|____|____|
			|V1	|Center		|V2	|			| 	 |	  |
			|___|___________|___|			|C___|___D|
			| 	|H2			|   |
			|C__|___________|__D|

		*/
		bool splitHorizontal = (width > texture.GetWidth());
		bool splitVertical = (height > texture.GetHeight());

		FloatRect A = FloatRect(x, y, x + (width / 2), y + (height / 2));
		if (splitHorizontal) A.right = x + (texture.GetWidth() * 0.5f);
		if (splitVertical) A.bottom = y + (texture.GetHeight() * 0.5f);

		FloatRect B = FloatRect(x + (width / 2), y, x + width, y + (height / 2));
		if (splitHorizontal) B.left = x + width - (texture.GetWidth() * 0.5f);
		if (splitVertical) B.bottom = y + (texture.GetHeight() * 0.5f);

		FloatRect C = FloatRect(x, y + (height / 2), x + (width / 2), y + height);
		if (splitHorizontal) C.right = x + (texture.GetWidth() * 0.5f);
		if (splitVertical) C.top = y + height - (texture.GetHeight() * 0.5f);

		FloatRect D = FloatRect(x + (width / 2), y + (height / 2), x + width, y + height);
		if (splitHorizontal) D.left = x + width - (texture.GetWidth() * 0.5f);
		if (splitVertical) D.top = y + height - (texture.GetHeight() * 0.5f);

		FloatRect	  V1 = FloatRect(A.left, A.bottom, A.right, C.top);
		FloatRect	  V2 = FloatRect(B.left, B.bottom, B.right, D.top);
		FloatRect	  H1 = FloatRect(A.right, A.top, B.left, A.bottom);
		FloatRect	  H2 = FloatRect(C.right, C.top, D.left, C.bottom);
		FloatRect Center = FloatRect(V1.right, H1.bottom, V2.left, H2.top);

		// Texture UV coordinates of all quads
		float W = (float)texture.GetWidth();
		float H = (float)texture.GetHeight();
		FloatRect	   A_UV = FloatRect(0.0f * W, 0.0f * H, 0.5f * W, 0.5f * H);
		FloatRect	   B_UV = FloatRect(0.5f * W, 0.0f * H, 1.0f * W, 0.5f * H);
		FloatRect	   C_UV = FloatRect(0.0f * W, 0.5f * H, 0.5f * W, 1.0f * H);
		FloatRect	   D_UV = FloatRect(0.5f * W, 0.5f * H, 1.0f * W, 1.0f * H);
		FloatRect	  V1_UV = FloatRect(0.0f * W, 0.5f * H, 0.5f * W, 0.5f * H);
		FloatRect	  V2_UV = FloatRect(0.5f * W, 0.5f * H, 1.0f * W, 0.5f * H);
		FloatRect	  H1_UV = FloatRect(0.5f * W, 0.0f * H, 0.5f * W, 0.5f * H);
		FloatRect	  H2_UV = FloatRect(0.5f * W, 0.5f * H, 0.5f * W, 1.0f * H);
		FloatRect Center_UV = FloatRect(0.5f * W, 0.5f * H, 0.5f * W, 0.5f * H);

		DrawTexture(texture, A.left, A.top, A.GetWidth(), A.GetHeight(), A_UV, color, style);
		DrawTexture(texture, B.left, B.top, B.GetWidth(), B.GetHeight(), B_UV, color, style);
		DrawTexture(texture, C.left, C.top, C.GetWidth(), C.GetHeight(), C_UV, color, style);
		DrawTexture(texture, D.left, D.top, D.GetWidth(), D.GetHeight(), D_UV, color, style);

		if (splitVertical)
		{
			DrawTexture(texture, V1.left, V1.top, V1.GetWidth(), V1.GetHeight(), V1_UV, color, style);
			DrawTexture(texture, V2.left, V2.top, V2.GetWidth(), V2.GetHeight(), V2_UV, color, style);
		}
		if (splitHorizontal)
		{
			DrawTexture(texture, H1.left, H1.top, H1.GetWidth(), H1.GetHeight(), H1_UV, color, style);
			DrawTexture(texture, H2.left, H2.top, H2.GetWidth(), H2.GetHeight(), H2_UV, color, style);
		}
		if (splitVertical && splitHorizontal)
		{
			DrawTexture(texture, Center.left, Center.top, Center.GetWidth(), Center.GetHeight(), Center_UV, color, style);
		}
	}

	void BatchRenderer::DrawSprite(const Texture& texture, float x, float y, IntRect uv, Color32 color)
	{
		DrawSprite(texture, x, y, (uint16_t)uv.GetWidth(), (uint16_t)uv.GetHeight(), (uint16_t)uv.left, (uint16_t)uv.top, color);
	}

	void BatchRenderer::DrawSprite(const Texture& texture, float x, float y, uint16_t width, uint16_t height, uint16_t u, uint16_t v, Color32 color)
	{
		UsingBatch((BatchType)StandardBatchType::Sprites);
		UsingTexture(texture);
		Vertex_Sprite V[] = { Vector2(x, y), width, height, u, v, color };
		AddPrimitive(V);
	}

	void BatchRenderer::DrawScaledSprite(const Texture& texture, float x, float y, float width, float height, FloatRect uv, Color32 color)
	{
		UsingBatch((BatchType)StandardBatchType::ScaledSprites);
		UsingTexture(texture);
		Vertex_ScaledSprite V[] = { Vector2(x, y), width, height, uv, color };
		AddPrimitive(V);
	}

	void BatchRenderer::DrawTransformedSprite(const Texture& texture, float x, float y, float width, float height,
		FloatRect uv, Vector2 rotationOrigin, float rotation, Color32 color)
	{
		UsingBatch((BatchType)StandardBatchType::TransformedSprites);
		UsingTexture(texture);
		Vertex_TransformedSprite V[] = { Vector2(x,y), width, height, uv, rotationOrigin, rotation, color };
		AddPrimitive(V);
	}



} //end of namespace ig