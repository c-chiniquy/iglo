
cbuffer PushConstants : register(b0)
{
	uint srcTextureIndex;
	uint destTextureIndex;
	uint bilinearClampSamplerIndex;
	uint padding;
	float2 inverseDestTextureSize;
}

//TODO: Make this work with sRGB texture formats and make sure it handles gamma correctly.

[numthreads( 8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
	Texture2D<float4> srcTexture = ResourceDescriptorHeap[srcTextureIndex];
	RWTexture2D<float4> destTexture = ResourceDescriptorHeap[destTextureIndex];
	SamplerState bilinearClampSampler = SamplerDescriptorHeap[bilinearClampSamplerIndex];

	float2 texCoords = (id.xy + 0.5) * inverseDestTextureSize;
	float4 color = srcTexture.SampleLevel(bilinearClampSampler, texCoords, 0);

	destTexture[id.xy] = color;
}
