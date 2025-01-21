
cbuffer PushConstants : register(b0)
{
	uint colorTextureIndex;
	uint depthTextureIndex;
	uint colorSamplerIndex;
	uint depthSamplerIndex;
	uint lightConstantsIndex;
	uint cameraConstantsIndex;
	uint modelConstantsIndex;
};

struct LightConstants
{
	matrix viewAndProj;
	float4 diffuseColor;
	float4 ambientColor;
	float3 position;
};

struct CameraConstants
{
	matrix viewAndProj;
};

struct ModelConstants
{
	matrix world;
};

struct VertexInput
{
	float4 position : POSITION;
	float2 texCoord : TEXCOORD0;
	float3 normal : NORMAL;
};

// Vertex shader used to create the shadow map
float4 VSDepth(VertexInput input) : SV_POSITION
{
	ConstantBuffer<LightConstants> light = ResourceDescriptorHeap[lightConstantsIndex];
	ConstantBuffer<ModelConstants> model = ResourceDescriptorHeap[modelConstantsIndex];

	input.position.w = 1.0f;

	float4 outputPosition = mul(input.position, model.world);
	outputPosition = mul(outputPosition, light.viewAndProj);

	return outputPosition;
}

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;

	// For calculating shadow intensity
	float4 lightViewPos : TEXCOORD1;

	// For calculating light intensity
	float3 normal : NORMAL;
	float3 relativeLightPos : TEXCOORD2;
};

PixelInput VSLightingAndShadows(VertexInput input)
{
	ConstantBuffer<LightConstants> light = ResourceDescriptorHeap[lightConstantsIndex];
	ConstantBuffer<CameraConstants> camera = ResourceDescriptorHeap[cameraConstantsIndex];
	ConstantBuffer<ModelConstants> model = ResourceDescriptorHeap[modelConstantsIndex];

	PixelInput output;

	input.position.w = 1.0f;

	output.position = mul(input.position, model.world);
	output.position = mul(output.position, camera.viewAndProj);

	output.lightViewPos = mul(input.position, model.world);
	output.lightViewPos = mul(output.lightViewPos, light.viewAndProj);

	output.relativeLightPos = light.position - float3(mul(input.position, model.world).xyz);
	output.normal = normalize(mul(input.normal, (float3x3)model.world));

	output.texCoord = input.texCoord;

	return output;
}

float4 PSLightingAndShadows(PixelInput input) : SV_TARGET
{
	ConstantBuffer<LightConstants> light = ResourceDescriptorHeap[lightConstantsIndex];
	Texture2D colorTexture = ResourceDescriptorHeap[colorTextureIndex];
	Texture2D depthTexture = ResourceDescriptorHeap[depthTextureIndex];
	SamplerState colorSampler = SamplerDescriptorHeap[colorSamplerIndex];
	SamplerComparisonState depthSampler = SamplerDescriptorHeap[depthSamplerIndex];

	float bias = 0.00001f;
	float4 color = light.ambientColor;

	float2 shadowMapTexCoord = float2(input.lightViewPos.x, -input.lightViewPos.y) / input.lightViewPos.w;
	shadowMapTexCoord *= 0.5f;
	shadowMapTexCoord += 0.5f;

	// If within the bounds of the shadow map
	if ((saturate(shadowMapTexCoord.x) == shadowMapTexCoord.x) &&
	   (saturate(shadowMapTexCoord.y) == shadowMapTexCoord.y))
	{
		float lightDepthValue = input.lightViewPos.z / input.lightViewPos.w;
		lightDepthValue -= bias;

		// Uses hardware accelerated percentage closer filtering.
		// shadowFactor is set to a value with a range of 0.0f to 1.0f.
		float shadowFactor = depthTexture.SampleCmpLevelZero(depthSampler, shadowMapTexCoord, lightDepthValue);

		// Relative light position has to be normalized in the pixel shader to make per-pixel light intensity accurate
		float3 normalizedLightPos = normalize(input.relativeLightPos);
		float lightIntensity = saturate(dot(input.normal, normalizedLightPos));
		lightIntensity *= shadowFactor;
		if (lightIntensity > 0.0f)
		{
			color += (light.diffuseColor * lightIntensity);
			color = saturate(color);
		}
	}
	color *= colorTexture.Sample(colorSampler, input.texCoord);
	return color;
}