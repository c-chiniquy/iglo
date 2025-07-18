
struct PushConstants
{
	uint srcTextureIndex;
	uint destTextureIndex;
	uint bilinearClampSamplerIndex;
	uint is_sRGB; // 1 if texture is sRGB, 0 otherwise
	float2 inverseDestTextureSize;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants : register(b0);

// Converts linear RGB values to sRGB color space
float3 LinearToSRGB(float3 linearColor)
{
	return select(linearColor < 0.0031308, linearColor * 12.92, 1.055 * pow(abs(linearColor), 1.0 / 2.4) - 0.055);
}

[numthreads( 8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
	Texture2D<float4> srcTexture = ResourceDescriptorHeap[pushConstants.srcTextureIndex];
	RWTexture2D<float4> destTexture = ResourceDescriptorHeap[pushConstants.destTextureIndex];
	SamplerState bilinearClampSampler = SamplerDescriptorHeap[pushConstants.bilinearClampSamplerIndex];

	float2 texCoords = (id.xy + 0.5) * pushConstants.inverseDestTextureSize;
	float4 color = srcTexture.SampleLevel(bilinearClampSampler, texCoords, 0);

	if (pushConstants.is_sRGB)
	{
		// Only convert RGB components, leave alpha linear
		color.rgb = LinearToSRGB(color.rgb);
	}

	destTexture[id.xy] = color;
}
