
#ifdef _WIN32

#include "iglo.h"

namespace ig
{
	constexpr UINT windowClassStyle = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	constexpr LONG windowStyleWindowed = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	constexpr LONG windowStyleWindowedResizable = windowStyleWindowed | WS_THICKFRAME | WS_MAXIMIZEBOX;
	constexpr LONG windowStyleBorderless = WS_POPUP;
	const LPCWSTR windowClassName = L"iglo";

	// We must keep track of the number of windows that exist in total because a window class should only be registered once,
	// and unregistered only after the last window is destroyed.
	unsigned int numWindows = 0;
	bool windowClassIsRegistered = false;

	void SetWindowState_Win32(Impl_WindowData& window, const WindowState& state, bool topMost, bool gainFocus)
	{
		if (!window.hwnd) return;
		UINT style;
		if (state.bordersVisible)
		{
			if (state.resizable) style = windowStyleWindowedResizable;
			else style = windowStyleWindowed;
		}
		else
		{
			style = windowStyleBorderless;
		}
		if (state.visible) style |= WS_VISIBLE;
		UINT resizableFlag = state.resizable ? WS_THICKFRAME | WS_MAXIMIZEBOX : 0;

		// Calling SetWindowLongPtr can sometimes send a WM_SIZE event with incorrect size.
		// We want to ignore these WM_SIZE messages.
		window.ignoreSizeMsg = true;
		SetWindowLongPtr(window.hwnd, GWL_STYLE, style);
		window.ignoreSizeMsg = false;

		RECT rc;
		SetRect(&rc, 0, 0, (int)state.size.width, (int)state.size.height);
		AdjustWindowRect(&rc, style, false);
		HWND insertAfter = HWND_NOTOPMOST;
		if (topMost) insertAfter = HWND_TOPMOST;
		UINT flags = SWP_FRAMECHANGED;
		if (!gainFocus) flags |= SWP_NOACTIVATE;
		SetWindowPos(window.hwnd, insertAfter, state.pos.x, state.pos.y, rc.right - rc.left, rc.bottom - rc.top, flags);
	}

	void CaptureMouse_Win32(Impl_WindowData& window, bool captured)
	{
		if (!window.hwnd) return;
		if (captured) SetCapture(window.hwnd);
		else ReleaseCapture();
	}

	void PopupMessage(const std::string& message, const std::string& caption, const IGLOContext* parent)
	{
		HWND hwnd = 0;
		if (parent) hwnd = parent->GetWindowHWND();
		MessageBoxW(hwnd, utf8_to_utf16(message).c_str(), utf8_to_utf16(caption).c_str(), 0);
	}

	Extent2D IGLOContext::GetActiveMonitorScreenResolution()
	{
		Extent2D out = {};
		HMONITOR active_monitor;
		if (window.hwnd)
		{
			active_monitor = MonitorFromWindow(window.hwnd, MONITOR_DEFAULTTONEAREST);
		}
		else
		{
			active_monitor = MonitorFromWindow(0, MONITOR_DEFAULTTOPRIMARY);
		}
		MONITORINFO minfo;
		minfo.cbSize = sizeof(minfo);
		if (GetMonitorInfo(active_monitor, &minfo))
		{
			out.width = unsigned int(abs(minfo.rcMonitor.right - minfo.rcMonitor.left));
			out.height = unsigned int(abs(minfo.rcMonitor.bottom - minfo.rcMonitor.top));
		}
		else
		{
			Log(LogType::Error, "Failed to get active screen resolution.");
		}
		return out;
	}

	std::vector<Extent2D> IGLOContext::GetAvailableScreenResolutions()
	{
		DEVMODEW devmode = {};
		std::vector<Extent2D> resolutions;

		for (DWORD i = 0; EnumDisplaySettingsW(nullptr, i, &devmode); i++)
		{
			Extent2D res = { devmode.dmPelsWidth, devmode.dmPelsHeight };
			if (std::find(resolutions.begin(), resolutions.end(), res) == resolutions.end())
			{
				resolutions.push_back(res);
			}
		}
		return resolutions;
	}

