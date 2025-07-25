
struct PushConstants
{
	matrix viewProj;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants : register(b0);

struct VertexInput
{
	float4 position : POSITION;
	float4 color : COLOR;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

PixelInput VSMain(VertexInput input)
{
	float4 pos = float4(input.position.xyz, 1.0f);

	PixelInput output;
	output.position = mul(pos, pushConstants.viewProj);
	output.color = input.color;

	return output;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
	return input.color;
}
