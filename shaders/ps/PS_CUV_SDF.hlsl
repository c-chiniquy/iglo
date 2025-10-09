#include "../common/BatchRendererCommon.hlsl"

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants : register(b0);

float4 PSMain(float4 position : SV_POSITION, float4 color : COLOR, float2 texCoord : TEXCOORD0) : SV_TARGET
{
	ConstantBuffer<TextureConstants> tc = ResourceDescriptorHeap[pushConstants.textureConstantsIndex];
	ConstantBuffer<SDFEffect> sdf = ResourceDescriptorHeap[pushConstants.renderConstantsIndex];
	Texture2D shaderTexture = ResourceDescriptorHeap[pushConstants.textureIndex];
	SamplerState samplerState = SamplerDescriptorHeap[pushConstants.samplerIndex];

	// Wait with applying vertex alpha
	const float inputColorAlpha = color.a;
	color.a = 1.0f;

	// Scale anti alias based on the size of a screen pixel
	// so the sprite always looks smooth no matter its apparent size.
	// Determining the size of a screen pixel is done with ddx and ddy, and this even works when rotation is involved.
	const float2 derivativeX = ddx(float2(texCoord.x * tc.textureSize.x, texCoord.y * tc.textureSize.y));
	const float2 derivativeY = ddy(float2(texCoord.x * tc.textureSize.x, texCoord.y * tc.textureSize.y));
	const float scaledSmoothing = sdf.smoothing * distance(derivativeX, derivativeY);

	const float dist = shaderTexture.Sample(samplerState, texCoord).r;
	float outerEdge = 0.5f;

	float baseAlpha;
	float4 finalColor;
	
	// Outline effect
	if (sdf.sdfEffectFlags & 0x1) 
	{
		outerEdge -= sdf.outlineThickness;
		baseAlpha = smoothstep(outerEdge - scaledSmoothing, outerEdge + scaledSmoothing, dist);
		float borderTransition = smoothstep(0.5 - scaledSmoothing, 0.5 + scaledSmoothing, dist);
		const float4 outlineColorNoAlpha = float4(sdf.outlineColor.rgb, 1.0f);
		finalColor = lerp(outlineColorNoAlpha, color, borderTransition) * baseAlpha;
	}
	else // No outline effect
	{
		baseAlpha = smoothstep(outerEdge - scaledSmoothing, outerEdge + scaledSmoothing, dist);
		finalColor = float4(1.0f, 1.0f, 1.0f, baseAlpha) * color;
	}

	// Glow effect
	if (sdf.sdfEffectFlags & 0x2) 
	{
		if (baseAlpha < 1.0f)
		{
			float glowDist = shaderTexture.Sample(samplerState, texCoord - (sdf.glowOffset * tc.inverseTextureSize)).r;
			float glowAlpha = smoothstep(outerEdge - sdf.glowSize, outerEdge + sdf.glowSize, glowDist);
			float4 glow = float4(sdf.glowColor.rgb, sdf.glowColor.a * glowAlpha);

			if (baseAlpha <= 0.0f)
			{
				finalColor = glow;
			}
			else
			{
				finalColor = AlphaBlend(finalColor, glow);
			}
		}
	}

	// Apply vertex alpha to everything
	finalColor.a *= inputColorAlpha;
	return finalColor;
}