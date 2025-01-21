
#include "../../../src/shaders/hlsl/common/BatchRendererCommon.hlsl"

ConstantBuffer<PushConstants> pushConstants : register(b0);

struct VertexInput
{
	float2 position;
	float width;
	float height;
	uint color;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

PixelInput VSMain(uint vertexID : SV_VertexID)
{
	#ifdef STRUCTURED_BUFFER
	StructuredBuffer<VertexInput> buffer = ResourceDescriptorHeap[pushConstants.rawOrStructuredBufferIndex]; // Structured buffer
	#endif

	#ifdef RAW_BUFFER
	ByteAddressBuffer buffer = ResourceDescriptorHeap[pushConstants.rawOrStructuredBufferIndex]; // Raw buffer
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
	output.color = float4((input.color & 0xFF) / 255.0,
		((input.color >> 8) & 0xFF) / 255.0,
		((input.color >> 16) & 0xFF) / 255.0,
		((input.color >> 24) & 0xFF) / 255.0);

	return output;
}
