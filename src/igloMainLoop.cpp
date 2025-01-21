#include "iglo.h"
#include "igloMainLoop.h"

namespace ig
{

	void MainLoop::MeasureTimePassed()
	{
		elapsedSeconds = timer.GetSecondsAndReset();
		framesPerSecond = 1.0 / elapsedSeconds;

		// Calculate avarage FPS
		avgFPS_numFrames++;
		avgFPS_totalElapsedSeconds += elapsedSeconds;
		const double avgFPSDuration = 0.5; // Avarage FPS updates every 500 milliseconds
		if (avgFPS_totalElapsedSeconds >= avgFPSDuration)
		{
			avgFPS = (int)round(1.0 / (avgFPS_totalElapsedSeconds / avgFPS_numFrames));
			avgFPS_numFrames = 0;
			avgFPS_totalElapsedSeconds = 0;
		}
	}

	void MainLoop::CallbackTick(void* userData)
	{
		((MainLoop*)userData)->Tick();
	}

	void MainLoop::Tick()
	{
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

		//-------------- Fixed time step Physics --------------//
		if (!mainLoopRunning || !context->IsLoaded()) return;
		if (callbackPhysics)
		{
			if (fixedPhysicsFrameRate > 0)
			{
				timeAccumulator += elapsedSeconds;
				const double physicsTargetTime = (1.0 / fixedPhysicsFrameRate);
				const double maxTime = physicsTargetTime * maxPhysicsStepsInARow;
				if (timeAccumulator > maxTime) timeAccumulator = maxTime;
				while (timeAccumulator >= physicsTargetTime)
				{
					timeAccumulator -= physicsTargetTime;
					if (callbackPhysics) callbackPhysics();
				}
				physicsFrameInterpolation = timeAccumulator / physicsTargetTime;
			}
		}

		//-------------- Draw --------------//
		if (!mainLoopRunning || !context->IsLoaded()) return;
		if (context->GetWindowMinimized())
		{
			if (minimizedWindowBehaviour == MinimizedWindowBehaviour::None)
			{
				if (callbackDraw) callbackDraw();
			}
			if (minimizedWindowBehaviour == MinimizedWindowBehaviour::SkipDraw)
			{
			}
			if (minimizedWindowBehaviour == MinimizedWindowBehaviour::SkipDrawAndSleep1MilliSecond)
			{
				ig::SleepMilliseconds(1);
			}
		}
		else
		{
			if (callbackDraw) callbackDraw();
		}

		//-------------- Time --------------//
		if (!mainLoopRunning || !context->IsLoaded()) return;
		if (frameRateLimit > 0) limiter.LimitFPS(frameRateLimit); // Limit frame rate
		MeasureTimePassed();

	}

	void MainLoop::Run(IGLOContext& context, CallbackStart callback_Start, CallbackOnLoopExited callback_OnLoopExited,
		CallbackDraw callback_Draw, CallbackUpdate callback_Update, CallbackOnEvent callback_OnEvent, bool useModalLoopCallback)
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
		this->callbackOnEvent = callback_OnEvent;

		this->timer.Reset();
		this->elapsedSeconds = 0;
		this->framesPerSecond = 0;
		this->timeAccumulator = 0;
		this->physicsFrameInterpolation = 0;

		this->avgFPS_numFrames = 0;
		this->avgFPS_totalElapsedSeconds = 0;
		this->avgFPS = 0;

		if (this->callbackStart) this->callbackStart();

		if (this->mainLoopRunning && context.IsLoaded())
		{
			CallbackModalLoop oldModalCallback = nullptr;
			void* oldModalCallbackUserData = nullptr;
			if (useModalLoopCallback)
			{
				// Remember the old modal loop callback
				context.GetModalLoopCallback(oldModalCallback, oldModalCallbackUserData);
				// Assign a modal loop callback to prevent the window from freezing during window drag/resize.
				context.SetModalLoopCallback(CallbackTick, this);
			}
			while (this->mainLoopRunning && context.IsLoaded())
			{
				Tick(); // Do updating, drawing etc...
			}
			if (useModalLoopCallback)
			{
				// Restore the modal callback to what it used to be
				context.SetModalLoopCallback(oldModalCallback, oldModalCallbackUserData);
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

	void MainLoop::EnableIdleMode(bool enable)
	{ 
		this->idleMode = enable; 
	}

	void MainLoop::SetFixedTimeStepPhysicsCallback(CallbackPhysics callbackPhysics, double fixedPhysicsFrameRate,
		unsigned int maxPhysicsStepsInARow)
	{
		this->callbackPhysics = callbackPhysics;
		this->fixedPhysicsFrameRate = fixedPhysicsFrameRate;
		this->maxPhysicsStepsInARow = maxPhysicsStepsInARow;
	}

	void MainLoop::SetMinimizedWindowBehaviour(MinimizedWindowBehaviour behaviour)
	{
		this->minimizedWindowBehaviour = behaviour;
	}


} //end of namespace ig