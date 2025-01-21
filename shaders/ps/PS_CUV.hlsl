#include "../common/BatchRendererCommon.hlsl"

ConstantBuffer<PushConstants> pushConstants : register(b0);

float4 PSMain(float4 position : SV_POSITION, float4 color : COLOR, float2 texCoord : TEXCOORD0) : SV_TARGET
{
	Texture2D shaderTexture = ResourceDescriptorHeap[pushConstants.textureIndex];
	SamplerState samplerState = SamplerDescriptorHeap[pushConstants.samplerIndex];
	return shaderTexture.Sample(samplerState, texCoord) * color;
}
