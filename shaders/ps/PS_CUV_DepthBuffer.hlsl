#include "../common/BatchRendererCommon.hlsl"

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants : register(b0);

#ifdef VULKAN
typedef Texture2DMS<uint> StencilTextureMS;
typedef Texture2D<uint> StencilTexture;
uint LoadStencilValueMS(StencilTextureMS stencilTextureMS, float2 texCoord, float2 textureSize, uint sampleIndex)
{
	return stencilTextureMS.Load(int2(texCoord * textureSize), sampleIndex);
}
uint LoadStencilValue(StencilTexture stencilTexture, float2 texCoord, float2 textureSize)
{
	return stencilTexture.Load(int3(texCoord * textureSize, 0));
}
#else
typedef Texture2DMS<uint2> StencilTextureMS;
typedef Texture2D<uint2> StencilTexture;
uint LoadStencilValueMS(StencilTextureMS stencilTextureMS, float2 texCoord, float2 textureSize, uint sampleIndex)
{
	return stencilTextureMS.Load(int2(texCoord * textureSize), sampleIndex).y;
}
uint LoadStencilValue(StencilTexture stencilTexture, float2 texCoord, float2 textureSize)
{
	return stencilTexture.Load(int3(texCoord * textureSize, 0)).y;
}
#endif

float4 PSMain(float4 position : SV_POSITION, float4 color : COLOR, float2 texCoord : TEXCOORD0) : SV_TARGET
{
	ConstantBuffer<TextureConstants> textureConstants = ResourceDescriptorHeap[pushConstants.textureConstantsIndex];
	SamplerState samplerState = SamplerDescriptorHeap[pushConstants.samplerIndex];
	ConstantBuffer<DepthBufferDrawStyle> renderConstants = ResourceDescriptorHeap[pushConstants.renderConstantsIndex];
	if (renderConstants.depthOrStencilComponent == 0)
	{ 
		// Draw depth component
		float mono;
		if (textureConstants.msaa > 1)
		{
			// Multisampled
			Texture2DMS<float> textureMS = ResourceDescriptorHeap[pushConstants.textureIndex];
			float value = 0;
			for (uint i = 0; i < textureConstants.msaa; i++)
			{
				value += textureMS.Load(int2(texCoord * textureConstants.textureSize), i);
			}
			mono = 1 - LinearizeDepth(value / float(textureConstants.msaa), renderConstants.zNear, renderConstants.zFar);
		}
		else
		{
			// Not multisampled
			Texture2D<float> textureRegular = ResourceDescriptorHeap[pushConstants.textureIndex];
			float value = textureRegular.Sample(samplerState, texCoord).r;
			mono = 1 - LinearizeDepth(value, renderConstants.zNear, renderConstants.zFar);
		}
		return float4(mono, mono, mono, 1) * color;
	}
	else
	{
		// Draw stencil component
		float mono;
		if (textureConstants.msaa > 1)
		{
			// Multisampled
			StencilTextureMS textureMS = ResourceDescriptorHeap[pushConstants.textureIndex];
			uint value = 0;
			for (uint i = 0; i < textureConstants.msaa; i++)
			{
				value += LoadStencilValueMS(textureMS, texCoord, textureConstants.textureSize, i);
			}
			mono = 1.0f - (value / (255.0f * textureConstants.msaa));
		}
		else
		{
			// Not multisampled
			StencilTexture textureRegular = ResourceDescriptorHeap[pushConstants.textureIndex];
			uint value = LoadStencilValue(textureRegular, texCoord, textureConstants.textureSize);
			mono = 1.0f - (value / 255.0f);
		}
		return float4(mono, mono, mono, 1) * color;
	}
}
