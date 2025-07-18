# iglo

iglo (Intuitive Graphics Layer Open-source) is a cross-platform, public domain C++20 rendering abstraction layer for D3D12 and Vulkan. The aim of iglo is to make these low level graphics APIs more intuitive and fun to use, without limiting their full potential. Additionally, iglo handles window creation and mouse/keyboard input.

## Roadmap

- [x] Helper components for 2D rendering
  -  [x] ig::Font
  -  [x] ig::BatchRenderer
- [ ] Helper components for 3D rendering
  -  [ ] Parse 3D mesh files
  -  [ ] Material shaders and pipelines
  -  [ ] Improved camera class

## Features

- Written from scratch with minimal dependencies.
- Fully bindless rendering with SM6.6's `ResourceDescriptorHeap` and `SamplerDescriptorHeap`.
- Enhanced barriers (D3D12).
- UTF-8 internal string representation.
- Vector, matrix and quaternion classes with essential math functions.
- Helper functions for:
  - File I/O (reading/writing files)
  - Randomness (`rand` and `mt19937`)
  - UTF string conversions and manipulation
- Simple app callbacks: `Start()`, `Update()`, `FixedUpdate()`, `Draw()`, `OnEvent()` and `OnLoopExited()`.
- Easy 2D rendering via `BatchRenderer` (text, sprites, shapes, etc...)
- Font rendering
  - Supports .ttf and .otf files.
  - Supports pre-baked font files (pre-rendered glyph atlases).
  - Supports signed distance fields (SDF) with glow & outline effects.
- Manages the window and handles mouse/keyboard input.

## Gallery

> The `Lighting and Shadows` example project demonstrates how to implement basic lighting and shadows in iglo, how to use multisampled anti-aliasing, and how to use sRGB formats for more accurate lighting calculations.
![](images/example-projects/LightingAndShadows.png)

<details>
<summary>Click to show more</summary>

> The `Camera and Skybox` example project demonstrates how to use the `ig::BaseCamera` class and how to render a skybox. It also shows how you can use instancing to render models (cubes in this case).
![](images/example-projects/CameraAndSkybox.png)

> The `Hello BatchRenderer` example project demonstrates how to use the `ig::BatchRenderer` class to render various types of 2D geometry.
![](images/example-projects/HelloBatchRenderer1.png)
![](images/example-projects/HelloBatchRenderer2.png)
![](images/example-projects/HelloBatchRenderer3.png)
![](images/example-projects/HelloBatchRenderer4.png)
![](images/example-projects/HelloBatchRenderer5.png)
![](images/example-projects/HelloBatchRenderer6.png)
![](images/example-projects/HelloBatchRenderer7.png)

> The `Hello cube` and `Hello world` example projects.
![](images/example-projects/HelloCube.png)
![](images/example-projects/HelloWorld.png)

</details>

## Build and Run

### Requirements

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

### Windows

Follow these steps to build and run the iglo example projects on Windows:

