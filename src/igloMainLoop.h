
#pragma once

namespace ig
{

	class MainLoop
	{
	public:
		MainLoop() = default;
		~MainLoop() = default;

		MainLoop& operator=(MainLoop&) = delete;
		MainLoop(MainLoop&) = delete;

		enum class MinimizedWindowBehaviour
		{
			// MainLoop does not change its behaviour when window is minimized
			None = 0,

			// When window is minimized, MainLoop will skip the Draw() callback.
			SkipDraw,

			// When window is minimized, MainLoop will skip the Draw() callback and
			// will sleep for 1 millisecond each frame to prevent 100% CPU usage.
			SkipDrawAndSleep1MilliSecond,
		};

		typedef void(*CallbackOnEvent)(Event e);
		typedef void(*CallbackStart)();
		typedef void(*CallbackOnLoopExited)();
		typedef void(*CallbackUpdate)(double elapsedSeconds);
		typedef void(*CallbackDraw)();
		typedef void(*CallbackPhysics)();

		// Enters the main application loop.
		// The 'Start' callback is called before entering the loop, and 'OnLoopExited' is called after exiting the loop.
		// If 'useModalLoopCallback' is true, app will not freeze when resizing/moving the window, with the trade-off being that
		// you can't nest another MainLoop or GetEvent() loop inside Update(),Draw(),OnEvent() when window is being dragged/resized.
		void Run(IGLOContext&, CallbackStart, CallbackOnLoopExited, CallbackDraw, CallbackUpdate, CallbackOnEvent,
			bool useModalLoopCallback = true);

		// Tells MainLoop that it should exit the loop the first chance it gets.
		void Quit();
		// If true, Quit() has been called and the loop will exit soon.
		bool IsQuitting() const { return !mainLoopRunning; }

		// Limits the frames per second.
		// Set to 0 to disable the frame rate limit.
		void SetFrameRateLimit(double frameRateLimit);
		double GetFrameRateLimit() const { return frameRateLimit; }

		// Idle mode takes advantage of the behavior of WaitForEvent() to minimize CPU/GPU usage while keeping input responsiveness high. 
		// The thread sleeps each frame until an event is received or a certain amount of time has passed.
		// This results in much lower FPS when no window/keyboard/mouse events occur, making it ideal for idle-heavy GUI apps 
		// that remain inactive for extended periods.
		// (Note: This is unrelated to the frame rate limiter.)
		void EnableIdleMode(bool enable);
		bool GetIdleModeEnabled() const { return this->idleMode; }

		// Allows the physics framerate to be decoupled from the visual framerate.
		// Useful when physics must run at a fixed rate, and you don't want higher refresh rates or Vsync to impact physics timing.
		// The Physics callback is invoked whenever sufficient time has accumulated, based on 'fixedPhysicsFrameRate'.
		// To disable the callback, pass 'nullptr'.
		void SetFixedTimeStepPhysicsCallback(CallbackPhysics, double fixedPhysicsFrameRate = 60,
			unsigned int maxPhysicsStepsInARow = 2);

		// Returns a value between 0.0 and 1.0 for interpolating between physics frames during rendering.
		// Designed for use with the fixed time step physics callback, this enables smooth visual updates 
		// by making physics appear to match the monitor's refresh rate, even when running at a fixed time step.
		// A lower value indicates the frame is closer to the previous physics state, while a higher value 
		// indicates it is closer to the next physics state.
		double GetPhysicsFrameInterpolation() const { return physicsFrameInterpolation; }

		// Set how MainLoop should behave when the window is minimized.
		void SetMinimizedWindowBehaviour(MinimizedWindowBehaviour);
		MinimizedWindowBehaviour GetMinimizedWindowBehaviour() const { minimizedWindowBehaviour; }

		// Seconds elapsed since last frame. 60 FPS = 0.0166667 elapsed seconds per frame.
		double GetElapsedSeconds() const { return elapsedSeconds; }

		// Frames per second
		double GetFPS() const { return framesPerSecond; }

		// Avarage frames per second. Rounded to nearest integer and updates only a few times every second.
		int GetAvarageFPS() const { return avgFPS; }
	private:
		static void CallbackTick(void* userData);
		void Tick();
		void MeasureTimePassed();

		IGLOContext* context = nullptr;
		bool mainLoopRunning = false;
		bool started = false;
		bool idleMode = false;

		// Default behaviour
		MinimizedWindowBehaviour minimizedWindowBehaviour = MinimizedWindowBehaviour::SkipDrawAndSleep1MilliSecond;

		// Standard callbacks
		CallbackOnEvent callbackOnEvent = nullptr;
		CallbackStart callbackStart = nullptr;
		CallbackOnLoopExited callbackOnLoopExited = nullptr;
		CallbackUpdate callbackUpdate = nullptr;
		CallbackDraw callbackDraw = nullptr;

		// Fixed time step physics
		double timeAccumulator = 0;
		double fixedPhysicsFrameRate = 0;
		double physicsFrameInterpolation = 0; // Value between 0.0 and 1.0
		unsigned int maxPhysicsStepsInARow = 0;
		CallbackPhysics callbackPhysics = nullptr;

		// Framerate limit
		FrameRateLimiter limiter;
		double frameRateLimit = 0;

		// Time
		Timer timer;
		double framesPerSecond = 0;
		double elapsedSeconds = 0;

		// Avarage FPS
		unsigned int avgFPS_numFrames = 0; // The number of frames that has occured since started measuring avarage FPS
		double avgFPS_totalElapsedSeconds = 0; // The total elapsed time since started measuring avarage FPS
		int avgFPS = 0;
	};




} //end of namespace ig