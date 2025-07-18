#include "../../../shaders/common/BatchRendererCommon.hlsl"

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants : register(b0);

struct WobbleConstants
{
    float animation;
    float intensityX;
    float intensityY;
};

float4 PSMain(float4 position : SV_POSITION, float4 color : COLOR, float2 texCoord : TEXCOORD0) : SV_TARGET
{
	Texture2D shaderTexture = ResourceDescriptorHeap[pushConstants.textureIndex];
	SamplerState samplerState = SamplerDescriptorHeap[pushConstants.samplerIndex];
    ConstantBuffer<WobbleConstants> wobble = ResourceDescriptorHeap[pushConstants.renderConstantsIndex];

    float wobbleX = sin((texCoord.y + wobble.animation) * wobble.intensityX) * 0.05;
    float wobbleY = cos((texCoord.x + wobble.animation) * wobble.intensityY) * 0.05;
    float2 wobbleTexCoord = texCoord + float2(wobbleX, wobbleY);

    return shaderTexture.Sample(samplerState, wobbleTexCoord) * color;
}
