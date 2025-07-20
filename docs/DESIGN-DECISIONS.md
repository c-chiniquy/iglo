
This document contains my personal thoughts behind various design decisions i had to make during development, as well as the coding style and some other info relevant to iglo development.

# Design decisions

- iglo should use the native D3D12/Vulkan command buffers directly.
  Don't use command buffer abstractions on top of the native API ones.
  ([I'm following the advice from this post](https://alextardif.com/RenderingAbstractionLayers.html))

- It should be possible to interact with the graphics API (D3D12/Vulkan) directly.
  This is useful for when you want to use functionality that iglo doesn't support.

- iglo should be lightweight, low level and easy to setup and use.
  iglo can't be perfect at everything. It must have a purpose and be good at achieving its purpose.

- I prefer the name 'IGLOContext' over 'RenderWindow', but i'm open to feedback on this issue.

- iglo shouldn't be able to throw exceptions based on user behaviour, much like D3D12.
  It's OK for iglo to throw exceptions in situations that should be impossible in the first place and would be iglo's own fault.

- iglo shouldn't support runtime shader compilation, it's a bad practice imo (needless waste of CPU).
  Windows Store doesn't support apps that compile shaders at runtime.

- Exclusive fullscreen mode is a thing of the past, and iglo should not support it.
  D3D12 doesn't support it, and windows 10 will "pretend" to be in exclusive fullscreen mode for games that try to enter exclusive fullscreen mode.

# Coding guidelines

- An iglo object that must be initialized before use should follow this design pattern:
  - `bool Load();`\
    Initializes the object. Replaces existing instance if already loaded.
  - `void Unload();`\
    Destroys the object. It's safe to call `Unload()` on an already unloaded object.
  - `bool IsLoaded() const;`\
    Returns whether the object is currently initialized.

- Calling `const` functions on unloaded iglo objects should always be safe.
  For example, `texture.GetWidth()` must return 0 if the texture isn't loaded.

- Functions should not use output parameters.
  Always consider returning a struct for multiple return values, even for vectors and strings.
  There are exceptions to this rule, but you should only make exceptions if there are good reasons for it.\
  Exceptions to this rule so far:
  - utf8 functions. Pointer output params are the fastest, and utf8 iteration should be fast for an app that uses utf8 internal string representation.
  - `bool IGLOContext::GetEvent(Event& out_event)`. Returning a struct here would just make things less readable and more complex for no good reason.

- Good: `ig::Buffer CreateCube();`\
  Bad: `void CreateCube(ig::Buffer& out_vertexBuffer);`

- iglo objects in function arguments should be reference if mandatory, and pointer if optional. (this goes for non iglo objects too)

- Checking if utf8 strings are valid and fixing broken utf8 strings is important.
  Any new utf8 string the app receives should be checked if valid.
  Use the functions `ig::utf8_is_valid` and `ig::utf8_make_valid` to check and repair utf8 strings.

- Since destructors run in reverse order of definitions, declare/define IGLOContext before other iglo resources so that IGLOContext is unloaded last.

## Dynamic arrays

Prefer using `std::vector` over `std::unique_ptr` for dynamic arrays.

### Using vectors efficiently

- Move vector without any copying happening: `vectorA.swap(vectorB);`\
  std::move does not guarantee that a pointer pointing to an element in the vector remains valid. This is why std::swap is preferred over std::move.
- Deallocate vector:
  ```
  vectorA.clear();
  VectorA.shrink_to_fit();
  ```
- Allocate vector with a specified size:
  ```
  vectorA.clear();
  vectorA.resize(newSize, initValue);
  ```


# win32 api

- I made the window wait with being made visible until after the graphics device has loaded.
  This prevents user from seeing an empty window for 0.2 seconds before entering borderless fullscreen, assuming the iglo app tries to enter fullscreen immediately after iglo context is loaded.
  With this change, the window icon also starts out as being set to the custom icon (and not the default icon for 0.2 secs), assuming the iglo app tries to set the window icon immediately after iglo context is loaded.
  With this change, if graphics device failed to load, a popup message will display without any iglo app window being visible, but this is OK.

- I fixed these window related bugs:
  - GetWindowMinimized() didn't always return the correct value.\
    Solution: Never ignore WM_SIZE minimized, maximized and restored messages.
  - There was a 0.2 second white flash when window was just created.\
    Solution: hbrBackground must be set to 0 when creating the window.
              Manually paint window backcolor to black in WM_ERASEBKGND.
  - Dragging the window caused flickering when modal loop was enabled.\
    Solution: hbrBackground must be set to 0 when creating the window.
              Don't manually paint the window backcolor in WM_ERASEBKGND when window is being dragged.
  - Popup messages couldn't get created after a window was destroyed.\
    Solution: Don't use PostQuitMessage() anywhere in your code.\
    [Info from stackoverflow](https://stackoverflow.com/questions/53459584/messagebox-not-being-displayed)

  - Another thing i fixed is being able to have multiple windows at the same time.
    Turns out, the window class must be registered only once, even when there are multiple windows.
    The window class must also be unregistered only once, when there are no more existing windows that use that class.
    I have now implemented static variables to keep track of when to register and unregister the window class.\
    From microsofts documentation:\
    "Before calling this function [UnregisterClassW], an application must destroy all windows created with the specified class."\
    [Info from microsoft](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-unregisterclassw)\
    [Info from stackoverflow](https://stackoverflow.com/questions/150803/side-effects-of-calling-registerwindow-multiple-times-with-same-window-class)

# Backend Technical Requirements

## Windows
- Agility SDK: Requires Windows 10 version 1909 or newer
- Frame rate limiter: Requires Windows 10 version 1803 or newer for high-resolution timer

## Linux
- Vulkan backend confirmed working with proprietary NVIDIA drivers on Linux Mint 21.3 which runs on Ubuntu 22.04.

## D3D12
- Requires support for these features:
  - Feature Level 12_1
  - Resource Binding Tier 3
  - Shader Model 6.6 (requires Agility SDK)
  - Enhanced barriers (requires Agility SDK)
- Minimum supported GPU: GTX 1000-series (and AMD/Intel equivalent)

## Vulkan
- Requires support for Vulkan 1.3, along with some additional extensions/features
- Minimum supported GPU: RTX 2000-series (and AMD/Intel equivalent)

# iglo Requirements

- Supported operating systems:
  - Windows 10 1909 or newer
  - Windows 10 LTSC 2021
  - Windows 11
  - Ubuntu 22.04 or newer
  - Likely works on other Linux distros as well, just haven't tested them yet
- Minimum supported GPUs:
  - GTX 1000-series (or AMD/Intel equivalent generation) for the D3D12 backend
  - RTX 2000-series (or AMD/Intel equivalent generation) for the Vulkan backend
- The D3D12 backend requires the Agility SDK for advanced features such as Shader Model 6.6 and Enhanced Barriers.
