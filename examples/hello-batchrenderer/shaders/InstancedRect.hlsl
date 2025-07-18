#include "../../../shaders/common/BatchRendererCommon.hlsl"

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants : register(b0);

struct VertexInput
{
	float2 position : POSITION;
	float width : WIDTH;
	float height : HEIGHT;
	float4 color : COLOR;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

PixelInput VSMain(VertexInput input, uint vertexID : SV_VertexID)
{
	ConstantBuffer<MatrixConstant> world = ResourceDescriptorHeap[pushConstants.worldMatrixIndex];
	ConstantBuffer<MatrixConstant> viewProj = ResourceDescriptorHeap[pushConstants.viewProjMatrixIndex];

	float2 quad[4] = { float2(0, 0), float2(input.width, 0), float2(0, input.height), float2(input.width, input.height) };
	float4 pos4 = float4(input.position + quad[vertexID], 0.0f, 1.0f);

	PixelInput output;
	output.position = mul(pos4, world.m);
	output.position = mul(output.position, viewProj.m);
	output.color = input.color;

	return output;
}
