
// This vertex shader demonstrates vertex pulling, a technique that can be used to draw quads.
// In vertex pulling, vertex data is stored in a raw or structured buffer.
// The vertex shader directly accesses this data to compute the vertices on the fly.
// By default, BatchRenderer instead uses instancing to render quads such as sprites.
// On most modern hardware, instancing is at least as fast or slightly faster than
// vertex pulling for rendering quads. This shader serves as an example of how you can
// use alternative vertex generation methods with BatchRenderer.

#include "../../../shaders/common/BatchRendererCommon.hlsl"

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
	#ifdef STRUCTURED_BUFFER
	StructuredBuffer<VertexInput> buffer = ResourceDescriptorHeap[pushConstants.rawOrStructuredBufferIndex]; // Structured buffer
	#endif

	#ifdef RAW_BUFFER
	ByteAddressBuffer buffer = ResourceDescriptorHeap[pushConstants.rawOrStructuredBufferIndex]; // Raw buffer
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
	output.color = float4((input.color & 0xFF) / 255.0,
		((input.color >> 8) & 0xFF) / 255.0,
		((input.color >> 16) & 0xFF) / 255.0,
		((input.color >> 24) & 0xFF) / 255.0);

	return output;
}
