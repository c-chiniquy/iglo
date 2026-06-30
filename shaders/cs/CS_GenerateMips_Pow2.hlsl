
struct PushConstants
{
	float2 inverseDestTextureSize;
	uint2 destTextureSize;
	uint numMips; // Can generate max 4 mips at once
	uint is_sRGB; // 1 if texture is sRGB, 0 otherwise
	uint bilinearClampSamplerIndex;
	uint srcTextureIndex;
	uint destTextureIndex0;
	uint destTextureIndex1;
	uint destTextureIndex2;
	uint destTextureIndex3;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants : register(b0);

groupshared float4 gs_tile[64];

float3 LinearToSRGB(float3 linearColor)
{
	float3 lowSegment = linearColor * 12.92;
	float3 highSegment = 1.055 * pow(abs(linearColor), 1.0 / 2.4) - 0.055;
	return select(linearColor < 0.0031308, lowSegment, highSegment);
}

void GenMip(RWTexture2D<float4> dest, uint localThreadIndex, uint3 globalThread, uint level)
{
	const uint i = localThreadIndex;
	const float4 total =
		gs_tile[i] +
		gs_tile[i + (0b000001u << level)] +
		gs_tile[i + (0b001000u << level)] +
		gs_tile[i + (0b001001u << level)];
	float4 avgColor = total * 0.25;

	gs_tile[i] = avgColor;

	if (pushConstants.is_sRGB) avgColor.rgb = LinearToSRGB(avgColor.rgb);

	uint2 texCoords = globalThread.xy >> (level + 1);
	uint2 mipSize = pushConstants.destTextureSize >> (level + 1);
	if (all(texCoords < mipSize)) dest[texCoords] = avgColor;
}

[numthreads(8, 8, 1)]
void CSMain(
	uint3 globalThread : SV_DispatchThreadID,
	uint3 globalGroup : SV_GroupID,
	uint3 localThread : SV_GroupThreadID, // 0-7, 0-7, 0
	uint localThreadIndex : SV_GroupIndex) // 0-63
{
	SamplerState smp = SamplerDescriptorHeap[pushConstants.bilinearClampSamplerIndex];
	Texture2D<float4> src = ResourceDescriptorHeap[pushConstants.srcTextureIndex];

	RWTexture2D<float4> dest0 = ResourceDescriptorHeap[pushConstants.destTextureIndex0];
	RWTexture2D<float4> dest1;
	RWTexture2D<float4> dest2;
	RWTexture2D<float4> dest3;

	if (globalThread.x < pushConstants.destTextureSize.x &&
		globalThread.y < pushConstants.destTextureSize.y)
	{
		float2 texCoords = (float2(globalThread.xy) + 0.5) * pushConstants.inverseDestTextureSize;
		float4 color = src.SampleLevel(smp, texCoords, 0); // Explicit LOD is required in compute shaders

		gs_tile[localThreadIndex] = color;

		// Only convert RGB components, leave alpha linear
		if (pushConstants.is_sRGB) color.rgb = LinearToSRGB(color.rgb);

		dest0[globalThread.xy] = color;
	}

	GroupMemoryBarrierWithGroupSync();

	if (pushConstants.numMips >= 2 && ((localThreadIndex & 0b001001) == 0)) // yyyxxx, 1 in 4 threads move on
	{
		dest1 = ResourceDescriptorHeap[pushConstants.destTextureIndex1];
		GenMip(dest1, localThreadIndex, globalThread, 0);
	}

	GroupMemoryBarrierWithGroupSync();

	if (pushConstants.numMips >= 3 && ((localThreadIndex & 0b011011) == 0)) // yyyxxx, 1 in 16 threads move on
	{
		dest2 = ResourceDescriptorHeap[pushConstants.destTextureIndex2];
		GenMip(dest2, localThreadIndex, globalThread, 1);
	}

	GroupMemoryBarrierWithGroupSync();

	if (pushConstants.numMips >= 4 && ((localThreadIndex & 0b111111) == 0)) // yyyxxx, 1 in 64 threads move on
	{
		dest3 = ResourceDescriptorHeap[pushConstants.destTextureIndex3];
		GenMip(dest3, localThreadIndex, globalThread, 2);
	}
}
