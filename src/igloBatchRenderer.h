
#pragma once

namespace ig
{

	class Font;

	struct Vertex_XYC
	{
		Vector2 position;
		Color32 color;
	};
	struct Vertex_XYCUV
	{
		Vector2 position;
		Color32 color;
		Vector2 uv;
	};
	struct Vertex_Rect
	{
		Vector2 position;
		float width;
		float height;
		Color32 color;
	};
	struct Vertex_Sprite
	{
		Vector2 position;
		uint16_t width;
		uint16_t height;
		uint16_t u;
		uint16_t v;
		Color32 color;
	};
	struct Vertex_ScaledSprite
	{
		Vector2 position;
		float width;
		float height;
		FloatRect uv;
		Color32 color;
	};
	struct Vertex_TransformedSprite
	{
		Vector2 position;
		float width;
		float height;
		FloatRect uv;
		Vector2 rotationOrigin;
		float rotation;
		Color32 color;
	};
	struct Vertex_Circle
	{
		Vector2 position;
		float radius;
		float smoothing; // For anti alias. Larger value makes it look smoother.
		float borderThickness;
		Color32 innerColor;
		Color32 outerColor;
		Color32 borderColor;
	};

	// You can enable multiple SDF effects at the same time.
	enum class SDFEffectFlags : uint32_t
	{
		NoEffects = 0, // Variables used: 'smoothing'
		Outline = 1, // Variables used: 'smoothing', 'outlineThickness', 'outlineColor'
		Glow = 2 // Variables used: 'smoothing', 'glowOffset', 'glowSize', 'glowColor'
	};

	struct SDFEffect
	{
		SDFEffect() :
			outlineColor(Colors::Black),
			glowColor(Color(0.0f, 0.0f, 0.0f, 0.5f)),
			smoothing(0.11f),
			outlineThickness(0.15f),
			glowOffset(1.0f, 1.0f),
			glowSize(0.25f),
			sdfEffectFlags((uint32_t)SDFEffectFlags::NoEffects)
		{
		}

		Color outlineColor;
		Color glowColor;
		float smoothing; // For anti alias. Larger value makes it look smoother.
		float outlineThickness;
		Vector2 glowOffset;
		float glowSize;
		uint32_t sdfEffectFlags;
	};

	typedef uint32_t BatchType;
	enum class StandardBatchType : BatchType
	{
		None = 0,
		Points_XYC,
		Lines_XYC, // Smooth lines
		Lines_XYC_Pixelated, // Pixelated lines
		Triangles_XYC,
		Triangles_XYCUV,
		Rects,
		Sprites,
		Sprites_MonoOpaque,
		Sprites_MonoTransparent,
		Sprites_SDF,
		ScaledSprites,
		ScaledSprites_MonoOpaque,
		ScaledSprites_MonoTransparent,
		ScaledSprites_SDF,
		ScaledSprites_DepthBuffer,
		ScaledSprites_PremultipliedAlpha,
		ScaledSprites_BlendDisabled,
		TransformedSprites,
		TransformedSprites_MonoOpaque,
		TransformedSprites_MonoTransparent,
		TransformedSprites_SDF,
		Circles,

		NumStandardBatchTypes // To keep count
	};

	enum class TextureDrawStyle : BatchType
	{
		Auto = 0,
		RGBA = (BatchType)StandardBatchType::ScaledSprites,
		MonochromeOpaque = (BatchType)StandardBatchType::ScaledSprites_MonoOpaque,
		MonochromeTransparent = (BatchType)StandardBatchType::ScaledSprites_MonoTransparent,
		SDF = (BatchType)StandardBatchType::ScaledSprites_SDF,
		DepthBuffer = (BatchType)StandardBatchType::ScaledSprites_DepthBuffer,
		PremultipliedAlpha = (BatchType)StandardBatchType::ScaledSprites_PremultipliedAlpha,
		BlendDisabled = (BatchType)StandardBatchType::ScaledSprites_BlendDisabled,
	};

	struct BatchDesc
	{
		Primitive primitive = Primitive::Undefined;
		uint32_t bytesPerVertex = 0; // Vertex stride
		uint32_t inputVerticesPerPrimitive = 0; // Minimum number of vertices you need to feed to BatchRenderer to draw 1 primitive.

		// Must be above 0 if a vertex generation method is used.
		uint32_t outputVerticesPerPrimitive = 0;

