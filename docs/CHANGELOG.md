
# Changelog

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
