
cbuffer PushConstants : register(b0)
{
	uint textureIndex;
	uint samplerIndex;
	uint constantIndex;
};

struct MatrixConstants
{
	matrix viewProj;
	matrix world;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

PixelInput VSMain(float4 position : POSITION, float2 texCoord : TEXCOORD0)
{
	ConstantBuffer<MatrixConstants> matrices = ResourceDescriptorHeap[constantIndex];

	PixelInput output;

	position.w = 1.0f;
	output.position = mul(position, matrices.world);
	output.position = mul(output.position, matrices.viewProj);
	output.texCoord = texCoord;

	return output;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
	Texture2D shaderTexture = ResourceDescriptorHeap[textureIndex];
	SamplerState textureSampler = SamplerDescriptorHeap[samplerIndex];

	float4 output = shaderTexture.Sample(textureSampler, input.texCoord);
	return output;
}
