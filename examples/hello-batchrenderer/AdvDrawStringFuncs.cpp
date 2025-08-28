#include "../../src/iglo.h"
#include "../../src/iglo_font.h"
#include "../../src/iglo_batch_renderer.h"
#include "AdvDrawStringFuncs.h"

namespace ig
{

	void AlignStringRectangles(std::vector<FloatRect>& rects, StringAlignment align, FloatRect area, float lineGap, bool pixelAligned)
	{
		if (rects.size() == 0) return;

		float alignedLineGap = lineGap;
		float areaCenterX = ((area.right - area.left) / 2.0f) + area.left;
		float areaCenterY = ((area.bottom - area.top) / 2.0f) + area.top;
		float areaBottom = area.bottom;
		float areaTop = area.top;
		float areaLeft = area.left;
		float areaRight = area.right;
		if (pixelAligned)
		{
			alignedLineGap = roundf(alignedLineGap);
			areaCenterX = roundf(areaCenterX);
			areaCenterY = roundf(areaCenterY);
			areaBottom = roundf(areaBottom);
			areaTop = roundf(areaTop);
			areaLeft = roundf(areaLeft);
			areaRight = roundf(areaRight);
			for (size_t i = 0; i < rects.size(); i++)
			{
				rects[i].left = roundf(rects[i].left);
				rects[i].top = roundf(rects[i].top);
				rects[i].right = roundf(rects[i].right);
				rects[i].bottom = roundf(rects[i].bottom);
			}
		}
		for (size_t i = 0; i < rects.size(); i++)
		{
			switch (align)
			{
			case StringAlignment::TopLeft:
			case StringAlignment::Left:
			case StringAlignment::BottomLeft:
				rects[i].right -= (rects[i].left - areaLeft);
				rects[i].left = areaLeft;
				break;

			case StringAlignment::TopRight:
			case StringAlignment::Right:
			case StringAlignment::BottomRight:
				rects[i].left -= (rects[i].right - areaRight);
				rects[i].right = areaRight;
				break;

			case StringAlignment::Top:
			case StringAlignment::Center:
			case StringAlignment::Bottom:
				float width = rects[i].right - rects[i].left;
				float halfWidth = width / 2.0f;
				if (pixelAligned) halfWidth = roundf(halfWidth);
				rects[i].left = areaCenterX - halfWidth;
				rects[i].right = rects[i].left + width;
				break;
			}

			// Move to top Y
			if (align == StringAlignment::TopLeft ||
				align == StringAlignment::Top ||
				align == StringAlignment::TopRight)
			{
				if (i == 0)
				{
					rects[i].bottom -= (rects[i].top - areaTop);
					rects[i].top = areaTop;
				}
				else
				{
					rects[i].bottom -= (rects[i].top - rects[i - 1].bottom);
					rects[i].top = rects[i - 1].bottom;
					rects[i] += Vector2(0, alignedLineGap);
				}
			}
		}

		// Move to center Y
		if (align == StringAlignment::Right ||
			align == StringAlignment::Center ||
			align == StringAlignment::Left)
		{
			float totalHeight = 0;
			for (size_t i = 0; i < rects.size(); i++)
			{
				totalHeight += (rects[i].bottom - rects[i].top);
			}
			if (rects.size() >= 2)
			{
				totalHeight += (float(rects.size() - 1) * alignedLineGap);
			}
			float halfHeight = totalHeight / 2.0f;
			if (pixelAligned) halfHeight = roundf(halfHeight);
			for (size_t i = 0; i < rects.size(); i++)
			{
				if (i == 0)
				{
					float height = rects[i].bottom - rects[i].top;
					rects[i].top = areaCenterY - halfHeight;
					rects[i].bottom = rects[i].top + height;
				}
				else
				{
					rects[i].bottom -= (rects[i].top - rects[i - 1].bottom);
					rects[i].top = rects[i - 1].bottom;
					rects[i] += Vector2(0, alignedLineGap);
				}
			}
		}
		// Move to bottom Y
		else if (align == StringAlignment::BottomLeft ||
			align == StringAlignment::Bottom ||
			align == StringAlignment::BottomRight)
		{
			for (std::size_t i = rects.size(); i-- != 0; ) // Will iterate from rects.size()-1 to 0.
			{
				if (i == rects.size() - 1)
				{
					rects[i].top -= (rects[i].bottom - areaBottom);
					rects[i].bottom = areaBottom;
				}
				else
				{
					rects[i].top -= (rects[i].bottom - rects[i + 1].top);
					rects[i].bottom = rects[i + 1].top;
					rects[i] -= Vector2(0, alignedLineGap);
				}
			}
		}
	}