		// If you use Indexing, make sure the provided index buffer is large enough to draw atleast
		// a few hundred or thousand primitives, because the maximum number of primitives that can be batched
		// together in the same draw call will be capped to match the size of this index buffer.
		// BatchRenderer will not write to this index buffer, the index buffer is meant to be populated with indices beforehand.
		Buffer* indexBuffer = nullptr;

		enum class VertexGenerationMethod
		{
			None = 0, // No additional vertices are generated.
			Instancing,
			Indexing, // Requires that you provide an index buffer.
			VertexPullingStructured,
			VertexPullingRaw,
		};
		VertexGenerationMethod vertGenMethod = VertexGenerationMethod::None;
	};

	struct BatchParams
	{
		PipelineDesc pipelineDesc;
		BatchDesc batchDesc;
	};

	BatchParams GetStandardBatchParams(StandardBatchType type, RenderTargetDesc renderTargetDesc);

	// BatchRenderer helps you draw 2D stuff effortlessly such as sprites, strings, shapes, etc.
	class BatchRenderer
	{
	public:
		BatchRenderer() = default;
		~BatchRenderer() { Unload(); }

		BatchRenderer& operator=(BatchRenderer&) = delete;
		BatchRenderer(BatchRenderer&) = delete;

		// Initializes the BatchRenderer. Returns true if success.
		bool Load(IGLOContext&, CommandList&, RenderTargetDesc renderTargetDesc);
		// Destroys the BatchRenderer.
		void Unload();

		bool IsLoaded() const { return isLoaded; }

		// The BatchRenderer drawing functions should be called in between Begin() and End().
		// By default, a 2D orthogonal projection is used that matches the size of the backbuffer so that 1 unit = 1 pixel.
		// If you draw on a rendertarget, you can call SetViewAndProjection2D(width,height) to match the size
		// of your rendertarget so that 1 unit will equal 1 pixel.
		void Begin(CommandList&);
		void End();

		// Gets the number of draw calls that previously occured between Begin() and End().
		uint64_t GetDrawCallCount() const { return prevDrawCallCounter; }

		// Draws and discards all primitives that have been added to the batch so far.
		// This is mostly used internally by BatchRenderer.
		void FlushPrimitives();

		void SetViewAndProjection3D(Vector3 position, Vector3 lookToDirection, Vector3 up,
			float aspectRatio, float fovInDegrees = 90.0f, float zNear = 0.07f, float zFar = 700.0f);

		// Sets view and projection matrices to 2D orthogonal mode.
		// An object drawn at 'viewX' and 'viewY' will appear in the center of the screen.
		// 'viewWidth' and 'viewHeight' behaves like zoom. Small width and height lead to large zoom.
		// When 'viewWidth' and 'viewHeight' is the size of the current viewport then 1 unit space = 1 pixel.
		void SetViewAndProjection2D(float viewX, float viewY, float viewWidth, float viewHeight, float zNear = 0.07f, float zFar = 700.0f);

		// Sets view and projection matrices to 2D orthogonal mode with a topleft origin.
		// Drawing at x=0 and y=0 will have it appear in the topleft corner.
		void SetViewAndProjection2D(float viewWidth, float viewHeight);

		// Sets view and projection matrices to 2D orthogonal mode.
		// 'viewArea' behaves like a 2D camera. By adjusting the location and size of the rectangle you adjust the
		// camera position and zoom.
		void SetViewAndProjection2D(FloatRect viewArea);
		void SetViewAndProjection(Matrix4x4 view, Matrix4x4 projection);

		void SetWorldMatrix(Vector3 translation, Quaternion quaternionRotation, Vector3 scale);
		void SetWorldMatrix(Matrix4x4 world);

		// Resets the world matrix to its default value Matrix4x4::Identity().
		void RestoreWorldMatrix();

		Matrix4x4 GetWorldMatrix() const { return state.world; }
		Matrix4x4 GetViewMatrix() const { return state.view; }
		Matrix4x4 GetProjectionMatrix() const { return state.proj; }

		void SetSampler(const Sampler&);

		// Use anisotropic filtering for textures. This sampler state is used by default.
		void SetSamplerToSmoothTextures();

		void SetSamplerToSmoothClampedTextures();

		// Use point sampling for textures.
		void SetSamplerToPixelatedTextures();

		// Draws a string of utf8 characters.
		void DrawString(float x, float y, const std::string& utf8string, Font& font, Color32 color, size_t startIndex, size_t endIndex);
		void DrawString(Vector2 position, const std::string& utf8string, Font& font, Color32 color, size_t startIndex, size_t endIndex);
		void DrawString(float x, float y, const std::string& utf8string, Font& font, Color32 color);
		void DrawString(Vector2 position, const std::string& utf8string, Font& font, Color32 color);