	bool IGLOContext::GetEvent(Event& out_event)
	{
		if (window.hwnd && !insideModalLoopCallback)
		{
			MSG msg;
			while (PeekMessage(&msg, window.hwnd, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		if (eventQueue.empty())
		{
			return false;
		}
		else
		{
			out_event = eventQueue.front();
			eventQueue.pop();
			return true;
		}
	}

	bool IGLOContext::WaitForEvent(Event& out_event, uint32_t timeoutMilliseconds)
	{
		if (eventQueue.empty())
		{
			if (window.hwnd && !insideModalLoopCallback)
			{
				if (MsgWaitForMultipleObjects(0, NULL, FALSE, timeoutMilliseconds, QS_ALLINPUT) == WAIT_OBJECT_0)
				{
					MSG msg;
					while (PeekMessage(&msg, window.hwnd, 0, 0, PM_REMOVE))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
			}
		}
		if (eventQueue.empty())
		{
			return false;
		}
		else
		{
			out_event = eventQueue.front();
			eventQueue.pop();
			return true;
		}
	}

	void IGLOContext::Impl_SetWindowIconFromImage_BGRA(uint32_t width, uint32_t height, const uint32_t* iconPixels)
	{
		HINSTANCE hInstance = (HINSTANCE)GetModuleHandleW(0);
		if (window.iconOwned)
		{
			DestroyIcon(window.iconOwned);
			window.iconOwned = 0;
		}
		window.iconOwned = CreateIcon(hInstance, width, height, 1, 32, nullptr, (byte*)iconPixels);
		SendMessage(window.hwnd, WM_SETICON, ICON_SMALL, (LPARAM)window.iconOwned);
		SendMessage(window.hwnd, WM_SETICON, ICON_BIG, (LPARAM)window.iconOwned);
	}

	void IGLOContext::SetWindowIconFromResource(int icon)
	{
		if (!window.hwnd) return;
		// If window used an owned icon, destroy that icon
		if (window.iconOwned)
		{
			DestroyIcon(window.iconOwned);
			window.iconOwned = 0;
		}
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(icon));
		SendMessage(window.hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(window.hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}

	void IGLOContext::SetWndProcHookCallback(CallbackWndProcHook callback)
	{
		window.callbackWndProcHook = callback;
	}

	void IGLOContext::CenterWindow()
	{
		Extent2D res = GetActiveMonitorScreenResolution();
		RECT rect = { NULL };
		if (GetWindowRect(window.hwnd, &rect))
		{
			int32_t totalWindowWidth = rect.right - rect.left;
			int32_t totalWindowHeight = rect.bottom - rect.top;
			int32_t x = (int32_t(res.width) / 2) - (totalWindowWidth / 2);
			int32_t y = (int32_t(res.height) / 2) - (totalWindowHeight / 2);
			SetWindowPosition(x, y);
		}
	}

	void IGLOContext::EnableDragAndDrop(bool enable)
	{
		window.enableDragAndDrop = enable;
		if (!window.hwnd) return;
		DragAcceptFiles(window.hwnd, enable);
	}

	void IGLOContext::Impl_SetWindowVisible(bool visible)
	{
		if (visible) ShowWindow(window.hwnd, SW_SHOW);
		else ShowWindow(window.hwnd, SW_HIDE);
	}
	void IGLOContext::Impl_SetWindowSize(uint32_t width, uint32_t height)
	{
		SetWindowState_Win32(window, windowedMode, false, false);
	}
	void IGLOContext::Impl_SetWindowPosition(int32_t x, int32_t y)
	{
		SetWindowPos(window.hwnd, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	}
	void IGLOContext::Impl_SetWindowResizable(bool resizable)
	{
		SetWindowState_Win32(window, windowedMode, false, false);
	}
	void IGLOContext::Impl_SetWindowBordersVisible(bool visible)
	{
		SetWindowState_Win32(window, windowedMode, false, false);
	}

	void IGLOContext::SetWindowTitle(std::string title)
	{
		window.title = title;
		SetWindowTextW(window.hwnd, utf8_to_utf16(title).c_str());
	}

	void IGLOContext::EnterWindowedMode(uint32_t width, uint32_t height)
	{
		displayMode = DisplayMode::Windowed;

		// If the window is in a minimized state, you can't resize it.
		// Restoring a minimized/maximized window with ShowWindow() sends a WM_SIZE event, which may modify windowedMode.size.
		ShowWindow(window.hwnd, SW_RESTORE);
		ShowWindow(window.hwnd, windowedMode.visible ? SW_SHOW : SW_HIDE);
		SetForegroundWindow(window.hwnd); // Set keyboard focus to this window

		windowedMode.size = Extent2D(width, height);
		SetWindowState_Win32(window, windowedMode, false, true);
	}

	void IGLOContext::EnterFullscreenMode()
	{
		displayMode = DisplayMode::BorderlessFullscreen;

		Extent2D res = GetActiveMonitorScreenResolution();

		// If getting desktop resolution failed, default to the most common resolution
		if (res.width == 0 || res.height == 0)
		{
			res.width = 1920;
			res.height = 1080;
		}

		ShowWindow(window.hwnd, SW_RESTORE);
		SetForegroundWindow(window.hwnd); // Set keyboard focus to this window

		WindowState temp;
		temp.pos = IntPoint(0, 0);
		temp.size = res;
		temp.resizable = false;
		temp.bordersVisible = false;
		temp.visible = true;
		SetWindowState_Win32(window, temp, true, true);
	}

	DetailedResult IGLOContext::Impl_LoadWindow(const WindowSettings& windowSettings)
	{
		HINSTANCE hInstance = (HINSTANCE)GetModuleHandleW(0);
		HCURSOR ArrowCursor = LoadCursorW(0, IDC_ARROW);

		// Create window class
		WNDCLASSEXW wc = {};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.hInstance = hInstance;
		wc.lpszClassName = windowClassName;
		wc.lpfnWndProc = WndProc;
		wc.style = windowClassStyle;
		wc.hbrBackground = 0; // Assigning a brush to hbrBackground causes the window to flicker when being dragged, so leave this at 0.
		wc.hCursor = ArrowCursor;

		// Only register the window class if it isn't already registered.
		// Registering an already registered class will result in an error.
		if (!windowClassIsRegistered)
		{
			if (RegisterClassExW(&wc))
			{
				windowClassIsRegistered = true;
			}
			else
			{
				return DetailedResult::MakeFail("RegisterClassExW() failed.");
			}
		}

		// Create window
		window.hwnd = CreateWindowExW(0, windowClassName, utf8_to_utf16(window.title).c_str(), windowStyleWindowed,
			CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0, hInstance, this);
		if (window.hwnd)
		{
			numWindows++;
		}
		else
		{
			return DetailedResult::MakeFail("CreateWindowExW() failed.");
		}

		// Get window location
		RECT rect = {};
		if (GetWindowRect(window.hwnd, &rect)) windowedMode.pos = IntPoint(rect.left, rect.top);

		// Don't make the window visible yet. Window should be made visible after graphics device has been loaded.
		WindowState temp = windowedMode;
		temp.visible = false;
		SetWindowState_Win32(window, temp, false, false);

		SetCursor(ArrowCursor);
		window.iconOwned = 0;

		if (windowSettings.centered)
		{
			CenterWindow();
		}

		return DetailedResult::MakeSuccess();
	}

	void IGLOContext::PrepareWindowPostGraphics(const WindowSettings& windowSettings)
	{
		// Show the window after the graphics device has been created
		if (windowedMode.visible) ShowWindow(window.hwnd, SW_NORMAL);
	}

	void IGLOContext::Impl_UnloadWindow()
	{
		if (window.iconOwned)
		{
			DestroyIcon(window.iconOwned);
			window.iconOwned = 0;
		}
		if (window.hwnd)
		{
			DestroyWindow(window.hwnd);
			window.hwnd = 0;
			if (numWindows > 0)
			{
				numWindows--;
				// Only unregister the window class if this was the last existing window.
				if (numWindows == 0 && windowClassIsRegistered)
				{
					UnregisterClassW(windowClassName, 0);
					windowClassIsRegistered = false;
				}
			}
		}
		window = Impl_WindowData();
	}

	LRESULT CALLBACK IGLOContext::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		IGLOContext* me;
		if (msg == WM_NCCREATE)
		{
			LPCREATESTRUCT createInfo = reinterpret_cast<LPCREATESTRUCT>(lParam);
			me = static_cast<IGLOContext*>(createInfo->lpCreateParams);
			if (me)
			{
				me->window.hwnd = hwnd;
				SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(me));
			}
		}
		else
		{
			me = reinterpret_cast<IGLOContext*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}
		if (me) return me->PrivateWndProc(msg, wParam, lParam);
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	LRESULT CALLBACK IGLOContext::PrivateWndProc(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (window.callbackWndProcHook)
		{
			if (window.callbackWndProcHook(window.hwnd, msg, wParam, lParam)) return 0;
		}

		Event event_out;
		event_out.type = EventType::None;

		switch (msg)
		{
		case WM_DROPFILES:
		{
			event_out.type = EventType::DragAndDrop;
			event_out.dragAndDrop.filenames.clear();

			HDROP hDrop = (HDROP)wParam;
			UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, 0, 0);
			UINT maxLength = 0;
			for (UINT i = 0; i < fileCount; i++)
			{
				UINT fileNameLength = DragQueryFileW(hDrop, i, 0, 0) + 1;
				if (fileNameLength > maxLength)
				{
					maxLength = fileNameLength;
				}
			}
			std::vector<wchar_t> filename;
			filename.clear();
			filename.resize(maxLength);
			for (UINT i = 0; i < fileCount; i++)
			{
				DragQueryFileW(hDrop, i, filename.data(), maxLength);

				event_out.dragAndDrop.filenames.push_back(utf16_to_utf8(filename.data()));
			}
			DragFinish(hDrop);
			eventQueue.push(event_out);
			break;
		}

		// Disable MessageBeep on Invalid Syskeypress
		case WM_MENUCHAR:
			return MNC_CLOSE << 16;

		case WM_PAINT:
			// Use the modal loop callback if there is a modal operation active
			if (isLoaded)
			{
				if (!insideModalLoopCallback && callbackModalLoop && (window.activeResizing || window.activeMenuLoop))
				{
					insideModalLoopCallback = true;
					callbackModalLoop();
					insideModalLoopCallback = false;
					return 0;
				}
			}
			break;

		case WM_ERASEBKGND:
			// Paint the window background black to prevent white flash after the window has just been created.
			// The background should not be painted here if window is being dragged/moved, as that causes flickering.
			if (!window.activeResizing)
			{
				RECT rc;
				GetClientRect(window.hwnd, &rc);
				FillRect((HDC)wParam, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
				return 0;
			}
			break;

		case WM_KILLFOCUS:
			// When alt-tabbing away in fullscreen mode, hide
			if (displayMode == DisplayMode::BorderlessFullscreen)
			{
				ShowWindow(window.hwnd, SW_MINIMIZE);
			}
			event_out.type = EventType::LostFocus;
			eventQueue.push(event_out);
			break;

		case WM_SETFOCUS:
			// When alt-tabbing toward this window in fullscreen mode, show
			event_out.type = EventType::GainedFocus;
			eventQueue.push(event_out);
			break;

		case WM_CLOSE:
			event_out.type = EventType::CloseRequest;
			eventQueue.push(event_out);
			return 0;

		case WM_SETCURSOR:
			// the HIWORD(lParam) tells what the cursor is doing.
			// the LOWORD(lParam) tells where the cursor is. 1 = inside the window
			// If the cursor is inside the window, then use custom cursor
			if (LOWORD(lParam) == 1 && window.cursor != NULL)
			{
				//TODO: Implement support for custom cursors at some point
				SetCursor(window.cursor);
				return true;
			}
			break;

		case WM_ENTERSIZEMOVE:
			window.activeResizing = true;
			RedrawWindow(window.hwnd, NULL, NULL, RDW_INVALIDATE);
			break;

		case WM_ENTERMENULOOP:
			window.activeMenuLoop = true;
			RedrawWindow(window.hwnd, NULL, NULL, RDW_INVALIDATE);
			break;

		case WM_EXITSIZEMOVE:
			window.activeResizing = false;
			if (displayMode == DisplayMode::Windowed)
			{
				RECT rect = {};
				if (GetWindowRect(window.hwnd, &rect))
				{
					windowedMode.pos = IntPoint(rect.left, rect.top);
					if (windowedMode.pos.x < 0) windowedMode.pos.x = 0;
					if (windowedMode.pos.y < 0) windowedMode.pos.y = 0;
				}
				RECT clientArea = {};
				GetClientRect(window.hwnd, &clientArea);
				Extent2D resize = Extent2D(clientArea.right - clientArea.left, clientArea.bottom - clientArea.top);
				if (resize != swapChain.extent && resize.width != 0 && resize.height != 0)
				{
					windowedMode.size = resize;
#ifdef IGLO_D3D12
					ResizeD3D12SwapChain(resize);
#endif
				}
			}
			break;

		case WM_EXITMENULOOP:
			window.activeMenuLoop = false;
			break;

		case WM_SIZE:
			// Restored, minimized and maximized messages should never be ignored, because we rely on these messages to
			// know when to send Minimized and Restored events to the iglo app.
			if (wParam == SIZE_MAXIMIZED ||
				wParam == SIZE_RESTORED ||
				wParam == SIZE_MINIMIZED)
			{
				bool oldMinimizedState = window.minimized;
				window.minimized = (wParam == SIZE_MINIMIZED);

				if (oldMinimizedState != window.minimized)
				{
					event_out.type = window.minimized ? EventType::Minimized : EventType::Restored;
					eventQueue.push(event_out);
				}
			}
			if (!window.activeResizing && !window.ignoreSizeMsg)
			{
				Extent2D resize = Extent2D(LOWORD(lParam), HIWORD(lParam));

				if (resize != swapChain.extent && resize.width != 0 && resize.height != 0)
				{
					if (displayMode == DisplayMode::Windowed) windowedMode.size = resize;
#ifdef IGLO_D3D12
					ResizeD3D12SwapChain(resize);
#endif
				}
			}
			break;

		case WM_GETMINMAXINFO:
		{
			if (displayMode != DisplayMode::Windowed) break;
			if (window.ignoreSizeMsg) break;

			// Get the window style and extended style
			DWORD style = GetWindowLong(window.hwnd, GWL_STYLE);
			DWORD exStyle = GetWindowLong(window.hwnd, GWL_EXSTYLE);

			// Calculate the required window size for a given client size
			RECT rc = { 0, 0, (LONG)windowResizeConstraints.minSize.width, (LONG)windowResizeConstraints.minSize.height };
			AdjustWindowRectEx(&rc, style, FALSE, exStyle);

			// Set minimum window size
			if (windowResizeConstraints.minSize.width != 0) ((MINMAXINFO*)lParam)->ptMinTrackSize.x = rc.right - rc.left;
			if (windowResizeConstraints.minSize.height != 0) ((MINMAXINFO*)lParam)->ptMinTrackSize.y = rc.bottom - rc.top;

			if (windowResizeConstraints.maxSize.width != 0 || windowResizeConstraints.maxSize.height != 0)
			{
				rc = { 0, 0, (LONG)windowResizeConstraints.maxSize.width, (LONG)windowResizeConstraints.maxSize.height };
				AdjustWindowRectEx(&rc, style, FALSE, exStyle);

				// Set maximum window size
				if (windowResizeConstraints.maxSize.width != 0) ((MINMAXINFO*)lParam)->ptMaxTrackSize.x = rc.right - rc.left;
				if (windowResizeConstraints.maxSize.height != 0) ((MINMAXINFO*)lParam)->ptMaxTrackSize.y = rc.bottom - rc.top;
			}
			break;
		}

		case WM_MOUSEMOVE:
			event_out.type = EventType::MouseMove;
			event_out.mouse.x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
			event_out.mouse.y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
			event_out.mouse.button = MouseButton::None;
			event_out.mouse.scrollWheel = 0;
			mousePosition.x = event_out.mouse.x;
			mousePosition.y = event_out.mouse.y;
			eventQueue.push(event_out);
			break;

		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONDBLCLK:
			event_out.type = EventType::MouseButtonDown;
			event_out.mouse.x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
			event_out.mouse.y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
			event_out.mouse.scrollWheel = 0;
			if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK)
			{
				event_out.mouse.button = MouseButton::Right;
			}
			else if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK)
			{
				event_out.mouse.button = MouseButton::Left;
			}
			else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK)
			{
				event_out.mouse.button = MouseButton::Middle;
			}
			else if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK)
			{
				event_out.mouse.button = MouseButton::XButton1;
				if (HIWORD(wParam) == 1) event_out.mouse.button = MouseButton::XButton1;
				if (HIWORD(wParam) == 2) event_out.mouse.button = MouseButton::XButton2;
			}
			else
			{
				event_out.mouse.button = MouseButton::Left; // Unknown button. Defaulting to left button.
			}
			//bool control = ((wParam & MK_CONTROL) == MK_CONTROL);
			//bool shift = ((wParam & MK_SHIFT) == MK_SHIFT);

			CaptureMouse_Win32(window, true);
			mouseButtonIsDown.SetTrue((uint64_t)event_out.mouse.button);
			mousePosition.x = event_out.mouse.x;
			mousePosition.y = event_out.mouse.y;
			eventQueue.push(event_out);
			break;

		case WM_RBUTTONUP:
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
			event_out.type = EventType::MouseButtonUp;
			event_out.mouse.x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
			event_out.mouse.y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
			event_out.mouse.scrollWheel = 0;
			if (msg == WM_RBUTTONUP)event_out.mouse.button = MouseButton::Right;
			else if (msg == WM_LBUTTONUP)event_out.mouse.button = MouseButton::Left;
			else if (msg == WM_MBUTTONUP)event_out.mouse.button = MouseButton::Middle;
			else if (msg == WM_XBUTTONUP)
			{
				event_out.mouse.button = MouseButton::XButton1;
				if (HIWORD(wParam) == 1) event_out.mouse.button = MouseButton::XButton1;
				if (HIWORD(wParam) == 2) event_out.mouse.button = MouseButton::XButton2;
			}
			else
			{
				event_out.mouse.button = MouseButton::Left; // Unknown button. Defaulting to left button.
			}

			CaptureMouse_Win32(window, false);
			mouseButtonIsDown.SetFalse((uint64_t)event_out.mouse.button);
			mousePosition.x = event_out.mouse.x;
			mousePosition.y = event_out.mouse.y;
			eventQueue.push(event_out);
			break;

		case WM_MOUSEWHEEL:
			if ((int)wParam != 0)
			{
				event_out.type = EventType::MouseWheel;
				event_out.mouse.scrollWheel = GET_WHEEL_DELTA_WPARAM(wParam);
				// WM_MOUSEWHEEL gives incorrect mouse coordinates, so use previous mouse coords instead
				event_out.mouse.x = mousePosition.x;
				event_out.mouse.y = mousePosition.y;
				event_out.mouse.button = MouseButton::None;
				eventQueue.push(event_out);
			}
			break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			event_out.key = Key::Unknown;
			UINT scancode = (lParam & 0x00ff0000) >> 16;
			int extended = (lParam & 0x01000000) != 0;
			switch (wParam)
			{
			case 'A': event_out.key = Key::A; break;
			case 'B': event_out.key = Key::B; break;
			case 'C': event_out.key = Key::C; break;
			case 'D': event_out.key = Key::D; break;
			case 'E': event_out.key = Key::E; break;
			case 'F': event_out.key = Key::F; break;
			case 'G': event_out.key = Key::G; break;
			case 'H': event_out.key = Key::H; break;
			case 'I': event_out.key = Key::I; break;
			case 'J': event_out.key = Key::J; break;
			case 'K': event_out.key = Key::K; break;
			case 'L': event_out.key = Key::L; break;
			case 'M': event_out.key = Key::M; break;
			case 'N': event_out.key = Key::N; break;
			case 'O': event_out.key = Key::O; break;
			case 'P': event_out.key = Key::P; break;
			case 'Q': event_out.key = Key::Q; break;
			case 'R': event_out.key = Key::R; break;
			case 'S': event_out.key = Key::S; break;
			case 'T': event_out.key = Key::T; break;
			case 'U': event_out.key = Key::U; break;
			case 'V': event_out.key = Key::V; break;
			case 'W': event_out.key = Key::W; break;
			case 'X': event_out.key = Key::X; break;
			case 'Y': event_out.key = Key::Y; break;
			case 'Z': event_out.key = Key::Z; break;
			case '0': event_out.key = Key::Num0; break;
			case '1': event_out.key = Key::Num1; break;
			case '2': event_out.key = Key::Num2; break;
			case '3': event_out.key = Key::Num3; break;
			case '4': event_out.key = Key::Num4; break;
			case '5': event_out.key = Key::Num5; break;
			case '6': event_out.key = Key::Num6; break;
			case '7': event_out.key = Key::Num7; break;
			case '8': event_out.key = Key::Num8; break;
			case '9': event_out.key = Key::Num9; break;
			case VK_NUMPAD0: event_out.key = Key::Numpad0; break;
			case VK_NUMPAD1: event_out.key = Key::Numpad1; break;
			case VK_NUMPAD2: event_out.key = Key::Numpad2; break;
			case VK_NUMPAD3: event_out.key = Key::Numpad3; break;
			case VK_NUMPAD4: event_out.key = Key::Numpad4; break;
			case VK_NUMPAD5: event_out.key = Key::Numpad5; break;
			case VK_NUMPAD6: event_out.key = Key::Numpad6; break;
			case VK_NUMPAD7: event_out.key = Key::Numpad7; break;
			case VK_NUMPAD8: event_out.key = Key::Numpad8; break;
			case VK_NUMPAD9: event_out.key = Key::Numpad9; break;
			case VK_F1: event_out.key = Key::F1; break;
			case VK_F2: event_out.key = Key::F2; break;
			case VK_F3: event_out.key = Key::F3; break;
			case VK_F4: event_out.key = Key::F4; break;
			case VK_F5: event_out.key = Key::F5; break;
			case VK_F6: event_out.key = Key::F6; break;
			case VK_F7: event_out.key = Key::F7; break;
			case VK_F8: event_out.key = Key::F8; break;
			case VK_F9: event_out.key = Key::F9; break;
			case VK_F10: event_out.key = Key::F10; break;
			case VK_F11: event_out.key = Key::F11; break;
			case VK_F12: event_out.key = Key::F12; break;
			case VK_F13: event_out.key = Key::F13; break;
			case VK_F14: event_out.key = Key::F14; break;
			case VK_F15: event_out.key = Key::F15; break;
			case VK_F16: event_out.key = Key::F16; break;
			case VK_F17: event_out.key = Key::F17; break;
			case VK_F18: event_out.key = Key::F18; break;
			case VK_F19: event_out.key = Key::F19; break;
			case VK_F20: event_out.key = Key::F20; break;
			case VK_F21: event_out.key = Key::F21; break;
			case VK_F22: event_out.key = Key::F22; break;
			case VK_F23: event_out.key = Key::F23; break;
			case VK_F24: event_out.key = Key::F24; break;
			case VK_CONTROL:
				event_out.key = Key::LeftControl;
				if (extended) event_out.key = Key::RightControl;
				break;
			case VK_SHIFT:
				if (MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) event_out.key = Key::LeftShift;
				else event_out.key = Key::RightShift;
				break;
			case VK_MENU:
				event_out.key = Key::LeftAlt;
				if (extended) event_out.key = Key::RightAlt;
				break;
			case VK_LWIN: event_out.key = Key::LeftSystem; break;
			case VK_RWIN: event_out.key = Key::RightSystem; break;
			case VK_PRIOR: event_out.key = Key::PageUp; break;
			case VK_NEXT: event_out.key = Key::PageDown; break;
			case VK_END: event_out.key = Key::End; break;
			case VK_HOME: event_out.key = Key::Home; break;
			case VK_ESCAPE: event_out.key = Key::Escape; break;
			case VK_SPACE: event_out.key = Key::Space; break;
			case VK_CAPITAL: event_out.key = Key::CapsLock; break;
			case VK_PAUSE: event_out.key = Key::Pause; break;
			case VK_RETURN: event_out.key = Key::Enter; break;
			case VK_CLEAR: event_out.key = Key::Clear; break;
			case VK_BACK: event_out.key = Key::Backspace; break;
			case VK_TAB: event_out.key = Key::Tab; break;
			case VK_INSERT: event_out.key = Key::Insert; break;
			case VK_DELETE: event_out.key = Key::Delete; break;
			case VK_SNAPSHOT: event_out.key = Key::PrintScreen; break;
			case VK_LEFT: event_out.key = Key::Left; break;
			case VK_UP: event_out.key = Key::Up; break;
			case VK_RIGHT: event_out.key = Key::Right; break;
			case VK_DOWN: event_out.key = Key::Down; break;
			case VK_SLEEP: event_out.key = Key::Sleep; break;
			case VK_MULTIPLY: event_out.key = Key::Multiply; break;
			case VK_ADD: event_out.key = Key::Add; break;
			case VK_SEPARATOR: event_out.key = Key::Seperator; break;
			case VK_SUBTRACT: event_out.key = Key::Subtract; break;
			case VK_DECIMAL: event_out.key = Key::Decimal; break;
			case VK_DIVIDE: event_out.key = Key::Divide; break;
			case VK_NUMLOCK: event_out.key = Key::NumLock; break;
			case VK_SCROLL: event_out.key = Key::ScrollLock; break;
			case VK_APPS: event_out.key = Key::Apps; break;
			case VK_VOLUME_MUTE: event_out.key = Key::VolumeMute; break;
			case VK_VOLUME_DOWN: event_out.key = Key::VolumeDown; break;
			case VK_VOLUME_UP: event_out.key = Key::VolumeUp; break;
			case VK_MEDIA_NEXT_TRACK: event_out.key = Key::MediaNextTrack; break;
			case VK_MEDIA_PREV_TRACK: event_out.key = Key::MediaPrevTrack; break;
			case VK_MEDIA_STOP: event_out.key = Key::MediaStop; break;
			case VK_MEDIA_PLAY_PAUSE: event_out.key = Key::MediaPlayPause; break;
			case VK_OEM_PLUS: event_out.key = Key::Plus; break;
			case VK_OEM_COMMA: event_out.key = Key::Comma; break;
			case VK_OEM_MINUS: event_out.key = Key::Minus; break;
			case VK_OEM_PERIOD: event_out.key = Key::Period; break;
			case VK_OEM_1: event_out.key = Key::OEM_1; break;
			case VK_OEM_2: event_out.key = Key::OEM_2; break;
			case VK_OEM_3: event_out.key = Key::OEM_3; break;
			case VK_OEM_4: event_out.key = Key::OEM_4; break;
			case VK_OEM_5: event_out.key = Key::OEM_5; break;
			case VK_OEM_6: event_out.key = Key::OEM_6; break;
			case VK_OEM_7: event_out.key = Key::OEM_7; break;
			case VK_OEM_8: event_out.key = Key::OEM_8; break;
			case VK_OEM_102: event_out.key = Key::OEM_102; break;
			default:
				break;
			}

			if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN)
			{
				if (event_out.key != Key::Unknown && (uint64_t)event_out.key < keyIsDown.GetSize())
				{
					keyIsDown.SetTrue((uint64_t)event_out.key);
				}
				event_out.type = EventType::KeyPress;
				bool keyIsDownBefore = lParam & (1 << 30);
				if (!keyIsDownBefore) // If this key wasn't down before...
				{
					Event bonusKeyDownEvent = event_out;
					bonusKeyDownEvent.type = EventType::KeyDown;
					eventQueue.push(bonusKeyDownEvent); //...then create a KeyDown event
				}
			}
			else // WM_KEYUP or WM_SYSKEYUP
			{
				if (event_out.key != Key::Unknown && (uint64_t)event_out.key < keyIsDown.GetSize())
				{
					keyIsDown.SetFalse((uint64_t)event_out.key);
				}
				event_out.type = EventType::KeyUp;
			}
			eventQueue.push(event_out);
		}
		break;

		case WM_CHAR:
			event_out.type = EventType::TextEntered;
			event_out.textEntered.character_utf32 = (uint32_t)wParam;
			eventQueue.push(event_out);
			break;

		} // switch (msg)

		return DefWindowProc(window.hwnd, msg, wParam, lParam);
	}
}

#endif