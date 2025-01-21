
cbuffer PushConstants : register(b0)
{
	matrix viewProj;
	float3 cameraPos;
	float padding;
	uint textureIndex;
	uint samplerIndex;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float3 viewDir : TEXCOORD0;
};

PixelInput VSMain(float4 position : POSITION)
{
	float4 pos = float4(position.xyz, 1.0f);

	// The skybox follows the camera
	pos.x += cameraPos.x;
	pos.y += cameraPos.y;
	pos.z += cameraPos.z;

	PixelInput output;
	output.position = mul(pos, viewProj);
	output.viewDir = position.xyz;

	return output;
}

float4 PSMain(PixelInput input) : SV_Target
{
	TextureCube cubemapTexture = ResourceDescriptorHeap[textureIndex];
	SamplerState samplerState = SamplerDescriptorHeap[samplerIndex];
    return cubemapTexture.Sample(samplerState, normalize(input.viewDir));
}