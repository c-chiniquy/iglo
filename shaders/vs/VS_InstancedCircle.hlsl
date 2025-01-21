#include "../common/BatchRendererCommon.hlsl"

ConstantBuffer<PushConstants> pushConstants : register(b0);

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

PixelInput VSMain(float2 position : POSITION, float radius : RADIUS, float smoothing : SMOOTHING, float borderThickness : BORDERTHICKNESS,
	float4 innerColor : INNERCOLOR, float4 outerColor : OUTERCOLOR, float4 borderColor : BORDERCOLOR, uint vertexID : SV_VertexID)
{
	ConstantBuffer<MatrixConstant> world = ResourceDescriptorHeap[pushConstants.worldMatrixIndex];
	ConstantBuffer<MatrixConstant> viewProj = ResourceDescriptorHeap[pushConstants.viewProjMatrixIndex];
	// Border size and anti alias smoothing will expand the quad
	float e = radius + (borderThickness * 0.5f) + (smoothing * 0.5f);
	float2 quad[4] = { float2(-e, -e), float2(e, -e), float2(-e, e), float2(e, e) };
	float4 pos4 = float4(position + quad[vertexID], 0.0f, 1.0f);

	PixelInput output;
	output.pos = mul(pos4, world.m);
	output.pos = mul(output.pos, viewProj.m);
	output.texCoord = quad[vertexID];
	output.radius = radius;
	output.smoothing = smoothing;
	output.borderThickness = borderThickness;
	output.innerColor = innerColor;
	output.outerColor = outerColor;
	output.borderColor = borderColor;

	return output;
}