	void DrawAlignedStringInsideRect(BatchRenderer& r, FloatRect rect, const std::string& str, Font& font, Color32 color,
		StringAlignment align, bool wordWrap, bool pixelAligned)
	{
		if (!r.IsLoaded()) return;
		if (str.length() == 0) return;
		if (!font.IsLoaded()) return;
		if (rect.left >= rect.right) return;
		if (rect.top >= rect.bottom) return;

		bool encounteredSpace = false; // If true, a space character has been encountered on the current line.
		size_t spaceIndex = 0; // The index of the latest encountered space character.
		float spaceRight = 0;

		bool encounteredAnyCharacter = false; // If true, any character has been encountered on the current line.
		size_t anyCharIndex = 0; // The index of the latest encountered character.

		const FloatRect emptyLineRect = FloatRect(rect.left, rect.top, rect.left, rect.top + float(font.GetFontDesc().lineHeight));

		std::vector<size_t> newLineIndex; // The index of each new line
		newLineIndex.push_back(0); // The first line starts at index 0.

		std::vector<FloatRect> lines;
		lines.push_back(emptyLineRect);

		// Measure every line
		{
			uint32_t codepoint;
			uint32_t prevCodepoint = 0xffffffff;
			size_t i = 0;
			size_t endIndex = str.size();
			while (utf8_next_codepoint(str, i, endIndex, &i, &codepoint))
			{
				switch (codepoint)
				{
				case '\r':
					continue;

				case '\n': // Newline
					prevCodepoint = 0xffffffff; // Next character will have no kerning, since this is a new line.
					newLineIndex.push_back(i);
					lines.push_back(emptyLineRect);
					encounteredSpace = false;
					encounteredAnyCharacter = false;
					continue;

				default:
					float glyphAdvanceX = font.GetGlyph(codepoint).advanceX;
					float kerningX = font.GetKerning(prevCodepoint, codepoint);
					if (wordWrap)
					{
						// Make a new line if this character is about to reach outside the rectangle.
						if (encounteredAnyCharacter && lines[lines.size() - 1].right + glyphAdvanceX + kerningX > rect.right)
						{
							// If a space was encountered earlier...
							if (encounteredSpace)
							{
								// ...revert all characters that appeared after that space, so they can be placed on the next line instead.
								lines[lines.size() - 1].right = spaceRight; // Reverts the width of this line
								newLineIndex.push_back(spaceIndex); // New line happens where that space was encountered earlier
								lines.push_back(emptyLineRect);
								i = spaceIndex; // Jump backwards to the character that came after that space
								encounteredSpace = false;
								encounteredAnyCharacter = false; // New line will start as empty
								prevCodepoint = 0xffffffff; // Next character will have no kerning, since this is a new line.
								continue;
							}
							else
							{
								kerningX = 0; // No kerning, since this is a new line
								newLineIndex.push_back(anyCharIndex);
								lines.push_back(emptyLineRect);
								encounteredSpace = false;
								encounteredAnyCharacter = true; // New line already contains a character
							}
						}
					}
					lines[lines.size() - 1].right += (kerningX + glyphAdvanceX);
					prevCodepoint = codepoint;

					encounteredAnyCharacter = true;
					anyCharIndex = i;

					if (codepoint == ' ')
					{
						encounteredSpace = true;
						spaceIndex = i;
						spaceRight = lines[lines.size() - 1].right;
					}
					continue;
				}
			}
		}

		// Position each line
		AlignStringRectangles(lines, align, rect, (float)font.GetFontDesc().lineGap, pixelAligned);

		// Make sure font texture is up to date
		if (font.TextureIsDirty()) font.ApplyChangesToTexture(*r.GetCurrentCommandList());

		if (font.GetFontType() == FontType::SDF)
		{
			r.UsingBatch((BatchType)StandardBatchType::ScaledSprites_SDF);
			r.UsingRenderConstants(r.GetSDFEffectRenderConstant());
		}
		else
		{
			uint32_t colorChannelCount = GetFormatInfo(font.GetTexture()->GetFormat()).elementCount;
			if (colorChannelCount == 1)
			{
				r.UsingBatch((BatchType)StandardBatchType::ScaledSprites_MonoTransparent);
			}
			else
			{
				r.UsingBatch((BatchType)StandardBatchType::ScaledSprites);
			}
		}
		r.UsingTexture(*font.GetTexture());

		// Draw each line
		for (size_t i = 0; i < newLineIndex.size(); i++)
		{
			if (!lines[i].OverlapsWithRect(rect)) continue;
			float charX = lines[i].left;
			float charY = lines[i].top;
			uint32_t codepoint;
			uint32_t prevCodepoint = 0xffffffff;
			size_t j = newLineIndex[i];
			size_t endIndex = str.size();
			if (i + 1 < newLineIndex.size()) endIndex = newLineIndex[i + 1];
			while (utf8_next_codepoint(str, j, endIndex, &j, &codepoint)) // Iterate each codepoint in the text
			{
				switch (codepoint)
				{
				case '\r':
					continue;

				case '\n': // Newline
					continue;

				default:
					Glyph glyph = font.GetGlyph(codepoint);
					charX += font.GetKerning(prevCodepoint, codepoint);

					float x = charX + (float)glyph.glyphOffsetX;
					float y = charY + (float)glyph.glyphOffsetY;
					float width = (float)glyph.glyphWidth;
					float height = (float)glyph.glyphHeight;
					float u = (float)glyph.texturePosX;
					float v = (float)glyph.texturePosY;

					// Draw quad if atleast partially visible
					if (FloatRect(x, y, x + width, y + height).OverlapsWithRect(rect))
					{
						// Crop parts that are outside rectangle
						if (x < rect.left)
						{
							float numPixelsCropped = rect.left - x;
							width -= numPixelsCropped;
							x += numPixelsCropped;
							u += numPixelsCropped;
							if (width < 0) width = 0;
						}
						if (y < rect.top)
						{
							float numPixelsCropped = rect.top - y;
							height -= numPixelsCropped;
							y += numPixelsCropped;
							v += numPixelsCropped;
							if (height < 0) height = 0;
						}
						if (x + width > rect.right)
						{
							float numPixelsCropped = (x + width) - rect.right;
							width -= numPixelsCropped;
							if (width < 0) width = 0;
						}
						if (y + height > rect.bottom)
						{
							float numPixelsCropped = (y + height) - rect.bottom;
							height -= numPixelsCropped;
							if (height < 0) height = 0;
						}
						Vertex_ScaledSprite V = { Vector2(x, y), width, height, FloatRect(u, v, u + width, v + height), color };
						r.AddPrimitive(&V);
					}
					charX += float(glyph.advanceX);
					prevCodepoint = codepoint;
					continue;
				}
			}
		}
	}

