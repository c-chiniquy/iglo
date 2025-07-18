#include "../common/BatchRendererCommon.hlsl"

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants : register(b0);

struct PixelInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 texCoord : TEXCOORD0;
};

PixelInput VSMain(float2 position : POSITION, float4 color : COLOR, float2 texCoord : TEXCOORD0)
{
	ConstantBuffer<MatrixConstant> world = ResourceDescriptorHeap[pushConstants.worldMatrixIndex];
	ConstantBuffer<MatrixConstant> viewProj = ResourceDescriptorHeap[pushConstants.viewProjMatrixIndex];

	PixelInput output;

	float4 pos4 = float4(position.x, position.y, 0.0f, 1.0f);
	output.position = mul(pos4, world.m);
	output.position = mul(output.position, viewProj.m);
	output.texCoord = texCoord;
	output.color = color;

	return output;
}
