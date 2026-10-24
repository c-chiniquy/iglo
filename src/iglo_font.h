
#pragma once

#ifdef IGLO_FREETYPE
typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_FaceRec_* FT_Face;
#else
#include "stb/stb_truetype.h"
#endif

#include <unordered_map>

namespace ig
{

	enum class FontType
	{
		Bitmap = 0, // Can be monochrome or RGBA.
		SDF, // Signed distance fields. Monochrome.
	};

	enum class FontHinting
	{
		None = 0, // No hinting
		Light, // Y hinting, using FreeType's auto-hinter
		Normal, // XY hinting, using FreeType's auto-hinter
		Native_v35, // XY hinting, using the font's own hinting instructions (like Windows GDI)
		Native_v40, // Y hinting, using the font's own hinting instructions (like ClearType)
	};
	const char* GetFontHintingName(FontHinting);

	enum class FontWeight : uint16_t
	{
		Thin = 100,
		ExtraLight = 200,
		Light = 300,
		Regular = 400,
		Medium = 500,
		SemiBold = 600,
		Bold = 700,
		ExtraBold = 800,
		Black = 900,
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
		FontType fontType = FontType::Bitmap;

		// For SDF fonts.
		// This is the number of pixels the SDF gradient will stretch from the glyph edge.
		// This determines the range of the outline and glow effects.
		// A larger spread increases the size of rasterized glyphs, which leads to a larger glyph atlas texture size.
		uint16_t sdfSpread = 3;

		// Increases the space between glyphs on the glyph atlas texture to prevent scaling artifacts.
		// This is the number of pixels each glyph quad will expand at every side (top, left, right, bottom).
		// For example, a padding of 1 will result in 2 pixels of empty space between every glyph on the texture.
		uint16_t glyphPadding = 1;

		// The starting width and height of the glyph atlas texture.
		uint32_t startingTextureSize = 128;

		// For Bitmap fonts.
		// How glyphs are snapped to pixels.
		// Any hinting other than 'None' requires FreeType (IGLO_FREETYPE).
		FontHinting hinting = FontHinting::None;

		// For Bitmap fonts.
		// If false, glyphs are rasterized in black and white (no smooth edges).
		// Disabling antialiasing requires FreeType (IGLO_FREETYPE).
		bool antialiasing = true;
	};

	struct FontLocation
	{
		std::string filename;
		uint32_t faceIndex = 0; // Which font to load in font collections (.TTC/.OTC)
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
		std::unique_ptr<Image> image;

		bool SaveToFile(const std::string& filename);
		bool LoadFromFile(const std::string& filename);
		bool LoadFromMemory(const byte* fileData, size_t numBytes);
	};

	// The font rasterizer being used. Example: "FreeType 2.15.0" or "stb_truetype".
	std::string GetFontRasterizerName();

	// An embedded Vegur15 font
	PrebakedFontData GetDefaultFont();

	// Finds an installed font by family name, weight and style.
	// Returns std::nullopt if no matching font is installed.
	std::optional<FontLocation> FindSystemFont(const std::string& familyName, FontWeight weight = FontWeight::Regular, bool italic = false);

	class KerningTable
	{
	public:
		void Build(const std::vector<Kerning>& kernList);
		void Clear();

		// Returns 0 if this pair has no kerning
		int16_t Find(uint32_t codepointPrev, uint32_t codepointNext) const;

		// The number of kerning pairs stored in the table
		uint32_t GetCount() const { return count; }

	private:
		size_t GetSlot(uint64_t key) const { return (size_t)((key * 0x9E3779B97F4A7C15ull) >> shift); }
		static uint64_t MakeKey(uint32_t codepointPrev, uint32_t codepointNext) { return ((uint64_t)codepointPrev << 32) | codepointNext; }

	private:
		static constexpr uint64_t emptyKey = IGLO_UINT64_MAX; // Never a valid key, since codepoints are at most 0x10FFFF.

		std::vector<uint64_t> keys; // Size is a power of two. Empty slots contain 'emptyKey'.
		std::vector<int16_t> values;
		uint32_t count = 0;
		uint32_t shift = 63; // 64 - log2(table size)
	};

	class Font
	{
	private:
		Font(const IGLOContext& context, const FontSettings& settings, bool isPrebaked)
			: context(context), fontSettings(settings), isPrebaked(isPrebaked) {}

		Font& operator=(const Font&) = delete;
		Font(const Font&) = delete;

	public:
		~Font();

		// Load .TTF/.OTF/.TTC/.OTC font from file.
		static std::unique_ptr<Font> LoadFromFile(const IGLOContext&, const std::string& filename,
			float fontSize, FontSettings fontSettings = FontSettings());
		static std::unique_ptr<Font> LoadFromFile(const IGLOContext&, const FontLocation& location,
			float fontSize, FontSettings fontSettings = FontSettings());
		
		// Load .TTF/.OTF/.TTC/.OTC font from memory.
		// Font will rely on the file data buffer during its lifetime,
		// so don't delete the data buffer before destroying this font.
		// 'faceIndex' decides which font to load in font collections (.TTC/.OTC)
		static std::unique_ptr<Font> LoadFromMemory(const IGLOContext&, const byte* fileData, size_t numBytes,
			std::string fontName, float fontSize, FontSettings fontSettings = FontSettings(), uint32_t faceIndex = 0);

		// Create prebaked font. A prebaked font has pre-rasterized glyphs.
		// You can programmatically fill in the values of PrebakedFontData, or you can load one from file.
		static std::unique_ptr<Font> CreatePrebaked(const IGLOContext&, CommandList&, const PrebakedFontData&);

		// Load prebaked font from file.
		// Such a font file can be created with PrebakedFontData::SaveToFile().
		static std::unique_ptr<Font> LoadPrebakedFromFile(const IGLOContext&, CommandList&, const std::string& filename);