	void DrawAlignedString(BatchRenderer& r, FloatRect rect, const std::string& str, Font& font, Color32 color,
		StringAlignment align, bool pixelAligned)
	{
		if (!r.IsLoaded()) return;
		if (str.length() == 0) return;
		if (!font.IsLoaded()) return;

		std::vector<size_t> newLineIndex; // The index of each new line (beginning after the '\n')
		newLineIndex.push_back(0); // The first line starts at index 0.

		// Find every newline in the string
		{
			uint32_t codepoint;
			size_t i = 0;
			while (utf8_next_codepoint(str, i, str.size(), &i, &codepoint))
			{
				if (codepoint == '\n') // Newline
				{
					newLineIndex.push_back(i);
				}
			}
		}

		// Calculate the area of each line
		std::vector<FloatRect> lines;
		lines.resize(newLineIndex.size());
		for (size_t i = 0; i < newLineIndex.size(); i++)
		{
			size_t endIndex = str.size();
			if (i + 1 < newLineIndex.size()) endIndex = newLineIndex[i + 1] - 1;
			Vector2 area = r.MeasureString(str, font, newLineIndex[i], endIndex);
			lines[i] = FloatRect(rect.left, 0, rect.left + area.x, (float)font.GetFontDesc().lineHeight);
		}

		// Position each line
		AlignStringRectangles(lines, align, rect, (float)font.GetFontDesc().lineGap, pixelAligned);

		// Draw each line
		for (size_t i = 0; i < newLineIndex.size(); i++)
		{
			size_t endIndex = str.size();
			if (i + 1 < newLineIndex.size()) endIndex = newLineIndex[i + 1] - 1;
			r.DrawString(lines[i].left, lines[i].top, str, font, color, newLineIndex[i], endIndex);
		}
	}

