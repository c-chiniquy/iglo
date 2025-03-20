#include "../common/BatchRendererCommon.hlsl"

ConstantBuffer<PushConstants> pushConstants : register(b0);

struct VertexInput
{
	float2 position;
	uint size;
	uint uv;
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

	uint elementSize = 5 * 4;
	uint elementIndex = (vertexID / 6);
	uint cornerIndex = (vertexID % 6);

	VertexInput input = buffer[elementIndex];

	uint width = input.size & 0xFFFF;
	uint height = (input.size >> 16) & 0xFFFF;
	uint2 uv = uint2(input.uv & 0xFFFF, (input.uv >> 16) & 0xFFFF);

	float2 quad[6] = { float2(0, 0), float2(width, 0), float2(0, height),
					   float2(0, height), float2(width, 0), float2(width, height) };
	float4 pos4 = float4(input.position + quad[cornerIndex], 0.0f, 1.0f);

	PixelInput output;
	output.position = mul(pos4, world.m);
	output.position = mul(output.position, viewProj.m);
	output.texCoord = ((float2)uv + quad[cornerIndex]) * textureConstants.inverseTextureSize;
	output.color = ConvertToFloat4(input.color);

	return output;
}
