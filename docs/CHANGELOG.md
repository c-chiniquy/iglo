
# Changelog

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
- Minor improvements/changes:
  - iglo utility
    - Added FloatRect::GetExpanded()
    - Added FloatRect::CroppedTo()
    - Added Lerp()
    - utf8_next_codepoint() now returns replacement characters for invalid byte sequences
    - The utf16 functions now use u16string instead of wstring
    - Made some timer functions const
  - iglo
    - Added GetFormatName()
    - MouseEvent::scrollWheel is now a float
    - Now compatible with Agility SDK 1.717.1-preview
    - Renamed ConvertKeyToString() to GetKeyName()
    - Fixed a typo in ig::Key
    - Fixed bug in Image::CalculateNumMips()
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
