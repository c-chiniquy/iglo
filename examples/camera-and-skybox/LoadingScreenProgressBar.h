#pragma once

namespace ig
{

	// A simple progress bar to replace your empty screen during long loading times.
	class LoadingScreenProgressBar
	{
	public:
		LoadingScreenProgressBar() = default;

		void SetCurrentTask(const std::string& taskName);
		void DrawAndPresent(IGLOContext&, CommandList&, BatchRenderer&, Font&, const std::string& progressBarText, float progress);

	private:

		std::string currentTask = "";
	};

}