
# Changelog

## v0.7.4

### Changes

- Added `CommandList::CopyTextureRegion()`.
- The global constants in iglo_utility.h are now constexpr.
- Renamed `IGLO_SQR2` to `IGLO_SQRT2`.
- Replaced `IGLO_ToRadian`/`IGLO_ToDegree` macros with constexpr functions `ig::ToRadians()`/`ig::ToDegrees()`.

## v0.7.3

### Changes

- Renamed `IGLOContext::GetMaxMultiSampleCount()` to `IGLOContext::GetMaxMSAA()`.
- Improved `IGLOContext::GetMaxMSAA()` on the Vulkan backend (correct usage flags + framebuffer-limit).

## v0.7.2

### Changes

- Added debug checks to catch `WaitForIdleDevice()` misuse.

## v0.7.1

### Changes

- Added a comment clarifying that `Resize` event is emitted on idle device, so feel free to destroy resources instantly on this event.
- Added a comment clarifying that `WaitForIdleDevice()` must not be called while in the middle of recording commands that rely on temp resources.
  This is because this function frees all temp resources.

## v0.7.0

### Changes

- Major/breaking changes:
  - Split `SimpleBarrier::ClearInactiveRenderTarget` into `ClearInactiveRenderTexture` and `ClearInactiveDepthBuffer`.
  - Renamed `Image::GetMipPixels()` to `Image::GetPixelsAtSubresource()`.
  - `generateMips=false` is now the default when loading textures.
    Passing `generateMips=true` for textures that already have >= 2 mips will skip mip gen and log a warning message.
- New features:
  - Added `CommandList::BeginRenderPass_Vulkan(const VulkanRenderInfo&)`.
    This function gives you finer control over the render pass parameters.
  - Added support for copying a texture subresource to a Readable texture.
  - Added `ig::LogLimited()`.
  - iglo can now generate mips for non-pow-2 textures.
- Improvements:
  - All mip gen shaders now do bounds checking (Vulkan needs it).
  - `bool IsPowerOf2(uint64_t value)` is now constexpr.
  - Optimized glyph placement and dynamic font texture growth (now uses memcpy).
  - There is now a hard limit to how much a dynamic font texture can grow (16k x 16k).
  - iglo can now generate up to 4 mips per dispatch for pow-2 textures (optimization).
- Bug fixes:
  - Fixed validation error in `ClearDepth()` on Vulkan when attempting to clear a stencil of an inactive non-stencil depth buffer.
  - Fixed Vulkan bug where `Texture::ReadPixels()` would read the pixels incorrectly (wrong row pitch).
  - Fixed Vulkan bug where `CopyTempBufferToTexture()` and `CopyTempBufferToTextureSubresource()` would copy incorrectly on devices where `bufferPlacementAlignments.textureRowPitch` > 1.

## v0.6.0

### Changes

- Major/breaking changes:
  - `IGLOContext::MoveToNextFrame()` is now a public function that must be called once per frame. `ig::MainLoop` calls this once per frame so you don't have to.
  - Renamed `CallbackOnDeviceRemoved` to `CallbackOnDeviceLost` and removed its `const std::string& deviceRemovalReason` parameter.
  - `WindowMinimizedBehaviour` enum moved outside of `ig::MainLoop`.
- New features:
  - Added `IGLOContext::IsSwapChainValid()`.
  - Added `IGLOContext::IsDeviceLost()`.
  - Added double precision `ig::Lerp()`.
- Improvements:
  - `ig::MainLoop` now skips `Draw()` on invalid swapchain and on lost device.
  - `SimpleBarrier::Discard` now uses `BarrierSync::All`.
  - Some Vulkan barrier values have been updated.
  - Synchronization validation is now always enabled on Vulkan Debug.
  - Improved presentation and swapchain handling for both backends.
  - Improved the handling of device loss.
  - Improved mipLevel validation in `ig::Image`.
  - Improved image validation when loading prebaked font files.
  - Font loading code now checks for invalid kerning pairs.
- Bug fixes:
  - Now using correct Vulkan barriers for clearing UAV and inactive render targets.
  - Fixed small memory leak caused by forgetting to clear `delayedDestroyVulkanImageViews` vector.
  - Fixed font loading bug that always ignored codepoint 0x10FFFF.
  - Fixed bug in `ig::PackedBoolArray` where asserts could wrongly fire in SetTrue() and SetFalse() when resizing (this only affected Debug builds).
  - Fixed bug in `ig::PackedBoolArray` where it would set already set booleans when growing in elements (this only affected performance).

