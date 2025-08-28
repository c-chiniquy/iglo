#pragma once

namespace ig
{

	/*
		These string drawing functions are shown as examples of how you can make your own custom string drawing functions
		with whatever functionality you need.
	*/

	enum class StringAlignment
	{
		TopLeft = 0,
		Top,
		TopRight,
		Left,
		Center,
		Right,
		BottomLeft,
		Bottom,
		BottomRight,
	};

	// Draws a string inside a given rectangle. Characters are clipped to not appear outside the given rectangle.
	// Supports wordwrap.
	void DrawAlignedStringInsideRect(BatchRenderer& r, FloatRect rect, const std::string& str, Font& font, Color32 color,
		StringAlignment align, bool wordWrap, bool pixelAligned);

	// Draws a string inside a given rectangle, but characters are not clipped and may appear outside the rectangle.
	void DrawAlignedString(BatchRenderer& r, FloatRect rect, const std::string& str, Font& font, Color32 color,
		StringAlignment align, bool pixelAligned);

	// The position and color of each character is animated based on 'animationStep'.
	void DrawColorfulAnimatedString(BatchRenderer& r, float x, float y, const std::string& str, Font& font,
		float animationStep, bool pixelAligned);

}