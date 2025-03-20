#include "../../../shaders/common/BatchRendererCommon.hlsl"

ConstantBuffer<PushConstants> pushConstants : register(b0);

struct VertexInput
{
	float2 position : POSITION;
	uint width : WIDTH;
	uint height : HEIGHT;
	uint2 texCoord : TEXCOORD0;
	float4 color : COLOR;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 texCoord : TEXCOORD0;
};

PixelInput VSMain(VertexInput input, uint vertexID : SV_VertexID)
{
	ConstantBuffer<MatrixConstant> world = ResourceDescriptorHeap[pushConstants.worldMatrixIndex];
	ConstantBuffer<MatrixConstant> viewProj = ResourceDescriptorHeap[pushConstants.viewProjMatrixIndex];
	ConstantBuffer<TextureConstants> textureConstants = ResourceDescriptorHeap[pushConstants.textureConstantsIndex];

	float2 quadPos[4] = { float2(0, 0), float2(input.width, 0), float2(0, input.height), float2(input.width, input.height) };
	float4 pos4 = float4(input.position + quadPos[vertexID], 0.0f, 1.0f);

	PixelInput output;
	output.position = mul(pos4, world.m);
	output.position = mul(output.position, viewProj.m);
	output.texCoord = ((float2)input.texCoord + quadPos[vertexID]) * textureConstants.inverseTextureSize;
	output.color = input.color;

	return output;
}
