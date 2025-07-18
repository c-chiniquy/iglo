
struct PushConstants
{
	uint textureIndex;
	uint samplerIndex;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants : register(b0);

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float4 PSMain(PixelInput input) : SV_Target
{
	Texture2D shaderTexture = ResourceDescriptorHeap[pushConstants.textureIndex];
	SamplerState samplerState = SamplerDescriptorHeap[pushConstants.samplerIndex];

	return shaderTexture.Sample(samplerState, input.uv);
}