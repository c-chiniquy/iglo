#include "../../../shaders/common/BatchRendererCommon.hlsl"

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants : register(b0);

struct VertexInput
{
	float2 position;
	uint size;
	uint uv;
	uint color;
#ifdef STRUCTURED_BUFFER
	uint padding; // Vulkan requires an alignment of 8 for structured buffers.
#endif
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 texCoord : TEXCOORD0;
};

float4 ConvertToFloat4(uint color32)
{
	return float4(
		(float)(color32 & 0xFF) / 255.0f,
		(float)((color32 >> 8) & 0xFF) / 255.0f,
		(float)((color32 >> 16) & 0xFF) / 255.0f,
		(float)((color32 >> 24) & 0xFF) / 255.0f);
}

PixelInput VSMain(uint vertexID : SV_VertexID)
{
	#ifdef STRUCTURED_BUFFER
	StructuredBuffer<VertexInput> buffer = ResourceDescriptorHeap[pushConstants.rawOrStructuredBufferIndex];
	#endif

	#ifdef RAW_BUFFER
	ByteAddressBuffer buffer = ResourceDescriptorHeap[pushConstants.rawOrStructuredBufferIndex];
	#endif

	ConstantBuffer<MatrixConstant> world = ResourceDescriptorHeap[pushConstants.worldMatrixIndex];
	ConstantBuffer<MatrixConstant> viewProj = ResourceDescriptorHeap[pushConstants.viewProjMatrixIndex];
	ConstantBuffer<TextureConstants> textureConstants = ResourceDescriptorHeap[pushConstants.textureConstantsIndex];

	uint elementSize = 5 * 4;
	uint elementIndex = (vertexID / 6);
	uint cornerIndex = (vertexID % 6);
	uint offset = elementSize * elementIndex;

	VertexInput input;

#ifdef RAW_BUFFER
	input.position = asfloat(buffer.Load2(offset));
	input.size = buffer.Load(offset + 8);
	input.uv = buffer.Load(offset + 12);
	input.color = buffer.Load(offset + 16);
#endif

#ifdef STRUCTURED_BUFFER
	input = buffer[elementIndex];
#endif

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