		// Measures the width and height of a string that would be drawn with DrawString().
		Vector2 MeasureString(const std::string& utf8string, Font& font);
		Vector2 MeasureString(const std::string& utf8string, Font& font, size_t startIndex, size_t endIndex);

		void DrawSprite(const Texture& texture, float x, float y, IntRect uv, Color32 color = Colors::White);
		void DrawSprite(const Texture& texture, float x, float y, uint16_t width, uint16_t height, uint16_t u, uint16_t v, Color32 color = Colors::White);
		void DrawScaledSprite(const Texture& texture, float x, float y, float width, float height, FloatRect uv, Color32 color = Colors::White);
		void DrawTransformedSprite(const Texture& texture, float x, float y, float width, float height,
			FloatRect uv, Vector2 rotationOrigin, float rotation, Color32 color = Colors::White);

		void DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, Color32 color);
		void DrawTriangle(Vector2 pos0, Vector2 pos1, Vector2 pos2, Color32 color);
		void DrawTriangle(Vector2 pos0, Vector2 pos1, Vector2 pos2, Color32 color0, Color32 color1, Color32 color2);

		void DrawTexturedTriangle(const Texture& texture, Vector2 pos0, Vector2 pos1, Vector2 pos2,
			Color32 color0, Color32 color1, Color32 color2, Vector2 texCoord0, Vector2 texCoord1, Vector2 texCoord2);

		// Draws a textured quad.
		// Note that textures drawn with different texture draw styles can't be batched together in the same draw call,
		// so try to group textures together that use the same texture draw style.
		void DrawTexture(const Texture& texture, float x, float y, Color32 color = Colors::White, TextureDrawStyle style = TextureDrawStyle::Auto);
		void DrawTexture(const Texture& texture, float x, float y, float width, float height, Color32 color = Colors::White,
			TextureDrawStyle style = TextureDrawStyle::Auto);
		void DrawTexture(const Texture& texture, float x, float y, float width, float height, FloatRect UV, Color32 color,
			TextureDrawStyle style = TextureDrawStyle::Auto);

		// Draws a colored quad.
		void DrawRectangle(float x, float y, float width, float height, Color32 color);
		void DrawRectangle(FloatRect rect, Color32 color);

		// Draws a rectangle border using four quads.
		// The border is drawn entirely inside the given area, and expands inwards.
		void DrawRectangleBorder(FloatRect rect, Color32 color, float borderThickness = 1.0f);
		void DrawRectangleBorder(float x, float y, float width, float height, Color32 color, float borderThickness = 1.0f);
		// Draws a rounded rectangle using triangle primitives.
		void DrawRoundedRectangle(float x, float y, float width, float height, Color32 color, float circleRadius, uint32_t circleSides = 16);
		// Draws a rounded rectangle border using line primitives.
		void DrawRoundedRectangleBorder(float x, float y, float width, float height, Color32 color, float circleRadius, uint32_t circleSides = 16);
		// Draws a texture that stretches only the center pixels when stretched.
		void DrawCenterStretchedTexture(const Texture& texture, float x, float y, float width, float height, Color32 color = Colors::White,
			TextureDrawStyle style = TextureDrawStyle::Auto);
		// Draws a line using triangle primitives.
		void DrawRectangularLine(float x0, float y0, float x1, float y1, float thickness, Color32 color);
		// Draws a textured line using textured triangle primitives.
		void DrawTexturedRectangularLine(const Texture& texture, float x0, float y0, float x1, float y1, float thickness, Color32 color);

		// Draws a circle using a circle pixel shader.
		// If you want the circle to have a border, set 'borderThickness' to any value above 0.
		// The border is drawn precisely at the circle edges.
		// 'smoothing' determines how much anti alias to use.
		void DrawCircle(float x, float y, float radius, float borderThickness, Color32 innerColor, Color32 outerColor, Color32 borderColor, float smoothing = 1.0f);
		void DrawCircle(float x, float y, float radius, float borderThickness, Color32 fillColor, Color32 borderColor);
		void DrawCircle(float x, float y, float radius, Color32 innerColor, Color32 outerColor);
		void DrawCircle(float x, float y, float radius, Color32 color);

