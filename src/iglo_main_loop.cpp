#include "iglo.h"
#include "iglo_main_loop.h"

namespace ig
{
	MainLoop::MainLoop()
	{
		SetFixedUpdateFrameRate(60);
	}

	void MainLoop::MeasureTimePassed()
	{
		elapsedSeconds = timer.GetSecondsAndReset();
		framesPerSecond = 1.0 / elapsedSeconds;

		// Calculate avarage FPS
		avgFPS_numFrames++;
		avgFPS_totalElapsedSeconds += elapsedSeconds;
		const double avgFPSDuration = 0.5; // The duration the avarage FPS is calculated
		if (avgFPS_totalElapsedSeconds >= avgFPSDuration)
		{
			avgFPS = (int)round(1.0 / (avgFPS_totalElapsedSeconds / avgFPS_numFrames));
			avgFPS_numFrames = 0;
			avgFPS_totalElapsedSeconds = 0;
		}
	}

	void MainLoop::Tick()
	{
		//-------------- Time --------------//
		if (!mainLoopRunning || !context->IsLoaded()) return;
		if (enableFixedFrameRateSync)
		{
			if (fixedUpdateFrameRate > 0)
			{
				// Sync the visual frame rate with the fixed update frame rate.
				// This is done by sleeping the amount of time remaining in the fixed update time accumulator.
				const double fixedTargetTime = (1.0 / fixedUpdateFrameRate);
				double currentSecondsElapsed = timer.GetSeconds();
				if (timeAccumulator + currentSecondsElapsed < fixedTargetTime)
				{
					// Not enough time has passed for FixedUpdate() to be called.
					// Sleep here until enough time has passed.
					PreciseSleep(fixedTargetTime - (timeAccumulator + currentSecondsElapsed));
				}
			}
		}
		else
		{
			// Use the independent frame rate limiter (it measures its own time).
			if (frameRateLimit > 0) limiter.LimitFPS(frameRateLimit);
		}
		MeasureTimePassed();

		//-------------- Events --------------//
		Event e;
		if (idleMode)
		{
			context->WaitForEvent(e, 70); // If no event is pending, then thread will sleep here for a little while.
			if (!mainLoopRunning || !context->IsLoaded()) return;
			if (callbackOnEvent) callbackOnEvent(e);
		}
		while (context->GetEvent(e))
		{
			if (!mainLoopRunning || !context->IsLoaded()) return;
			if (callbackOnEvent) callbackOnEvent(e);
		}

		//-------------- Update --------------//
		if (!mainLoopRunning || !context->IsLoaded()) return;
		if (callbackUpdate) callbackUpdate(elapsedSeconds);

		//-------------- Fixed time step updates --------------//
		if (!mainLoopRunning || !context->IsLoaded()) return;
		if (fixedUpdateFrameRate > 0)
		{
			timeAccumulator += elapsedSeconds;
			const double fixedTargetTime = (1.0 / fixedUpdateFrameRate);
			const double maxTime = fixedTargetTime * maxFixedUpdatesInARow;
			if (timeAccumulator > maxTime) timeAccumulator = maxTime;
			while (timeAccumulator >= fixedTargetTime)
			{
				timeAccumulator -= fixedTargetTime;
				if (callbackFixedUpdate) callbackFixedUpdate();
			}
			fixedUpdateFrameInterpolation = timeAccumulator / fixedTargetTime;
		}

		//-------------- Draw --------------//
		if (!mainLoopRunning || !context->IsLoaded()) return;
		if (context->GetWindowMinimized())
		{
			if (windowMinimizedBehaviour == WindowMinimizedBehaviour::None)
			{
				if (callbackDraw) callbackDraw();
			}
			else if (windowMinimizedBehaviour == WindowMinimizedBehaviour::SkipDraw)
			{
			}
			else if (windowMinimizedBehaviour == WindowMinimizedBehaviour::SkipDrawAndSleep)
			{
				BasicSleep(MillisecondsToSleepWhenMinimized);
			}
		}
		else
		{
			if (callbackDraw) callbackDraw();
		}
	}

	void MainLoop::Run(IGLOContext& context, CallbackStart callback_Start, CallbackOnLoopExited callback_OnLoopExited,
		CallbackDraw callback_Draw, CallbackUpdate callback_Update, CallbackFixedUpdate callback_FixedUpdate,
		CallbackOnEvent callback_OnEvent, bool useModalLoopCallback)
	{
		if (!context.IsLoaded())
		{
			Log(LogType::Error, "Failed to start main loop. Reason: iglo context is not loaded.");
			return;
		}
		if (this->started)
		{
			Log(LogType::Error, "Failed to start main loop. Reason: This main loop is already running.");
			return;
		}
		if (context.IsInsideModalLoopCallback())
		{
			Log(LogType::Warning, "Main loop started while inside a modal loop callback. This may make the window unresponsive.");
		}
		this->context = &context;
		this->mainLoopRunning = true;
		this->started = true;

		this->callbackStart = callback_Start;
		this->callbackOnLoopExited = callback_OnLoopExited;
		this->callbackDraw = callback_Draw;
		this->callbackUpdate = callback_Update;
		this->callbackFixedUpdate = callback_FixedUpdate;
		this->callbackOnEvent = callback_OnEvent;

		this->timer.Reset();
		this->elapsedSeconds = 0;
		this->framesPerSecond = 0;
		this->timeAccumulator = 0;
		this->fixedUpdateFrameInterpolation = 0;

		this->avgFPS_numFrames = 0;
		this->avgFPS_totalElapsedSeconds = 0;
		this->avgFPS = 0;

		if (this->callbackStart) this->callbackStart();

		if (this->mainLoopRunning && context.IsLoaded())
		{
			CallbackModalLoop oldModalCallback = nullptr;
			if (useModalLoopCallback)
			{
				// Remember the old modal loop callback
				oldModalCallback = context.GetModalLoopCallback();
				// Assign a modal loop callback to prevent the window from freezing during window drag/resize.
				context.SetModalLoopCallback(std::bind(&MainLoop::Tick, this));
			}
			while (this->mainLoopRunning && context.IsLoaded())
			{
				Tick(); // Do updating, drawing etc...
			}
			if (useModalLoopCallback)
			{
				// Restore the modal callback to what it used to be
				context.SetModalLoopCallback(oldModalCallback);
			}
		}
		if (this->callbackOnLoopExited) this->callbackOnLoopExited();
		this->started = false;
		this->mainLoopRunning = false;
	}

	void MainLoop::Quit()
	{
		this->mainLoopRunning = false;
	}

	void MainLoop::SetFrameRateLimit(double frameRateLimit)
	{
		this->frameRateLimit = frameRateLimit;
	}

	void MainLoop::EnableFixedFrameRateSync(bool enable)
	{
		this->enableFixedFrameRateSync = enable;
	}

	void MainLoop::EnableIdleMode(bool enable)
	{
		this->idleMode = enable;
	}

	void MainLoop::SetFixedUpdateFrameRate(double fixedUpdateFrameRate, uint32_t maxFixedUpdatesInARow)
	{
		this->fixedUpdateFrameRate = fixedUpdateFrameRate;
		this->maxFixedUpdatesInARow = maxFixedUpdatesInARow;
	}

	void MainLoop::SetWindowMinimizedBehaviour(WindowMinimizedBehaviour behaviour)
	{
		this->windowMinimizedBehaviour = behaviour;
	}


} //end of namespace ig