	void DrawColorfulAnimatedString(BatchRenderer& r, float x, float y, const std::string& str, Font& font,
		float animationStep, bool pixelAligned)
	{
		if (!r.IsLoaded()) return;
		if (!font.IsLoaded()) return;
		if (str.length() == 0) return;

		size_t step = 0;

		bool hasSelectedBatch = false;

		float charX = x;
		float charY = y;
		uint32_t codepoint;
		uint32_t prevCodepoint = 0xffffffff;
		size_t i = 0;
		while (utf8_next_codepoint(str, i, str.size(), &i, &codepoint))
		{
			switch (codepoint)
			{
			case '\r':
				continue;

			case '\n': // Newline
				prevCodepoint = 0xffffffff;
				charY += float(font.GetFontDesc().lineHeight + font.GetFontDesc().lineGap);
				charX = x;
				continue;

			default:
				Glyph glyph = font.GetGlyph(codepoint);
				charX += font.GetKerning(prevCodepoint, codepoint);

				if (font.TextureIsDirty())
				{
					font.PreloadGlyphs(str);
					font.ApplyChangesToTexture(*r.GetCurrentCommandList());
					r.UsingTexture(*font.GetTexture());
				}

				if (!hasSelectedBatch)
				{
					hasSelectedBatch = true;
					if (font.GetFontType() == FontType::SDF)
					{
						r.UsingBatch((BatchType)StandardBatchType::Sprites_SDF);
						r.UsingRenderConstants(r.GetSDFEffectRenderConstant());
					}
					else
					{
						uint32_t colorChannelCount = GetFormatInfo(font.GetTexture()->GetFormat()).elementCount;
						if (colorChannelCount == 1)
						{
							r.UsingBatch((BatchType)StandardBatchType::Sprites_MonoTransparent);
						}
						else
						{
							r.UsingBatch((BatchType)StandardBatchType::Sprites);
						}
					}
					r.UsingTexture(*font.GetTexture());
				}

				step++;
				float anim = (float(step) * 0.5f) + (animationStep * 5.0f);
				float x = sin(anim) * 2.0f;
				float y = cos(anim) * 2.0f;
				Color32 color = Color(
					cos(anim - ((float(IGLO_PI) / 3.0f) * 1.0f)) + 0.5f,
					cos(anim - ((float(IGLO_PI) / 3.0f) * 2.0f)) + 0.5f,
					sin(anim - ((float(IGLO_PI) / 3.0f) * 3.0f)) + 0.5f).ToColor32();


				x += charX + float(glyph.glyphOffsetX);
				y += charY + float(glyph.glyphOffsetY);
				if (pixelAligned)
				{
					x = float(int(x));
					y = float(int(y));
				}

				Vertex_Sprite V = { Vector2(x, y), glyph.glyphWidth, glyph.glyphHeight, glyph.texturePosX, glyph.texturePosY, color };
				r.AddPrimitive(&V);

				charX += float(glyph.advanceX);
				prevCodepoint = codepoint;
				continue;
			}
		}
	}

}