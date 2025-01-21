#include "../common/BatchRendererCommon.hlsl"

ConstantBuffer<PushConstants> pushConstants : register(b0);

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
			Texture2DMS<uint2> textureMS = ResourceDescriptorHeap[pushConstants.stencilComponentTextureIndex];
			uint2 value = uint2(0, 0);
			for (uint i = 0; i < textureConstants.msaa; i++)
			{
				value += textureMS.Load(int2(texCoord * textureConstants.textureSize), i);
			}
			mono = 1.0f - (value.y / (255.0 * textureConstants.msaa));
		}
		else
		{
			// Not multisampled
			Texture2D<uint2> textureRegular = ResourceDescriptorHeap[pushConstants.stencilComponentTextureIndex];
			uint2 value = textureRegular.Load(int3(texCoord * textureConstants.textureSize, 0));
			mono = 1.0f - (value.y / 255.0);
		}
		return float4(mono, mono, mono, 1) * color;
	}
}
