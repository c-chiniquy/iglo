
#pragma once

namespace ig
{

	class MainLoop
	{
	public:
		MainLoop();
		~MainLoop() = default;

		MainLoop& operator=(MainLoop&) = delete;
		MainLoop(MainLoop&) = delete;

		enum class WindowMinimizedBehaviour
		{
			// MainLoop does not change its behaviour when window is minimized.
			None = 0,

			// When window is minimized, MainLoop will skip the Draw() callback.
			SkipDraw,

			// When window is minimized, MainLoop will skip the Draw() callback and
			// will sleep for a few milliseconds every frame to prevent 100% CPU usage.
			SkipDrawAndSleep,
		};

		using CallbackOnEvent = std::function<void(Event e)>;
		using CallbackStart = std::function<void()>;
		using CallbackOnLoopExited = std::function<void()>;
		using CallbackUpdate = std::function<void(double elapsedSeconds)>;
		using CallbackFixedUpdate = std::function<void()>;
		using CallbackDraw = std::function<void()>;

		// Enters the main application loop.
		// The 'Start' callback is called before entering the loop, and 'OnLoopExited' is called after exiting the loop.
		// If 'useModalLoopCallback' is true, app will not freeze when resizing/moving the window, with the trade-off being that
		// you can't nest another MainLoop or GetEvent() loop inside any MainLoop callbacks when window is being dragged/resized.
		void Run(IGLOContext&,
			CallbackStart,
			CallbackOnLoopExited,
			CallbackDraw,
			CallbackUpdate,
			CallbackFixedUpdate,
			CallbackOnEvent,
			bool useModalLoopCallback = true);

		// Tells MainLoop that it should exit the loop the first chance it gets.
		void Quit();
		// If true, Quit() has been called and the loop will exit soon.
		bool IsQuitting() const { return !mainLoopRunning; }

		// Limits the frames per second.
		// Set to 0 to disable the frame rate limit.
		void SetFrameRateLimit(double frameRateLimit);
		double GetFrameRateLimit() const { return frameRateLimit; }

		// If enabled, the frame rate will be capped and synced to the fixed update frame rate.
		// This setting will override the frame rate set with SetFrameRateLimit().
		// While enabled, it ensures that FixedUpdate() is called atleast once before Draw() is called every frame.
		// It will still be possible for FixedUpdate() to be called multiple times in a row (when frames are behind schedule).
		// This is useful for games that must run its game physics at a fixed frame rate without using frame interpolation when rendering
		// and don't want to waste GPU resources rendering a frame with no new information in it.
		// So if you don't want to use frame interpolation, and you still want physics to run at a fixed frame rate,
		// then you should probably enable this setting!
		void EnableFixedFrameRateSync(bool enable);
		bool GetFixedFrameRateSyncEnabled() const { return enableFixedFrameRateSync; };

		// Idle mode takes advantage of the behavior of WaitForEvent() to minimize CPU/GPU usage while keeping input responsiveness high. 
		// While enabled, each frame the thread will sleep until an event is received or a certain amount of time has passed.
		// This results in much lower FPS when no window/keyboard/mouse events occur, making it ideal for idle-heavy GUI apps 
		// that remain inactive for extended periods.
		// (Note: This is unrelated to the frame rate limiter.)
		void EnableIdleMode(bool enable);
		bool GetIdleModeEnabled() const { return this->idleMode; }

		// The FixedUpdate callback allows the game physics frame rate to be decoupled from the visual frame rate.
		// This is useful for when game physics must run at a fixed rate, and you don't want higher refresh rates or Vsync to impact physics timing.
		// The FixedUpdate callback is invoked whenever sufficient time has accumulated, based on 'fixedUpdateFrameRate'.
		// The default value for 'fixedUpdateFrameRate' is 60.
		void SetFixedUpdateFrameRate(double fixedUpdateFrameRate, uint32_t maxFixedUpdatesInARow = 2);
		double GetFixedUpdateFrameRate() const { return fixedUpdateFrameRate; }

		// Returns a value between 0.0 and 1.0 for interpolating between fixed update frames during rendering.
		// This is meant to be used together with the fixed timestep update callback (FixedUpdate).
		// This enables smooth visual updates by making game physics appear to match the monitor's refresh rate.
		// A lower value indicates the frame is closer to the previous physics state, while a higher value 
		// indicates it is closer to the next physics state.
		double GetFixedUpdateFrameInterpolation() const { return fixedUpdateFrameInterpolation; }

		// Set how MainLoop should behave when the window is minimized.
		void SetWindowMinimizedBehaviour(WindowMinimizedBehaviour);
		WindowMinimizedBehaviour GetWindowMinimizedBehaviour() const { return windowMinimizedBehaviour; }

		// Seconds elapsed since last frame. 60 FPS = 0.0166667 elapsed seconds per frame.
		double GetElapsedSeconds() const { return elapsedSeconds; }

		// Frames per second
		double GetFPS() const { return framesPerSecond; }

		// Avarage frames per second. Rounded to nearest integer and updates only a few times every second.
		int32_t GetAvarageFPS() const { return avgFPS; }

	private:
		IGLOContext* context = nullptr;
		bool mainLoopRunning = false;
		bool started = false;
		bool idleMode = false;

		// Default behaviour
		WindowMinimizedBehaviour windowMinimizedBehaviour = WindowMinimizedBehaviour::SkipDrawAndSleep;

		// Callbacks
		CallbackOnEvent callbackOnEvent = nullptr;
		CallbackStart callbackStart = nullptr;
		CallbackOnLoopExited callbackOnLoopExited = nullptr;
		CallbackUpdate callbackUpdate = nullptr;
		CallbackFixedUpdate callbackFixedUpdate = nullptr;
		CallbackDraw callbackDraw = nullptr;

		// Fixed time step update
		uint32_t maxFixedUpdatesInARow = 0;
		double fixedUpdateFrameRate = 0;
		double fixedUpdateFrameInterpolation = 0; // A value between 0.0 and 1.0
		double timeAccumulator = 0;

		// Framerate limit
		FrameRateLimiter limiter;
		double frameRateLimit = 0;
		bool enableFixedFrameRateSync = false;

		// Time
		Timer timer;
		double framesPerSecond = 0;
		double elapsedSeconds = 0;

		// Avarage FPS
		uint32_t avgFPS_numFrames = 0; // Number of frames passed since start of measurement
		double avgFPS_totalElapsedSeconds = 0; // The total elapsed time since started measuring avarage FPS
		int32_t avgFPS = 0;

		void Tick();
		void MeasureTimePassed();

		static constexpr uint32_t MillisecondsToSleepWhenMinimized = 15;
	};


} //end of namespace ig