		// Draws a cake shape using triangle primitives.
		void DrawCake(float x, float y, float radius, uint32_t sides, float degreesStart, float degreesLength, Color32 colorCenter, Color32 colorBorder);
		// Draws a cake shape using line primitives.
		void DrawCakeBorder(float x, float y, float radius, uint32_t sides, float degreesStart, float degreesLength, Color32 colorCenter, Color32 colorBorder);

		// Draws a line primitive.
		// 'rasterization' allows you to choose how the line should be rasterized, which affects its appearance.
		// Note that lines drawn with different rasterization modes can't be batched together in the same draw call,
		// so try to group lines together that use the same rasterization mode.
		void DrawLine(float x0, float y0, float x1, float y1, Color32 color, LineRasterizationMode rasterization = LineRasterizationMode::Smooth);
		void DrawLine(float x0, float y0, float x1, float y1, Color32 color0, Color32 color1, LineRasterizationMode rasterization = LineRasterizationMode::Smooth);
		void DrawLine(Vector2 pos0, Vector2 pos1, Color32 color0, Color32 color1, LineRasterizationMode rasterization = LineRasterizationMode::Smooth);

		// Draws a point primitive.
		void DrawPoint(float x, float y, Color32 color);

		BatchType CreateBatchType(BatchParams batchParams);

		void SetDepthBufferDrawStyle(float zNear, float zFar, bool drawStencilComponent);
		void SetSDFEffect(const SDFEffect&);

		Descriptor GetSDFEffectRenderConstant() const { return tempConstantSDFEffect; }
		Descriptor GetDepthBufferDrawStyleRenderConstant() const { return tempConstantDepthBufferDrawStyle; }

		// All UsingX() functions use redundancy checks, so calling them repeatedly with the same value is OK.
		void UsingBatch(BatchType batchType);
		void UsingTexture(const Texture& texture);
		void UsingRenderConstants(const Descriptor& descriptor);

		void AddPrimitive(const void* data);
		void AddPrimitives(const void* data, uint32_t numPrimitives);

	private:

		void ResetRenderStates();

		struct PushConstants
		{
			uint32_t textureIndex = IGLO_UINT32_MAX;
			uint32_t stencilComponentTextureIndex = IGLO_UINT32_MAX; // Only for depth buffers with stencil component
			uint32_t samplerIndex = IGLO_UINT32_MAX;
			uint32_t worldMatrixIndex = IGLO_UINT32_MAX;
			uint32_t viewProjMatrixIndex = IGLO_UINT32_MAX;
			uint32_t textureConstantsIndex = IGLO_UINT32_MAX;
			uint32_t renderConstantsIndex = IGLO_UINT32_MAX;
			uint32_t rawOrStructuredBufferIndex = IGLO_UINT32_MAX;
		};

		struct TextureConstants
		{
			Vector2 textureSize;
			Vector2 inverseTextureSize;
			uint32_t msaa = 0;
		};

		struct BatchState
		{
			BatchType batchType = 0;
			BatchDesc batchDesc;
			uint32_t maxPrimitives = 0;
			uint32_t bytesPerPrimitive = 0;

			Matrix4x4 world;
			Matrix4x4 view;
			Matrix4x4 proj;

			const Sampler* sampler = nullptr;
			const Texture* texture = nullptr;

			PushConstants pushConstants;
		};

		struct BatchPipeline
		{
			std::unique_ptr<Pipeline> pipeline;
			BatchDesc batchDesc;
		};

		struct DepthBufferDrawStyle
		{
			uint32_t depthOrStencilComponent = 0; // 0 = draw depth component, 1 = draw stencil component
			float zNear = 0; // When drawing depth component
			float zFar = 0; // When drawing depth component
		};

		static constexpr uint32_t MaxBatchSizeInBytes = IGLO_MEGABYTE * 1;

		bool isLoaded = false;
		const IGLOContext* context = nullptr;
		CommandList* cmd = nullptr;
		RenderTargetDesc renderTargetDesc;

		Texture nullTexture;
		Sampler samplerSmoothTextures;
		Sampler samplerSmoothClampedTextures;
		Sampler samplerPixelatedTextures;

		Descriptor tempConstantDepthBufferDrawStyle;
		Descriptor tempConstantSDFEffect;

		std::vector<BatchPipeline> batchPipelines; // One for each batch type
		
		std::vector<byte> vertices; // Vertex data
		bool hasBegunDrawing = false;
		uint32_t nextPrimitive = 0; // Index of next primitive to write to
		uint64_t drawCallCounter = 0;
		uint64_t prevDrawCallCounter = 0;

		BatchState state;
	};




} //end of namespace ig