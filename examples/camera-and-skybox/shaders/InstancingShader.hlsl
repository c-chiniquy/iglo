
cbuffer PushConstants : register(b0)
{
	matrix viewProj;
};

struct VertexInput
{
	float4 position : POSITION;
	float4 color : COLOR;
	float4 instancePos : INSTANCEPOS;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

PixelInput VSMain(VertexInput input)
{
	float4 pos = float4(input.position.xyz, 1.0f);
	pos.x += input.instancePos.x;
	pos.y += input.instancePos.y;
	pos.z += input.instancePos.z;

	PixelInput output;
	output.position = mul(pos, viewProj);
	output.color = input.color;

	return output;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
	return input.color;
}
