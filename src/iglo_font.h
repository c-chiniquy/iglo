
#pragma once

#include "stb/stb_truetype.h"
#include <unordered_map>

namespace ig
{

	enum class FontType
	{
		Bitmap = 0, // Can be monochrome or RGBA.
		SDF, // Signed distance fields. Monochrome.
	};

	struct FontDesc
	{
		std::string fontName = "Unnamed font";
		float fontSize = 0; // Font size in pixels (NOTE: not the same as line height)
		int32_t baseline = 0; // The Y-coordinate of the "ground" that glyphs stand on.
		int32_t lineHeight = 0; // The height of each line of text in pixels.
		int32_t lineGap = 0; // Extra space between lines
	};

	struct FontSettings
	{
		FontSettings(
			FontType fontType = FontType::Bitmap,
			uint16_t sdfOutwardGradientSize = 3,
			uint16_t glyphPadding = 1,
			uint32_t startingTextureSize = 128)
		{
			this->fontType = fontType;
			this->sdfOutwardGradientSize = sdfOutwardGradientSize;
			this->glyphPadding = glyphPadding;
			this->startingTextureSize = startingTextureSize;
		}

		FontType fontType;

		// Only relevant if font is of type SDF.
		// This is the number of pixels the SDF gradient will stretch outwards from the glyph.
		// This determines the range of the outline and glow effects.
		// A larger gradient increases the size of rasterized glyphs, which leads to a larger glyph atlas texture size.
		uint16_t sdfOutwardGradientSize;

		// Increases the space between glyphs on the glyph atlas texture to prevent scaling artifacts.
		// This is the number of pixels each glyph quad will expand at every side (top, left, right, bottom).
		// For example, a padding of 1 will result in 2 pixels of empty space between every glyph on the texture.
		uint16_t glyphPadding;

		// The starting width and height of the glyph atlas texture.
		uint32_t startingTextureSize;
	};

	struct Glyph
	{
		uint16_t texturePosX = 0;
		uint16_t texturePosY = 0;

		uint16_t glyphWidth = 0; // The width of the glyph bitmap image
		uint16_t glyphHeight = 0; // The height of the glyph bitmap image

		int16_t glyphOffsetX = 0; // Glyph image pos relative to topleft corner of character cell.
		int16_t glyphOffsetY = 0;

		int16_t advanceX = 0; // The amount of pixels the character advances forward. AKA cell width.
		uint16_t flags = 0; // Used internally by ig::Font.
	};

	struct Kerning
	{
		uint32_t codepointPrev = 0; // UTF-32 codepoint of the left character
		uint32_t codepointNext = 0; // UTF-32 codepoint of the right character
		int16_t x = 0;  // The kerning
	};

	struct CodepointGlyph
	{
		uint32_t codepoint = 0;
		Glyph glyph;
	};

	// All data needed to create a prebaked font.
	// A prebaked font has a prefilled glyph atlas texture, where no new glyphs can be rasterized at runtime.
	struct PrebakedFontData
	{
		FontType fontType = FontType::Bitmap;
		FontDesc fontDesc;
		std::vector<CodepointGlyph> glyphs;
		std::vector<Kerning> kerns;
		Glyph errorGlyph;
		Image image;

		bool SaveToFile(const std::string& filename);
		bool LoadFromFile(const std::string& filename);
		bool LoadFromMemory(const byte* fileData, size_t numBytes);
	};

	// An embedded Vegur15 font.
	PrebakedFontData GetDefaultFont();

	class Font
	{
	public:

		Font() = default;
		~Font() { Unload(); }

		Font& operator=(Font&) = delete;
		Font(Font&) = delete;

		void Unload();
		bool IsLoaded() const { return isLoaded; }

		// Loads the font from a file. Returns true if success.
		// Files that can be loaded are .ttf and .otf files. 
		bool LoadFromFile(const IGLOContext&, const std::string& filename, float fontSize, FontSettings fontSettings = FontSettings());

		// Loads the font from a file in memory.
		// This font will rely on the file data buffer during its lifetime, so don't delete the data buffer before unloading this font.
		bool LoadFromMemory(const IGLOContext&, const byte* data, size_t numBytes, std::string fontName, float fontSize,
			FontSettings fontSettings = FontSettings());

		// Loads the font as prebaked, which means that all the glyphs has already been rasterized at one font size.
		// You can programmatically fill in the values of PrebakedFontData, or you can load one from file.
		bool LoadAsPrebaked(const IGLOContext&, CommandList&, const PrebakedFontData&);

		// Loads the font from an iglo prebaked font file.
		// Such a font file can be created with PrebakedFontData::SaveToFile().
		bool LoadAsPrebakedFromFile(const IGLOContext&, CommandList&, const std::string& filename);

		// Loads the font from an iglo prebaked font file located in memory.
		bool LoadAsPrebakedFromMemory(const IGLOContext&, CommandList&, const byte* fileData, size_t numBytes);

		// Gets a glyph that corresponds to the specified UTF-32 codepoint.
		// If the glyph hasn't been loaded yet, it is rasterized and placed on the glyph atlas texture.
		// New glyphs placed on the atlas texture are not immediately visible, calling ApplyChangesToTexture() makes the new glyphs visible.
		Glyph GetGlyph(uint32_t codepoint);

		// Gets the extra X advance per character combination.
		int16_t GetKerning(uint32_t prev, uint32_t next) const;

		// Preloads all glyphs contained in the specified string.
		// If this font is prebaked then this function does nothing.
		void PreloadGlyphs(const std::string& utf8string);

		// Preloads all glyphs between 'first' codepoint and 'last' codepoint (including first and last).
		// If this font is prebaked then this function does nothing.
		void PreloadGlyphs(uint32_t first, uint32_t last);