## v0.5.2

### Changes
- Added reusable RTV and DSV descriptor heaps in the D3D12 backend.
  These are small non-shader-visible heaps that help you create disposable RTVs and DSVs.
- The asserts in `ClearUnorderedAccessTextureFloat()` and `ClearUnorderedAccessTextureUInt32()` now consider `UnorderedAccessRenderTexture` a valid texture usage.
- ig::Shader now stores entry point name using `const char*` instead of `std::string`.
- Initializing ig::Shader from a byte vector is now a static function instead of an overloaded constructor.
- Improved the `Texture::GenerateD3D12Desc_SRV/UAV/RTV/DSV` helper functions.
- The arguments in `Texture::GenerateD3D12Desc_SRV()` have changed.

## v0.5.1

### Changes
- Added a new overload to `BatchRenderer::DrawTexture()`.
- Renamed `Texture::GenerateD3D12Desc_SRV_AllMips()` to `Texture::GenerateD3D12Desc_SRV()`.

## v0.5.0

### Changes
- Major changes:
  - Render passes are now explicit.
  - Added `BeginRenderPass()`, `BeginRenderPassMultiTarget()`, `EndRenderPass()`, `SafePauseRenderPass()`, `SafeResumeRenderPass()`.
  - `SetRenderTarget()` and `SetRenderTargets()` are now D3D12-only. Cross-backend code should use `BeginRenderPass()` and `EndRenderPass()` instead.
  - Renamed `ig::PresentMode` enum values to more accurately match the Vulkan present modes.
    - `Immediate` -> `Mailbox`
    - `ImmediateWithTearing` -> `Immediate`
- New features:
  - `ig::DescriptorHeap` now manages RTV and DSV descriptors (D3D12).
  - Added `ig::SimpleBarrier::VertexShaderResource`
  - You can now create buffer barriers using `ig::SimpleBarrier`.
  - Added `ig::TextureUsage::UnorderedAccessRenderTexture`
  - Added `ig::TextureFlags` enum. You can now specify texture flags at texture creation.
- Bug fixes:
  - Clearing unordered access buffers and textures now works properly in Vulkan.
  - Vulkan backend now correctly targets only the first mip for UAVs, RTVs and DSVs (descriptors and image views).
    Both backends now work uniformly: Default created SRVs target all mips. Default created UAVs, RTVs and DSVs only target the first mip.
- Improvements:
  - Refactored `ig::DescriptorHeap` class.
  - Refactored `ig::Texture` and `ig::Buffer` classes.
  - D3D12 now prefers high-performance GPUs when enumerating adapters.
  - The CMake script now uses the latest release version of Agility SDK (not pre-release).

## v0.4.5

### Changes
- Fixed bug in Xoshiro256pp class (NextInt32 is now replaced with NextUInt32Range)

## v0.4.4

### Changes
- iglo_config.h is now included in iglo_utility.h instead of iglo.h
- Refactored some includes in iglo.h and iglo_utility.h/cpp
- Moved DetailedResult and Log() to iglo_utility.h
- iglo_utility.cpp now calls Fatal() instead of throwing exceptions

## v0.4.3

### Changes
- Fixed compile error on Linux caused by the removal of `#include <random>` in iglo_utility.h. `<cmath>` and `<memory>` are now explicitly included on both Windows and Linux.

## v0.4.2

### Changes
- OnDeviceRemoved() callback now passes the string argument as const&.

## v0.4.1

### Changes
- Fixed ig::CreateDirectory() so it returns false on failure instead of throwing exceptions.

## v0.4.0

### Changes
- Major changes:
  - iglo resources and components are now managed with unique_ptr (stack allocation no longer supported)
- New features:
  - Added ability to query system and video memory info
  - Added custom cursor support
  - Added clipboard functionality
  - Added SimpleBarrier::ClearInactiveRenderTarget
- Bug fixes:
  - Fixed segfault caused by NVIDIA drivers on Linux/Vulkan backend that would appear when closing the window
  - Fixed bug in Vector4::GetNormalized()
  - Fixed bug in Image::CalculateNumMips()
  - Fixed potential overflow in Image::CalculateMipSize()
  - Fixed bug in IGLOContext::SetWindowSize()
  - Fixed bug in CommandList::ValidatePushConstants()
  - Fixed bug in BatchRenderer::DrawString()
  - Fixed bug in Font::AllocateRoomForGlyph()
