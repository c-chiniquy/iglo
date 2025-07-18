
struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

PixelInput VSMain(uint vertexID : SV_VertexID)
{
	float2 positions[4] = { float2(-1,  1), float2(1,  1), float2(-1, -1), float2(1, -1) };
	float2 uvs[4] = { float2(0, 0), float2(1, 0), float2(0, 1), float2(1, 1) };

	PixelInput output;
	output.position = float4(positions[vertexID], 0.0f, 1.0f);
	output.uv = uvs[vertexID];
	return output;
}
