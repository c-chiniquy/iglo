
struct PushConstants
{
	uint2 srcTextureSize;
	uint2 destTextureSize;
	uint is_sRGB; // 1 if texture is sRGB, 0 otherwise
	uint bilinearClampSamplerIndex;
	uint srcTextureIndex;
	uint destTextureIndex;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants : register(b0);

float3 LinearToSRGB(float3 linearColor)
{
	float3 lowSegment = linearColor * 12.92;
	float3 highSegment = 1.055 * pow(abs(linearColor), 1.0 / 2.4) - 0.055;
	return select(linearColor < 0.0031308, lowSegment, highSegment);
}

struct AxisTaps
{
	uint offset[3];
	float weight[3];
};

AxisTaps ComputeAxisTaps(uint srcSize, uint destSize, uint i)
{
	AxisTaps t;
	uint base = i << 1;

	t.offset[0] = base;
	t.offset[1] = base;
	t.offset[2] = base;
	t.weight[0] = 0.0;
	t.weight[1] = 0.0;
	t.weight[2] = 0.0;

	if (srcSize == 1)
	{
		// Single pixel
		t.offset[0] = 0;
		t.weight[0] = 1.0;
	}
	else if ((srcSize & 1) == 0)
	{
		// Even
		t.offset[0] = base;
		t.offset[1] = base + 1;
		t.weight[0] = 0.5;
		t.weight[1] = 0.5;
	}
	else
	{
		// Odd
		float S = float(srcSize);
		float D = float(destSize);
		float fi = float(i);
		t.offset[0] = base;
		t.offset[1] = base + 1;
		t.offset[2] = base + 2;
		t.weight[0] = (D - fi) / S;
		t.weight[1] = D / S;
		t.weight[2] = (fi + 1.0) / S;
	}
	return t;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
	if (id.x >= pushConstants.destTextureSize.x ||
		id.y >= pushConstants.destTextureSize.y) return;

	Texture2D<float4> srcTexture = ResourceDescriptorHeap[pushConstants.srcTextureIndex];
	RWTexture2D<float4> destTexture = ResourceDescriptorHeap[pushConstants.destTextureIndex];
	SamplerState bilinearClampSampler = SamplerDescriptorHeap[pushConstants.bilinearClampSamplerIndex];

	AxisTaps tx = ComputeAxisTaps(pushConstants.srcTextureSize.x, pushConstants.destTextureSize.x, id.x);
	AxisTaps ty = ComputeAxisTaps(pushConstants.srcTextureSize.y, pushConstants.destTextureSize.y, id.y);

	float2 invSrc = 1.0 / float2(pushConstants.srcTextureSize);

	float4 color = float4(0, 0, 0, 0);
	[unroll] for (uint y = 0; y < 3; y++)
	{
		[unroll] for (uint x = 0; x < 3; x++)
		{
			float w = tx.weight[x] * ty.weight[y];
			if (w != 0.0)
			{
				float2 texel = float2(float(tx.offset[x]), float(ty.offset[y])) + 0.5;
				float2 uv = texel * invSrc;
				color += w * srcTexture.SampleLevel(bilinearClampSampler, uv, 0);
			}
		}
	}

	if (pushConstants.is_sRGB)
	{
		// Only convert RGB components, leave alpha linear
		color.rgb = LinearToSRGB(color.rgb);
	}

	destTexture[id.xy] = color;
}
