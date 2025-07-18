#include "iglo.h"
#include "iglo_font.h"
#include "iglo_batch_renderer.h"
#include "LoadingScreenProgressBar.h"


namespace ig
{

	void LoadingScreenProgressBar::SetCurrentTask(const std::string& taskName)
	{
		this->currentTask = taskName;
	}

	void LoadingScreenProgressBar::DrawAndPresent(IGLOContext& context, CommandList& cmd, BatchRenderer& r, Font& font,
		const std::string& progressBarText, float progress)
	{
		cmd.Begin();
		{
			cmd.AddTextureBarrier(context.GetBackBuffer(), SimpleBarrier::Discard, SimpleBarrier::RenderTarget);
			cmd.FlushBarriers();

			cmd.SetRenderTarget(&context.GetBackBuffer(), nullptr, true);
			cmd.SetViewport((float)context.GetWidth(), (float)context.GetHeight());
			cmd.SetScissorRectangle(context.GetWidth(), context.GetHeight());

			r.Begin(cmd);
			{
				const float progressBarHeight = 32.0f;

				// Set progress bar width and height
				FloatRect barRect(0, 0, (float)context.GetWidth() / 2.0f, progressBarHeight);

				// Position the progress bar
				barRect += Vector2(
					((float)context.GetWidth() / 2.0f) - (barRect.GetWidth() / 2.0f), // X
					((float)context.GetHeight() / 2.0f) - (barRect.GetHeight() / 2.0f)); // Y

				r.DrawRectangle(barRect.left, barRect.top, barRect.GetWidth() * progress, barRect.GetHeight(), Colors::Gray);
				r.DrawRectangleBorder(barRect, Colors::White);

				float totalLineHeight = float(font.GetFontDesc().lineHeight + font.GetFontDesc().lineGap);
				r.DrawString(roundf(barRect.left), roundf(barRect.top - totalLineHeight - 5), currentTask, font, Colors::White);

				Vector2 strSize = r.MeasureString(progressBarText, font);
				Vector2 strPos = Vector2(
					((barRect.left + barRect.right) / 2.0f) - (strSize.x / 2.0f), // X
					((barRect.top + barRect.bottom) / 2.0f) - (strSize.y / 2.0f)); // Y
				r.DrawString(roundf(strPos.x), roundf(strPos.y), progressBarText, font, Colors::White);
			}
			r.End();

			cmd.AddTextureBarrier(context.GetBackBuffer(), SimpleBarrier::RenderTarget, SimpleBarrier::Present);
			cmd.FlushBarriers();
		}
		cmd.End();

		context.Submit(cmd);
		context.Present();
	}

}