		// Clears the glyph atlas texture and erases all loaded glyphs.
		// Frees up RAM at the expense of slowing down future glyph rendering for a while as it needs to re render all new glyphs.
		// If this font is prebaked, then this function does nothing.
		void ClearTexture();

		// Gets a description about this font.
		FontDesc GetFontDesc() const { return fontDesc; }

		// Gets the number of kerning pairs this font contains.
		uint32_t GetKernCount() const { return atlas.kernCount; }

		// Commits any changes made to the texture to the GPU.
		void ApplyChangesToTexture(CommandList&);

		// When new glyphs have been loaded and placed on the glyph atlas texture, the texture is considered 'dirty'
		// until the new texture data is uploaded to the GPU.
		// If this returns true, you need to call ApplyChangesToTexture() before using the glyph atlas texture.
		bool TextureIsDirty() const { return page.dirty; }

		// Gets the glyph atlas texture for this font.
		const Texture* GetTexture() const { return page.texture.get(); }

		// Gets the total number of glyphs this font contains. 
		uint32_t GetTotalGlyphCount() const { return atlas.glyphCount; }

		// Gets the number of glyphs that has been loaded and placed on the texture so far.
		uint32_t GetLoadedGlyphCount() const { return atlas.loadedGlyphCount; }

		// Gets the highest codepoint used by any glyph in this font.
		uint32_t GetHighestGlyphCodepoint() const { return atlas.highestGlyphCodepoint; }

		// Gets the highest codepoint used by left side of any kerning pair in this font.
		uint32_t GetHighestLeftKernCodepoint() const { return atlas.highestLeftKernCodepoint; }

		// If this font contains a glyph for the given codepoint.
		bool HasGlyph(uint32_t codepoint) const;

		// If the font is prebaked then all glyphs are already rasterized and no new glyphs can be placed on the atlas texture.
		bool IsPrebaked() const { return isPrebaked; }

		FontType GetFontType() const { return fontSettings.fontType; }

	private:

		// Places a glyph bitmap on the glyph atlas texture.
		void PlaceGlyph(const byte* bitmap, uint16_t texPosX, uint16_t texPosY, uint16_t bitmapWidth, uint16_t bitmapHeight);

		void SetKerning(const std::vector<Kerning>& kernList);

		// Sets the glyphs for a dynamic font.
		void SetDynamicGlyphs(const std::vector<uint32_t>& codepoints, const PackedBoolArray& exists, uint32_t totalNumValidCodepoints);

		// Sets the glyphs for a prebaked font.
		void SetPrebakedGlyphs(const std::vector<CodepointGlyph>& codepointGlyphs, Glyph errorGlyph);

		// Rasterizes and places a glyph on the glyph atlas texture, then returns it.
		// This should only be called for glyphs that hasn't already been loaded (or it will rasterize and place the same glyph multiple times).
		Glyph LoadGlyph(uint32_t codepoint);

		// Allocates an empty rectangle area in the glyph atlas texture so a glyph can be placed there.
		// Returns IntRect(0, 0, 0, 0) if no more room could be allocated.
		IntRect AllocateRoomForGlyph(uint16_t glyphWidth, uint16_t glyphHeight);

		uint64_t GetKernKey(uint32_t codepointPrev, uint32_t codepointNext) const;

		bool isLoaded = false; // If this font is loaded, it's ready to be used.
		const IGLOContext* context = nullptr;

		bool isPrebaked = true; // If true, new glyphs can't be loaded and placed on the glyph atlas texture.
		const byte* fileDataReadOnly = nullptr; // A read only pointer to font file data. (dont delete this)
		std::vector<byte> fileDataOwned; // Only used if this font owns the font file data.

		FontDesc fontDesc;
		FontSettings fontSettings;

		float stbttScale = 0;
		stbtt_fontinfo stbttFontInfo = stbtt_fontinfo(); // font identity

		struct Atlas
		{
			uint32_t highestLeftKernCodepoint = 0; // Any codepoint higher than this doesn't belong to the left side of any kerning pair.
			uint32_t highestGlyphCodepoint = 0; // Any codepoint higher than this does not belong to any glyph.
			uint32_t glyphCount = 0; // The total number of glyphs/valid codepoints/characters that exist.
			uint32_t kernCount = 0; // Total number of kerning pairs that exist

			// Number of glyphs that has been loaded (displayed on the texture).
			uint32_t loadedGlyphCount = 0;

			// These glyph flags are used by the glyphs stored in the glyph table (for lower codepoints only).
			// This improves the performance for GetGlyph() and GetKerning() for all the lower codepoints.
			enum class GlyphFlags : uint16_t
			{
				IsLoaded = 1, // If this glyph is loaded yet
				HasLeftKern = 2, // If this glyph belongs to the left side of any kerning pairs
			};
			static constexpr uint32_t glyphTableSize = 592;
			std::vector<Glyph> glyphTable; // Lower codepoints are stored here.
			std::unordered_map<uint32_t, Glyph> glyphMap; // Higher codepoints are stored here.
			std::unordered_map<uint64_t, int16_t> kernMap;
			Glyph errorGlyph;

		};
		Atlas atlas;

		struct Page
		{
			std::shared_ptr<Texture> texture; // Glyph atlas texture.
			std::vector<IntRect> rects; // Rectangle areas of which pixels are occupied.
			std::vector<byte> pixels; // A pixel buffer containing the pixel values of the texture. Monochrome. Not used if font is prebaked.
			uint32_t width = 0; // The width of the pixel buffer.
			uint32_t height = 0; // The height of the pixel buffer.
			bool dirty = false; // If true, the texture contents and the pixel buffer contents do not match.
		};
		Page page;


	};


} //end of namespace ig