- Minor changes:
  - iglo utility
    - Added FloatRect::GetExpanded()
    - Added FloatRect::CroppedTo()
    - Added Lerp()
    - utf8_next_codepoint() now returns replacement characters for invalid byte sequences
    - The utf16 functions now use u16string instead of wstring
    - Improved utf8_is_next_sequence_valid()
    - Improved utf8_is_valid()
    - Made some timer functions const
    - Replaced UniformRandom class with Xoshiro256pp class
    - ig::Random now uses Xoshiro256pp instead of rand()
  - iglo
    - Now using NDEBUG instead of _DEBUG
    - Added Fatal()
    - Added GetFormatName()
    - Renamed ConvertKeyToString() to GetKeyName()
    - MouseEvent::scrollWheel is now a float
    - Fixed a typo in ig::Key enum
    - DescriptorHeap::PersistentIndexAllocator now checks against double frees in Release
    - Improved Viewport struct (better defaults)
  - IGLOContext
    - Added GetMouseButtonState()
    - Added GetKeyState()
    - Made some functions const
    - TextEntered event now ignores unprintable characters
  - BatchRenderer
    - Added DrawImmediate()
    - Added GetCurrentBatchType()
    - Added GetCurrentCommandList()
    - Added IsActive()
    - Renamed DrawCenterStretchedTexture() to DrawNineSliceSprite()
    - Improved SDF pixel shader (better shadows)
  - Font
    - PrebakedFontData loading function is now more explicit (to remove compiler warnings)
    - Added out_isErrorGlyph option to Font::GetGlyph()
  - Updated Agility SDK to version 1.717.1-preview in the CMake script

## v0.3.2

### Changes
- Fixed a padding issue with the Prebaked font file header struct.

## v0.3.1

### Changes
- Added SimpleBarrier::ClearUnorderedAccess
- Moved stb includes away from iglo.h

## v0.3.0

### Changes
- Platform support:
  - Added Vulkan support
  - Added Linux support (X11)
  - The default backbuffer format is now BGRA for compatibility with X11 on Linux
- Simplified barriers:
  - You no longer need to specify whether to use a queue-specific layout with `ig::SimpleBarrier`
  - Use `ig::SimpleBarrier::Present` when presenting backbuffer
  - Use `ig::SimpleBarrier::Discard` when you don't care about preserving the previous contents
  - Removed `initialLayout` option when creating textures
- New features:
  - Added the `ScreenRenderer` class for rendering fullscreen quads
  - Added an option to enable fixed update frame-rate sync in `MainLoop`.
    This option is meant for games that run at fixed frame-rates and don't use frame interpolation.
  - Added ability to change the number of frames in flight at runtime which allows finer control over vsync input latency:
    - 2 frames in flight with 3 backbuffers: **Classic vsync**
    - 1 frame in flight with 2 backbuffers: **Lower latency vsync**  
      Note: This configuration is unstable at high workloads as CPU and GPU don't work asynchronously. Should be OK for simpler games that have low CPU and GPU workloads.
      More info can be found at: https://www.intel.com/content/www/us/en/developer/articles/code-sample/sample-application-for-direct3d-12-flip-model-swap-chains.html
  - Added the ability to retrieve an sRGB-opposite view in IGLOContext::GetBackBuffer(). This makes it easier to control when hardware linear-to-sRGB conversions (and vice versa) are applied.
- Code improvements:
  - Moved all implementation code to separate files
  - Refactored a bunch of code
  - Upgraded to C++20

## v0.2.3

### Changes
- BatchRenderer now uses Instancing to draw quads.

## v0.2.2

### Changes
- BatchRenderer now uses the Vertex Pulling method to draw quads.

## v0.2.1

### Changes
- Fixed a bug where Dynamic usage raw/structured buffers would create all descriptors for the first resource only.
- Clarified with comments that the order at which a Dynamic buffer is updated and bound to the pipeline matters due to how Dynamic buffers work internally.

## v0.2.0

### Changes
- Improved the frame-rate limiter. It now uses a more precise sleep function.
- Renamed the Physics callback to FixedUpdate. It is now set in MainLoop.Run() along with the rest of the callbacks.

## v0.1.1

### Changes
- Improved the build system. CMake now downloads AgilitySDK directly from www.nuget.org without using nuget.exe.
- AgilitySDK .pdb files are no longer copied over to the D3D12 .dll folder.

## v0.1.0

- First released version
