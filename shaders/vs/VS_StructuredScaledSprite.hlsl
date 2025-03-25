#include "../common/BatchRendererCommon.hlsl"

ConstantBuffer<PushConstants> pushConstants : register(b0);

struct VertexInput
{
	float2 position;
	float width;
	float height;
	float4 uvRect;
	uint color;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 texCoord : TEXCOORD0;
};

PixelInput VSMain(uint vertexID : SV_VertexID)
{
	StructuredBuffer<VertexInput> buffer = ResourceDescriptorHeap[pushConstants.rawOrStructuredBufferIndex];
	ConstantBuffer<MatrixConstant> world = ResourceDescriptorHeap[pushConstants.worldMatrixIndex];
	ConstantBuffer<MatrixConstant> viewProj = ResourceDescriptorHeap[pushConstants.viewProjMatrixIndex];
	ConstantBuffer<TextureConstants> textureConstants = ResourceDescriptorHeap[pushConstants.textureConstantsIndex];

	uint elementIndex = (vertexID / 6);
	uint cornerIndex = (vertexID % 6);

	VertexInput input = buffer[elementIndex];

	float2 quad[6] = { float2(0, 0), float2(input.width, 0), float2(0, input.height),
				   float2(0, input.height), float2(input.width, 0), float2(input.width, input.height) };
	float2 quadUV[6] = { input.uvRect.xy, input.uvRect.zy, input.uvRect.xw, input.uvRect.xw, input.uvRect.zy, input.uvRect.zw };
	float4 pos4 = float4(input.position + quad[cornerIndex], 0.0f, 1.0f);

	PixelInput output;
	output.position = mul(pos4, world.m);
	output.position = mul(output.position, viewProj.m);
	output.texCoord = quadUV[cornerIndex] * textureConstants.inverseTextureSize;
	output.color = ConvertToFloat4(input.color);

	return output;
}
