
# Changelog

## v0.3.0

### Added or changed
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
      Note: This configuration is unstable at high workloads as CPU and GPU don't work asynchronously. Should be OK for simple 2D games with low CPU and GPU workloads.
- Code improvements:
  - Moved all implementation code to separate files
  - Refactored a bunch of code
  - Upgraded to C++20

## v0.2.3

### Added or changed
- BatchRenderer now uses Instancing to draw quads.

## v0.2.2

### Added or changed
- BatchRenderer now uses the Vertex Pulling method to draw quads.

## v0.2.1

### Added or changed
- Fixed a bug where Dynamic usage raw/structured buffers would create all descriptors for the first resource only.
- Clarified with comments that the order at which a Dynamic buffer is updated and bound to the pipeline matters due to how Dynamic buffers work internally.

## v0.2.0

### Added or changed
- Improved the frame-rate limiter. It now uses a more precise sleep function.
- Renamed the Physics callback to FixedUpdate. It is now set in MainLoop.Run() along with the rest of the callbacks.

## v0.1.1

### Added or changed
- Improved the build system. CMake now downloads AgilitySDK directly from www.nuget.org without using nuget.exe.

### Removed
- AgilitySDK .pdb files are no longer copied over to the D3D12 .dll folder.

## v0.1.0

- First released version
