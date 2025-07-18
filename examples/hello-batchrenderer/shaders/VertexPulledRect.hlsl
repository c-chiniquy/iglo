#include "../../../shaders/common/BatchRendererCommon.hlsl"

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants : register(b0);

struct VertexInput
{
	float2 position;
	float width;
	float height;
	uint color;
#ifdef STRUCTURED_BUFFER
	uint padding; // Vulkan requires an alignment of 8 for structured buffers.
#endif
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
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

	uint elementSize = 5 * 4;
	uint elementIndex = (vertexID / 6);
	uint cornerIndex = (vertexID % 6);
	uint offset = elementSize * elementIndex;

	VertexInput input;

#ifdef RAW_BUFFER
	input.position = asfloat(buffer.Load2(offset));
	input.width = asfloat(buffer.Load(offset + 8));
	input.height = asfloat(buffer.Load(offset + 12));
	input.color = buffer.Load(offset + 16);
#endif

#ifdef STRUCTURED_BUFFER
	input = buffer[elementIndex]; 
#endif

	float2 quad[6] = { float2(0, 0), float2(input.width, 0), float2(0, input.height),
					   float2(0, input.height), float2(input.width, 0), float2(input.width, input.height) };
	float4 pos4 = float4(input.position + quad[cornerIndex], 0.0f, 1.0f);

	PixelInput output;
	output.position = mul(pos4, world.m);
	output.position = mul(output.position, viewProj.m);
	output.color = ConvertToFloat4(input.color);

	return output;
}
