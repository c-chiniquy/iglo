#include "../common/BatchRendererCommon.hlsl"

ConstantBuffer<PushConstants> pushConstants : register(b0);

struct VertexInput
{
	float2 position;
	float radius;
	float smoothing;
	float borderThickness;
	uint innerColor;
	uint outerColor;
	uint borderColor;
};

struct PixelInput
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD0;
	float radius : RADIUS;
	float smoothing : SMOOTHING;
	float borderThickness : BORDERTHICKNESS;
	float4 innerColor : COLOR0;
	float4 outerColor : COLOR1;
	float4 borderColor : COLOR2;
};

PixelInput VSMain(uint vertexID : SV_VertexID)
{
	StructuredBuffer<VertexInput> buffer = ResourceDescriptorHeap[pushConstants.rawOrStructuredBufferIndex];
	ConstantBuffer<MatrixConstant> world = ResourceDescriptorHeap[pushConstants.worldMatrixIndex];
	ConstantBuffer<MatrixConstant> viewProj = ResourceDescriptorHeap[pushConstants.viewProjMatrixIndex];

	uint elementIndex = (vertexID / 6);
	uint cornerIndex = (vertexID % 6);

	VertexInput input = buffer[elementIndex];

	// Border size and anti alias smoothing will expand the quad
	float e = input.radius + (input.borderThickness * 0.5f) + (input.smoothing * 0.5f);
	float2 quad[6] = { float2(-e, -e), float2(e, -e), float2(-e, e), float2(-e, e), float2(e, -e), float2(e, e) };
	float4 pos4 = float4(input.position + quad[cornerIndex], 0.0f, 1.0f);

	PixelInput output;
	output.pos = mul(pos4, world.m);
	output.pos = mul(output.pos, viewProj.m);
	output.texCoord = quad[cornerIndex];
	output.radius = input.radius;
	output.smoothing = input.smoothing;
	output.borderThickness = input.borderThickness;
	output.innerColor = ConvertToFloat4(input.innerColor);
	output.outerColor = ConvertToFloat4(input.outerColor);
	output.borderColor = ConvertToFloat4(input.borderColor);

	return output;
}