		// Load prebaked font from memory.
		static std::unique_ptr<Font> LoadPrebakedFromMemory(const IGLOContext&, CommandList&, const byte* fileData, size_t numBytes);

		// Gets a glyph that corresponds to the specified UTF-32 codepoint.
		// If the glyph hasn't been loaded yet, it is rasterized and placed on the glyph atlas texture.
		// New glyphs placed on the atlas texture are not immediately visible, calling ApplyChangesToTexture() makes the new glyphs visible.
		// If this font doesn't contain the requested glyph, an error glyph (usually a square) is returned.
		Glyph GetGlyph(uint32_t codepoint, bool* out_isErrorGlyph = nullptr);

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

		const FontDesc& GetFontDesc() const { return fontDesc; }

		// The number of kerning pairs this font contains
		uint32_t GetKernCount() const { return atlas.kerning.GetCount(); }

		// Commits any changes made to the texture to the GPU
		void ApplyChangesToTexture(CommandList&);

		// When new glyphs have been loaded and placed on the glyph atlas texture, the texture is considered 'dirty'
		// until the new texture data is uploaded to the GPU.
		// If this returns true, you need to call ApplyChangesToTexture() before using the glyph atlas texture.
		bool TextureIsDirty() const { return page.dirty; }

		// The glyph atlas texture for this font
		const Texture* GetTexture() const { return page.texture.get(); }

		// The total number of glyphs this font contains
		uint32_t GetTotalGlyphCount() const { return atlas.glyphCount; }

		// The number of glyphs that has been loaded and placed on the texture so far
		uint32_t GetLoadedGlyphCount() const { return atlas.loadedGlyphCount; }

		// The highest codepoint used by any glyph in this font
		uint32_t GetHighestGlyphCodepoint() const { return atlas.highestGlyphCodepoint; }

		// The highest codepoint used by left side of any kerning pair in this font
		uint32_t GetHighestLeftKernCodepoint() const { return atlas.highestLeftKernCodepoint; }

		// Whether this font contains a glyph for the given codepoint
		bool HasGlyph(uint32_t codepoint) const;

		// If the font is prebaked then all glyphs are already rasterized and no new glyphs can be placed on the atlas texture.
		bool IsPrebaked() const { return isPrebaked; }

		FontType GetFontType() const { return fontSettings.fontType; }

	private:

		void SetKerning(const std::vector<Kerning>& kernList);

		// Sets the glyphs for a dynamic font.
		void SetDynamicGlyphs(const std::vector<uint32_t>& codepoints, const PackedBoolArray& exists, uint32_t totalNumValidCodepoints);

		// Sets the glyphs for a prebaked font.
		void SetPrebakedGlyphs(const std::vector<CodepointGlyph>& codepointGlyphs, Glyph errorGlyph);

		// Rasterizes and places a glyph on the glyph atlas texture, then returns it.
		// This should only be called for glyphs that hasn't already been loaded (or it will rasterize and place the same glyph multiple times).
		Glyph LoadGlyph(uint32_t codepoint);

		// Places a rasterized glyph on the glyph atlas texture, then returns it.
		// offsetX is relative to the left edge of the character cell, offsetY is relative to the baseline.
		Glyph BuildGlyph(const byte* bitmap, uint16_t width, uint16_t height, int16_t offsetX, int16_t offsetY, int16_t advanceX);

		// Allocates an empty rectangle area in the glyph atlas texture so a glyph can be placed there.
		// Returns std::nullopt if no room could be allocated.
		std::optional<IntPoint> AllocateRoomForGlyph(uint16_t width, uint16_t height);

		// Places a glyph bitmap on the glyph atlas texture.
		void PlaceGlyph(const byte* bitmap, uint16_t width, uint16_t height, uint16_t texPosX, uint16_t texPosY);

		uint32_t GetGlyphIndex(uint32_t codepoint) const;

	private:
		const IGLOContext& context;
		const FontSettings fontSettings;
		const bool isPrebaked = true; // If true, new glyphs can't be loaded and placed on the glyph atlas texture.

		FontDesc fontDesc;

		const byte* fileDataReadOnly = nullptr; // A read only pointer to font file data. (dont delete this)
		std::vector<byte> fileDataOwned; // Only used if this font owns the font file data.

#ifdef IGLO_FREETYPE
		// Each font has its own FreeType library instance, because the SDF spread is a library-wide setting,
		// and because a FreeType library instance must not be used by multiple threads at once.
		FT_Library ftLibrary = nullptr;
		FT_Face ftFace = nullptr;
#else
		float stbttScale = 0;
		stbtt_fontinfo stbttFontInfo = stbtt_fontinfo(); // font identity
#endif

		struct Atlas
		{
			uint32_t highestLeftKernCodepoint = 0; // Any codepoint higher than this doesn't belong to the left side of any kerning pair.
			uint32_t highestGlyphCodepoint = 0; // Any codepoint higher than this does not belong to any glyph.
			uint32_t glyphCount = 0; // The total number of glyphs/valid codepoints/characters that exist.

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
			KerningTable kerning;
			Glyph errorGlyph;
		};
		Atlas atlas;

		struct Page
		{
			std::unique_ptr<Texture> texture; // Glyph atlas texture.
			std::vector<IntRect> rects; // Rectangle areas of which pixels are occupied.
			std::vector<byte> pixels; // A pixel buffer containing the pixel values of the texture. Monochrome. Not used if font is prebaked.
			uint32_t width = 0; // The width of the pixel buffer.
			uint32_t height = 0; // The height of the pixel buffer.
			bool dirty = false; // If true, the texture contents and the pixel buffer contents do not match.
		};
		Page page;

	};

} //end of namespace ig