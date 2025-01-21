#include "iglo.h"
#include "igloFont.h"
#include "igloBatchRenderer.h"
#include "igloMainLoop.h"

#ifdef IGLO_D3D12
// Agility SDK path and version. Support for enhanced barriers and shader model 6.6 is required.
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 715; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }
#endif

ig::IGLOContext context;
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
	context.WaitForIdleDevice(); // Wait for GPU to finish all remaining work before resources get released
}

void Update(double elapsedSeconds)
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
		cmd.AddTextureBarrier(context.GetBackBuffer(), ig::SimpleBarrier::Common, ig::SimpleBarrier::RenderTarget, false);
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
		cmd.AddTextureBarrier(context.GetBackBuffer(), ig::SimpleBarrier::RenderTarget, ig::SimpleBarrier::Common, false);
		cmd.FlushBarriers();
	}
	cmd.End();

	context.Submit(cmd);
	context.Present();
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShow)
#endif
#ifdef __linux__
int main(int argc, char** argv)
#endif
{
	if (context.Load(
		ig::WindowSettings("Hello world!", 640, 480),
		ig::RenderSettings(ig::PresentMode::Vsync)))
	{
		mainloop.Run(context, Start, OnLoopExited, Draw, Update, OnEvent);
	}
	// The iglo objects declared on the stack get released here in the reverse order they were declared
	return 0;
}
