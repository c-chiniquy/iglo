#include "../common/BatchRendererCommon.hlsl"

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants : register(b0);

float4 PSMain(float4 position : SV_POSITION, float4 color : COLOR, float2 texCoord : TEXCOORD0) : SV_TARGET
{
	ConstantBuffer<TextureConstants> tc = ResourceDescriptorHeap[pushConstants.textureConstantsIndex];
	ConstantBuffer<SDFEffect> sdf = ResourceDescriptorHeap[pushConstants.renderConstantsIndex];
	Texture2D shaderTexture = ResourceDescriptorHeap[pushConstants.textureIndex];
	SamplerState samplerState = SamplerDescriptorHeap[pushConstants.samplerIndex];

	// Scale anti alias based on the size of a screen pixel
	// so the sprite always looks smooth no matter its apparent size.
	// Determining the size of a screen pixel is done with ddx and ddy, and this even works when rotation is involved.
	float2 derivativeX = ddx(float2(texCoord.x * tc.textureSize.x, texCoord.y * tc.textureSize.y));
	float2 derivativeY = ddy(float2(texCoord.x * tc.textureSize.x, texCoord.y * tc.textureSize.y));
	float scaledSmoothing = sdf.smoothing * distance(derivativeX, derivativeY);

	float dist = shaderTexture.Sample(samplerState, texCoord).r;
	float outerEdge = 0.5f;
	float baseAlpha = 0;

	float4 finalColor;

	if (sdf.sdfEffectFlags & 0x1) // Outline effect
	{
		outerEdge = 0.5 - sdf.outlineThickness;
		baseAlpha = smoothstep(outerEdge - scaledSmoothing, outerEdge + scaledSmoothing, dist);
		float borderTransition = smoothstep(0.5 - scaledSmoothing, 0.5 + scaledSmoothing, dist);
		finalColor = lerp(sdf.outlineColor, color, borderTransition) * baseAlpha;
	}
	else
	{
		// No outline effect
		baseAlpha = smoothstep(0.5 - scaledSmoothing, 0.5 + scaledSmoothing, dist);
		finalColor = float4(1.0f, 1.0f, 1.0f, baseAlpha) * color;
	}

	if (sdf.sdfEffectFlags & 0x2) // Glow effect
	{
		float glowDist = shaderTexture.Sample(samplerState, texCoord - (sdf.glowOffset * tc.inverseTextureSize)).r;
		float glowAlpha = smoothstep(outerEdge - sdf.glowSize, outerEdge + sdf.glowSize, glowDist);
		float4 glow = float4(sdf.glowColor.rgb, sdf.glowColor.a * glowAlpha);
		finalColor = lerp(glow, finalColor, baseAlpha);
	}

	return finalColor;
}