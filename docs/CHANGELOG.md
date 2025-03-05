
# Changelog

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
