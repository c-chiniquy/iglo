
struct PushConstants
{
	matrix viewProj;
	float3 cameraPos;
	float padding;
	uint textureIndex;
	uint samplerIndex;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants : register(b0);

struct PixelInput
{
	float4 position : SV_POSITION;
	float3 viewDir : TEXCOORD0;
};

PixelInput VSMain(float4 position : POSITION)
{
	// The skybox follows the camera
	float4 pos = float4(position.xyz + pushConstants.cameraPos, 1.0f);

	PixelInput output;
	output.position = mul(pos, pushConstants.viewProj);
	output.viewDir = position.xyz;

	return output;
}

float4 PSMain(PixelInput input) : SV_Target
{
	TextureCube cubemapTexture = ResourceDescriptorHeap[pushConstants.textureIndex];
	SamplerState samplerState = SamplerDescriptorHeap[pushConstants.samplerIndex];
    return cubemapTexture.Sample(samplerState, normalize(input.viewDir));
}