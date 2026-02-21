#include "iglo.h"
#include "iglo_font.h"
#include "iglo_batch_renderer.h"
#include "iglo_main_loop.h"

#ifdef IGLO_D3D12
// Agility SDK path and version
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 717; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
#endif

class App
{
public:

	void Run()
	{
		context = ig::IGLOContext::CreateContext(
			ig::WindowSettings
			{
				.title = "Hello world!",
				.width = 640,
				.height = 480,
			},
			ig::RenderSettings
			{
				.presentMode = ig::PresentMode::Vsync,
			});

		if (context)
		{
			mainloop.Run(*context,
				std::bind(&App::Start, this),
				std::bind(&App::OnLoopExited, this),
				std::bind(&App::Draw, this),
				std::bind(&App::Update, this, std::placeholders::_1),
				std::bind(&App::FixedUpdate, this),
				std::bind(&App::OnEvent, this, std::placeholders::_1));
		}
	}

private:

	std::unique_ptr<ig::IGLOContext> context; // IGLOContext is declared first so it gets destroyed last
	std::unique_ptr<ig::CommandList> cmd;
	std::unique_ptr<ig::BatchRenderer> r;
	std::unique_ptr<ig::Font> defaultFont;
	ig::MainLoop mainloop;

	void Start()
	{
		cmd = ig::CommandList::Create(*context, ig::CommandListType::Graphics);
		r = ig::BatchRenderer::Create(*context, context->GetBackBufferRenderTargetDesc());

		cmd->Begin();
		{
			defaultFont = ig::Font::CreatePrebaked(*context, *cmd, ig::GetDefaultFont()); // Load embedded font
		}
		cmd->End();

		// Submit work to the GPU and wait for the work to complete before proceeding
		context->WaitForCompletion(context->Submit(*cmd));
	}

	void OnLoopExited()
	{
		// Wait for GPU to finish all remaining work before destroying resources
		context->WaitForIdleDevice();
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
		cmd->Begin();
		{
			// The back buffer will now be used as a render target
			cmd->AddTextureBarrier(context->GetBackBuffer(), ig::SimpleBarrier::Discard, ig::SimpleBarrier::RenderTarget);
			cmd->FlushBarriers();

			cmd->SetRenderTarget(&context->GetBackBuffer());
			cmd->SetViewport((float)context->GetWidth(), (float)context->GetHeight());
			cmd->SetScissorRectangle(context->GetWidth(), context->GetHeight());
			cmd->ClearColor(context->GetBackBuffer(), ig::Colors::Black);

			r->Begin(*cmd);
			{
				r->DrawString(64, 64, "Hello world!", *defaultFont, ig::Colors::Green);
			}
			r->End();

			// The back buffer will now be used to present
			cmd->AddTextureBarrier(context->GetBackBuffer(), ig::SimpleBarrier::RenderTarget, ig::SimpleBarrier::Present);
			cmd->FlushBarriers();
		}
		cmd->End();

		context->Submit(*cmd);
		context->Present();
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
	app = nullptr; // All resources get cleaned up in the destructor
	return 0;
}