- Install [CMake](https://cmake.org/download/). Version 3.22 or higher is required.
- Ensure CMake is accessible from the command line. 
- For Visual Studio: Run `build.cmd`. Generated project files will appear in `/build/`. 
- For other:
  ```
  cmake -B build
  cmake --build build
  ```
- CMake will automatically download Agility SDK if needed and place it in the build folder.

### Linux

¯\\_(ツ)_/¯

### iglo and Visual Studio

Follow these steps to create a new iglo project in Visual Studio 2019:  

1. Open Visual Studio and create a new **Empty C++ Project**.  

2. Right-click on the project in the Solution Explorer and select **Properties**. Navigate to **Linker** → **System** → **Subsystem**, and set it to **Not Set**.  

3. In the same **Properties** window, go to **General** → **C++ Language Standard**, and select **C++20**.  

4. Copy and paste the example iglo "Hello World" code (provided below) into a new file named `main.cpp`.  

5. Go to **Properties** → **C/C++** → **Additional Include Directories**, and add the path to the `iglo` source folder `/src/`.  

6. In the Solution Explorer, create a new **Filter** named `iglo` under your project. Add all source files from the iglo `/src/` directory to this filter. You don't have to add the files from `/src/stb/` and `/src/shaders/`. 

7. Right-click on the project and select **Manage NuGet Packages...**. Check the box for **Include prerelease**. Install the latest prerelease version of **Microsoft.Direct3D.D3D12** (DirectX 12 Agility SDK).  

8. In your `main.cpp` file, ensure the `UINT D3D12SDKVersion` value matches the version of the Agility SDK you installed.  

You can now build and run your iglo project using the D3D12 backend!
If you want to use the Vulkan backend instead, modify iglo_config.h to enable IGLO_WIN32_VULKAN, then download and install the Vulkan SDK and add its include and library directories to your project.

### Hello world example

<details>
<summary>Click to show code</summary>

```
#include "iglo.h"
#include "iglo_font.h"
#include "iglo_batch_renderer.h"
#include "iglo_main_loop.h"

#ifdef IGLO_D3D12
// Agility SDK path and version. Support for enhanced barriers and shader model 6.6 is required.
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 715; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
#endif

class App
{
public:

    void Run()
    {
        if (context.Load(
            ig::WindowSettings
            {
                .title = "Hello world!",
                .width = 640,
                .height = 480,
            },
            ig::RenderSettings
            {
                .presentMode = ig::PresentMode::Vsync,
            }))
        {
            mainloop.Run(context,
                std::bind(&App::Start, this),
                std::bind(&App::OnLoopExited, this),
                std::bind(&App::Draw, this),
                std::bind(&App::Update, this, std::placeholders::_1),
                std::bind(&App::FixedUpdate, this),
                std::bind(&App::OnEvent, this, std::placeholders::_1));
        }
    }

private:

    ig::IGLOContext context; // IGLOContext must be declared first so it gets unloaded last
    ig::CommandList cmd;
    ig::Font defaultFont;
    ig::BatchRenderer r;
    ig::MainLoop mainloop;

    void Start()
    {
        cmd.Load(context, ig::CommandListType::Graphics);

        cmd.Begin();
        {
            defaultFont.LoadAsPrebaked(context, cmd, ig::GetDefaultFont()); // Load embedded prebaked font
            r.Load(context, cmd, context.GetBackBufferRenderTargetDesc());
        }
        cmd.End();

        // Submit work to the GPU and wait for the work to complete before proceeding
        context.WaitForCompletion(context.Submit(cmd));
    }

    void OnLoopExited()
    {
        // Wait for GPU to finish all remaining work before unloading resources
        context.WaitForIdleDevice();
    }

    // Called once per frame.
    void Update(double elapsedSeconds)
    {
    }

    // Called at a fixed timestep.
    // This frame-rate–independent callback lets you decouple game physics from the visual frame rate.
    void FixedUpdate()
    {
    }

    void OnEvent(ig::Event e)
    {
        if (e.type == ig::EventType::CloseRequest)
        {
            mainloop.Quit();
            return;
        }
    }

    void Draw()
    {
        cmd.Begin();
        {
            // The back buffer will now be used as a render target
            cmd.AddTextureBarrier(context.GetBackBuffer(), ig::SimpleBarrier::Discard, ig::SimpleBarrier::RenderTarget);
            cmd.FlushBarriers();

            cmd.SetRenderTarget(&context.GetBackBuffer());
            cmd.SetViewport((float)context.GetWidth(), (float)context.GetHeight());
            cmd.SetScissorRectangle(context.GetWidth(), context.GetHeight());
            cmd.ClearColor(context.GetBackBuffer(), ig::Colors::Black);

            r.Begin(cmd);
            {
                r.DrawString(64, 64, "Hello world!", defaultFont, ig::Colors::Green);
            }
            r.End();

            // The back buffer will now be used to present
            cmd.AddTextureBarrier(context.GetBackBuffer(), ig::SimpleBarrier::RenderTarget, ig::SimpleBarrier::Present);
            cmd.FlushBarriers();
        }
        cmd.End();

        context.Submit(cmd);
        context.Present();
    }

};

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShow)
#else
int main(int argc, char** argv)
#endif
{
    std::unique_ptr<App> app = std::make_unique<App>();
    app->Run();
    app = nullptr; // All resources get unloaded when the destructor is called
    return 0;
}
```

</details>

## Third Party Libraries

Libraries included with iglo:
- [stb_truetype.h](https://github.com/nothings/stb/): Font text rasterizer.
- [stb_image.h](https://github.com/nothings/stb/): Image loader.
- [stb_image_write.h](https://github.com/nothings/stb/): Image writer.

Libraries *not* included with iglo:
- [Agility SDK](https://devblogs.microsoft.com/directx/directx12agility/): Adds the newest D3D12 features. The CMake script automatically downloads this if needed.

## Authors

- Initial work: [Christoffer Chiniquy](https://github.com/c-chiniquy)

## License

iglo is in the public domain. iglo is also licensed under the MIT open source license for lawyers who are unhappy with public domain.

## Acknowledgments

I found inspiration on how to design iglo from [Alex Tardif's blog post on rendering abstraction layers](https://alextardif.com/RenderingAbstractionLayers.html).

Code snippets used (all of these are public domain):
- [gingerBill's math functions](https://github.com/gingerBill/gb/blob/master/gb_math.h)
- [Jeff Bezanson's utf8 functions](https://github.com/JeffBezanson/cutef8/blob/master/utf8.c)
- [wareya's utf conversion functions](https://github.com/wareya/unishim/blob/master/unishim.h)
- [sheredom's utf8 functions](https://github.com/sheredom/utf8.h/blob/master/utf8.h)
- [littlstar's DDS loading code](https://github.com/littlstar/soil/)
- [Blat Blatnik's precise win32 sleep function](https://github.com/blat-blatnik/Snippets/blob/main/precise_sleep.c)

Assets used:
- [Vegur font](https://www.fontspace.com/vegur-font-f7408): Used as iglo's default font.
