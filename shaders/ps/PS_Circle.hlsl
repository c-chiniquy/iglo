#include "../common/BatchRendererCommon.hlsl"

struct PixelInput
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD0;
	float radius : RADIUS;
	float smoothing : SMOOTHING;
	float borderThickness : BORDERTHICKNESS;
	float4 innerColor : COLOR0;
	float4 outerColor : COLOR1;
	float4 borderColor : COLOR2;
};

float4 CalcInnerColor(float4 innerColor, float4 outerColor, float distance, float radius)
{
	return lerp(innerColor, outerColor, distance / radius);
}
float4 PSMain(PixelInput input) : SV_TARGET
{
	float d = length(input.texCoord); // 0,0 is at the center.
	float halfBorder = input.borderThickness * 0.5f;
	float halfSmooth = input.smoothing * 0.5f;

	// Handle most common situations first (these two if statements increased the pixel shader performance quite a bit)
	if (d < input.radius - halfBorder - halfSmooth) // Fully inside circle
	{
		return CalcInnerColor(input.innerColor, input.outerColor, d, input.radius); // No need for anti alias here
	}
	else if (d > input.radius + halfBorder + halfSmooth) // Fully outside circle
	{
		return float4(0,0,0,0);
	}

	// Draw inner circle
	float4 filledColor = CalcInnerColor(input.innerColor, input.outerColor, d, input.radius);
	filledColor.a *= (1.0f - smoothstep(input.radius - halfSmooth, input.radius + halfSmooth, d)); // Apply anti alias to alpha value
	if (input.borderThickness <= 0 || input.borderColor.a <= 0) return filledColor;

	// Draw circle border
	float t1 = smoothstep(input.radius - halfBorder - halfSmooth, input.radius - halfBorder + halfSmooth, d);
	float t2 = smoothstep(input.radius + halfBorder - halfSmooth, input.radius + halfBorder + halfSmooth, d);
	float t = t1 - t2;
	float4 ringColor = lerp(float4(input.borderColor.rgb, 0), input.borderColor, t);

	// Blend the inner circle and circle border together
	return AlphaBlend(ringColor, filledColor);
}