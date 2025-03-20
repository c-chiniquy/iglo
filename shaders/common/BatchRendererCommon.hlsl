
//////////////////////
// Helper functions //
//////////////////////

#define INV_SQRT2 0.70710678118

float4 AlphaBlend(float4 srcColor, float4 destColor)
{
	float srcAlpha = srcColor.a;
	float destAlpha = destColor.a;
	float outAlpha = srcAlpha + destAlpha * (1.0 - srcAlpha);
	float3 outColor = ((srcColor.rgb * srcAlpha) + (destColor.rgb * destAlpha * (1.0 - srcAlpha))) / outAlpha;
	return float4(outColor, outAlpha);
}

float LinearizeDepth(float depth, float zNear, float zFar)
{
	return ((2.0 * zNear) / (zFar + zNear - depth * (zFar - zNear)));
}

float4 ConvertToFloat4(uint color32)
{
	return float4(
		(float)(color32 & 0xFF) / 255.0f,
		(float)((color32 >> 8) & 0xFF) / 255.0f,
		(float)((color32 >> 16) & 0xFF) / 255.0f,
		(float)((color32 >> 24) & 0xFF) / 255.0f);
}

////////////////////////////////////
// BatchRenderer shader constants //
////////////////////////////////////

struct PushConstants
{
	uint textureIndex;
	uint stencilComponentTextureIndex; // Only for depth buffers with stencil component
	uint samplerIndex;
	uint worldMatrixIndex;
	uint viewProjMatrixIndex;
	uint textureConstantsIndex;
	uint renderConstantsIndex;
	uint rawOrStructuredBufferIndex;
};

struct TextureConstants
{
	float2 textureSize;
	float2 inverseTextureSize;
	uint msaa;
};

struct MatrixConstant
{
	matrix m;
};

struct SDFEffect
{
	float4 outlineColor;
	float4 glowColor;
	float smoothing;
	float outlineThickness;
	float2 glowOffset;
	float glowSize;
	uint sdfEffectFlags;
};

struct DepthBufferDrawStyle
{
	uint depthOrStencilComponent; // 0 = draw depth component, 1 = draw stencil component
	float zNear; // When drawing depth component
	float zFar; // When drawing depth component
};
