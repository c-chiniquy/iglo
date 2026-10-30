
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
		FontType type = FontType::Bitmap;

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

		uint16_t width = 0; // The width of the glyph bitmap image
		uint16_t height = 0; // The height of the glyph bitmap image

		int16_t offsetX = 0; // Glyph image pos relative to topleft corner of character cell.
		int16_t offsetY = 0;

		int16_t advanceX = 0; // The amount of pixels the character advances forward. AKA cell width.
		uint16_t glyphIndex = 0; // Index of this glyph in the font. 0 = error glyph.
	};

	struct Kerning
	{
		uint16_t glyphIndexPrev = 0; // Glyph index of the left glyph
		uint16_t glyphIndexNext = 0; // Glyph index of the right glyph
		int16_t x = 0; // The kerning
	};

	struct CodepointGlyph
	{
		uint32_t codepoint = 0;
		Glyph glyph;
	};

	struct CodepointKerning
	{
		uint32_t codepointPrev = 0; // UTF-32 codepoint of the left character
		uint32_t codepointNext = 0; // UTF-32 codepoint of the right character
		int16_t x = 0; // The kerning
	};

	// All data needed to create a prebaked font.
	// A prebaked font has a prefilled glyph atlas texture, where no new glyphs can be rasterized at runtime.
	struct PrebakedFontData
	{
		FontType type = FontType::Bitmap;
		FontDesc desc;
		std::vector<CodepointGlyph> glyphs; // 'Glyph::glyphIndex' is ignored
		std::vector<CodepointKerning> kerns;
		Glyph errorGlyph; // 'Glyph::glyphIndex' is ignored
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

		// Pairs with no kerning (x == 0) and pairs with glyph index 0 are ignored.
		void Build(const std::vector<Kerning>& kernList);
		void Clear();

		// Returns 0 if this pair has no kerning
		int16_t Find(uint16_t glyphIndexPrev, uint16_t glyphIndexNext) const;

		// The number of kerning pairs stored in the table
		uint32_t GetCount() const { return count; }

		// Glyph index 0 is the error glyph, which also means "no glyph" (such as no previous glyph), so it never has kerning.
		static bool IsUsableKerning(const Kerning& k) { return k.x != 0 && k.glyphIndexPrev != 0 && k.glyphIndexNext != 0; }

	private:
		size_t GetSlot(uint32_t key) const { return (size_t)((key * 0x9E3779B9u) >> shift); }
		static uint32_t MakeKey(uint16_t glyphIndexPrev, uint16_t glyphIndexNext) { return ((uint32_t)glyphIndexPrev << 16) | glyphIndexNext; }

	private:
		static constexpr uint32_t emptyKey = 0; // Never a valid key, since pairs with glyph index 0 are never stored

		std::vector<uint32_t> keys; // Size is a power of two. Empty slots contain 'emptyKey'.
		std::vector<int16_t> values;
		uint32_t count = 0;
		uint32_t shift = 31; // 32 - log2(table size)
	};

	class Font
	{
	private:
		Font(const IGLOContext& context, const FontSettings& settings, bool isPrebaked)
			: context(context), settings(settings), isPrebaked(isPrebaked) {}

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

		// Gets the glyph that belongs to the given codepoint.
		// If the glyph hasn't been loaded yet, it is rasterized and placed on the glyph atlas texture.
		// New glyphs placed on the atlas texture are not immediately visible, calling ApplyChangesToTexture() makes the new glyphs visible.
		// If the requested glyph does not exist, the error glyph is returned which has a glyph index of 0.
		Glyph GetGlyph(uint32_t codepoint);

		// Gets the extra X advance between two glyphs.
		// Glyph index 0 never has kerning, so pass 0 as 'prevGlyphIndex' when there is no previous glyph.
		int16_t GetKerning(uint16_t prevGlyphIndex, uint16_t nextGlyphIndex) const;

		// Whether this font contains a glyph for the given codepoint.
		bool HasGlyph(uint32_t codepoint) const;

		// Preloads all glyphs contained in the specified utf8 string.
		// If this font is prebaked then this function does nothing.
		void PreloadGlyphs(const std::string& str);
		void PreloadGlyphs(const std::string& str, size_t startIndex, size_t endIndex);

		// Preloads all glyphs between 'first' codepoint and 'last' codepoint (including first and last).
		// If this font is prebaked then this function does nothing.
		void PreloadGlyphs(uint32_t first, uint32_t last);

		// Commits any changes made to the texture to the GPU
		void ApplyChangesToTexture(CommandList&);

		// Clears the glyph atlas texture and erases all loaded glyphs.
		// Frees up RAM at the expense of slowing down future glyph rendering for a while as it needs to re render all new glyphs.
		// If this font is prebaked, then this function does nothing.
		void ClearTexture();

		// When new glyphs have been loaded and placed on the glyph atlas texture, the texture is considered 'dirty'
		// until the new texture data is uploaded to the GPU.
		// If this returns true, you need to call ApplyChangesToTexture() before using the glyph atlas texture.
		bool TextureIsDirty() const { return page.dirty; }

		// The glyph atlas texture for this font.
		const Texture* GetTexture() const { return page.texture.get(); }

		// The number of glyphs this font contains, including the error glyph.
		uint32_t GetGlyphCount() const { return atlas.glyphCount; }

		// The number of glyphs that have been rasterized and placed on the texture so far, including the error glyph.
		// Can exceed the number of glyphs the font contains, if the font has multiple codepoints belonging to the same glyph.
		uint32_t GetRasterizedGlyphCount() const { return atlas.rasterizedGlyphCount; }

		// The number of kerning pairs this font contains.
		uint32_t GetKernCount() const { return atlas.kerning.GetCount(); }

		// If the font is prebaked then all glyphs are already rasterized and no new glyphs can be placed on the atlas texture.
		bool IsPrebaked() const { return isPrebaked; }

		const FontDesc& GetDesc() const { return desc; }
		FontType GetType() const { return settings.type; }

	private:
		Glyph GetGlyphSlowPath(uint32_t codepoint);

		// Returns 0 if the font doesn't have this codepoint.
		uint16_t QueryFontGlyphIndex(uint32_t codepoint) const;

		void InitDynamicGlyphs(uint32_t numGlyphs);
		void InitPrebaked(const std::vector<CodepointGlyph>& glyphs, const std::vector<CodepointKerning>& kerns, Glyph errorGlyph);
		void InitKerning(const std::vector<Kerning>&);

		// Rasterizes a glyph and adds it to the atlas.
		Glyph RasterizeGlyph(uint16_t glyphIndex);

		// offsetX is relative to the left edge of the character cell, offsetY is relative to the baseline.
		Glyph AddGlyphToAtlas(const byte* bitmap, uint16_t width, uint16_t height, int16_t offsetX, int16_t offsetY,
			int16_t advanceX, uint16_t glyphIndex);

		// Allocates an empty rectangle area in the glyph atlas texture so a glyph can be placed there.
		// Returns std::nullopt if no room could be allocated.
		std::optional<IntPoint> AllocateAtlasSpace(uint16_t width, uint16_t height);

		void CopyBitmapToAtlas(const byte* bitmap, uint16_t width, uint16_t height, uint16_t texPosX, uint16_t texPosY);

	private:
		const IGLOContext& context;
		const FontSettings settings;
		const bool isPrebaked = true; // If true, new glyphs can't be loaded and placed on the glyph atlas texture.

		FontDesc desc;

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
			uint32_t glyphCount = 0; // Number of glyph indices in the font (includes error glyph)
			uint32_t rasterizedGlyphCount = 0; // Number of glyphs placed on the atlas, including the error glyph and duplicates

			// Lower codepoint glyphs are stored here.
			// glyphIndex == 0 means not loaded yet, or not in the font (missing glyphs are not stored).
			std::array<Glyph, 592> codepointTable = {};

			// Higher codepoint glyphs are stored here.
			// Only codepoints that exist in the font are stored.
			std::unordered_map<uint32_t, Glyph> codepointMap;

			Glyph errorGlyph;

			KerningTable kerning;
			PackedBoolArray hasLeftKern; // Size = highest left-kern glyph index + 1
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