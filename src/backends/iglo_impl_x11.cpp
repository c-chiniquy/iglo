

#ifdef __linux__

#include "iglo.h"

#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/XF86keysym.h>
#include <cstring>

namespace ig
{

	static int X11ErrorHandler(Display* display, XErrorEvent* e)
	{
		char buffer[256];
		XGetErrorText(display, e->error_code, buffer, sizeof(buffer));
		Print(ToString("X11 Error: ", buffer, "\n"));
		return 0;
	}

	void PopupMessage(const std::string& message, const std::string& caption, const IGLOContext* parent)
	{
		Display* display = XOpenDisplay(nullptr);
		if (!display) return;

		int screen = DefaultScreen(display);
		Window root = RootWindow(display, screen);

		// Load fixed-width font for text measurement
		XFontStruct* font = XLoadQueryFont(display, "fixed");
		if (!font) font = XLoadQueryFont(display, "9x15");
		if (!font) font = XLoadQueryFont(display, "6x13");

		int char_width = 8;  // Default character width if font loading fails
		int line_height = 15; // Default line height
		if (font)
		{
			char_width = font->max_bounds.width;
			line_height = font->ascent + font->descent;
		}

		// Word wrapping calculations
		const int width = 400;
		const int margin = 20;
		const int max_chars_per_line = (width - 2 * margin) / char_width;

		// Remove carriage returns
		std::string cleaned_message = message;
		cleaned_message.erase(std::remove(cleaned_message.begin(), cleaned_message.end(), '\r'), cleaned_message.end());

		// Split message into lines
		std::vector<std::string> lines;
		std::istringstream iss(cleaned_message);
		std::string paragraph;
		while (std::getline(iss, paragraph))
		{
			if (paragraph.empty())
			{
				lines.push_back(""); // Preserve empty lines from newlines
				continue;
			}

			// Word-wrap each paragraph
			size_t max_chars = static_cast<size_t>(max_chars_per_line);
			std::string current_line;
			bool has_words = false;

			// Process each word in the paragraph
			std::istringstream word_stream(paragraph);
			std::string word;
			while (word_stream >> word)
			{
				has_words = true;
				size_t space_needed = current_line.empty() ? 0 : 1;

				if (current_line.length() + word.length() + space_needed > max_chars)
				{
					if (current_line.empty())
					{
						// Handle very long word
						while (word.length() > max_chars)
						{
							lines.push_back(word.substr(0, max_chars));
							word = word.substr(max_chars);
						}
						current_line = word;
					}
					else
					{
						lines.push_back(current_line);
						current_line = word;
					}
				}
				else
				{
					if (!current_line.empty()) current_line += ' ';
					current_line += word;
				}
			}

			// Handle remaining content after processing words
			if (!current_line.empty())
			{
				lines.push_back(current_line);
			}
			else if (!has_words)
			{
				lines.push_back(""); // Paragraph was all whitespace
			}
		}

		// Calculate window height based on wrapped text
		const int button_height = 30;
		const int button_margin = 20;
		const int text_top_margin = 20;
		int height = text_top_margin + lines.size() * line_height + button_height + button_margin * 2;

		// Center window on screen
		int win_x = (DisplayWidth(display, screen) - width) / 2;
		int win_y = (DisplayHeight(display, screen) - height) / 2;

		// Create main window
		Window win = XCreateSimpleWindow(display, root, win_x, win_y, width, height, 1,
			BlackPixel(display, screen),
			WhitePixel(display, screen));
		XStoreName(display, win, caption.c_str());

		// Create color resources
		Colormap colormap = DefaultColormap(display, screen);
		XColor normal_color;
		XParseColor(display, colormap, "#DDDDDD", &normal_color);
		XAllocColor(display, colormap, &normal_color);
		XColor pressed_color;
		XParseColor(display, colormap, "#AAAAAA", &pressed_color);
		XAllocColor(display, colormap, &pressed_color);

		// Create "OK" button
		int button_width = 80;
		int button_x = (width - button_width) / 2;
		int button_y = height - button_height - button_margin;
		Window button = XCreateSimpleWindow(display, win, button_x, button_y,
			button_width, button_height, 1,
			BlackPixel(display, screen),
			normal_color.pixel);

		// Prevent window resizing
		XSizeHints hints;
		hints.flags = PMinSize | PMaxSize;
		hints.min_width = hints.max_width = width;
		hints.min_height = hints.max_height = height;
		XSetWMNormalHints(display, win, &hints);

		// Set event masks
		XSelectInput(display, win, ExposureMask | StructureNotifyMask);
		XSelectInput(display, button, ExposureMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);

		// Handle window close protocol
		Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
		XSetWMProtocols(display, win, &wm_delete_window, 1);

		XMapWindow(display, win);
		XMapWindow(display, button);

		GC gc = XCreateGC(display, win, 0, nullptr);
		XSetForeground(display, gc, BlackPixel(display, screen));

		if (font) XSetFont(display, gc, font->fid);

		bool button_pressed = false;
		bool window_exposed = false;

		auto DrawButtonText = [&]()
		{
			XDrawString(display, button, gc,
				(button_width / 2) - char_width,
				(button_height / 2) + (line_height / 2) - 1,
				"OK", 2);
		};

		XEvent ev;
		while (true)
		{
			XNextEvent(display, &ev);

			if (ev.type == Expose)
			{
				if (ev.xexpose.window == win && !window_exposed)
				{
					// Draw wrapped text lines
					int y = text_top_margin + (font ? font->ascent : 15);
					for (const auto& line : lines)
					{
						XDrawString(display, win, gc, margin, y, line.c_str(), line.length());
						y += line_height;
					}
					window_exposed = true;
				}
				else if (ev.xexpose.window == button)
				{
					DrawButtonText();
				}
			}
			else if (ev.type == ButtonPress)
			{
				if (ev.xbutton.window == button)
				{
					button_pressed = true;
					XSetWindowBackground(display, button, pressed_color.pixel);
					XClearWindow(display, button);
					DrawButtonText();
				}
			}
			else if (ev.type == ButtonRelease)
			{
				if (ev.xbutton.window == button && button_pressed)
				{
					break; // Close when button is released after press
				}
				if (button_pressed)
				{
					button_pressed = false;
					XSetWindowBackground(display, button, normal_color.pixel);
					XClearWindow(display, button);
					DrawButtonText();
				}
			}
			else if (ev.type == LeaveNotify && ev.xcrossing.window == button)
			{
				if (button_pressed)
				{
					button_pressed = false;
					XSetWindowBackground(display, button, normal_color.pixel);
					XClearWindow(display, button);
					DrawButtonText();
				}
			}
			else if (ev.type == EnterNotify && ev.xcrossing.window == button)
			{
				if (ev.xcrossing.state & Button1Mask)
				{
					button_pressed = true;
					XSetWindowBackground(display, button, pressed_color.pixel);
					XClearWindow(display, button);
					DrawButtonText();
				}
			}
			else if (ev.type == ClientMessage)
			{
				if (ev.xclient.data.l[0] == (long)wm_delete_window)
				{
					break; // Handle window manager close
				}
			}
			else if (ev.type == DestroyNotify)
			{
				break; // Handle destruction
			}
		}

		if (font) XFreeFont(display, font);
		XDestroyWindow(display, win);
		XFreeGC(display, gc);
		XCloseDisplay(display);
	}

	void SetXDecorationsVisible(Display* display, Window window, bool enable)
	{
		Atom prop = XInternAtom(display, "_MOTIF_WM_HINTS", False);

		struct MotifWmHints
		{
			unsigned long flags;
			unsigned long functions;
			unsigned long decorations;
			long input_mode;
			unsigned long status;
		};

		MotifWmHints hints = {};
		hints.flags = (1L << 1); // MWM_HINTS_DECORATIONS
		hints.decorations = enable ? 1 : 0;

		XChangeProperty(display, window, prop, prop, 32, PropModeReplace, (unsigned char*)&hints, 5);
		XFlush(display);
	}

	void SetXResizable(Display* display, Window window, bool resizable, Extent2D resizableExtent)
	{
		// Now set size hints to restrict resizing
		XSizeHints* size_hints = XAllocSizeHints();
		if (size_hints)
		{
			if (resizable)
			{
				size_hints->flags = PSize; // Initial size only
			}
			else
			{
				int width = resizableExtent.width;
				int height = resizableExtent.height;

				size_hints->flags = PMinSize | PMaxSize;
				size_hints->min_width = size_hints->max_width = width;
				size_hints->min_height = size_hints->max_height = height;
			}

			XSetWMNormalHints(display, window, size_hints);
			XFree(size_hints);
		}

		XFlush(display);
	}

	void SetXFullscreen(Display* display, Window window, bool fullscreen)
	{
		Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
		Atom fsAtom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

		XEvent xev = {};
		xev.type = ClientMessage;
		xev.xclient.window = window;
		xev.xclient.message_type = wmState;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = fullscreen ? 1 : 0;  // _NET_WM_STATE_ADD : REMOVE
		xev.xclient.data.l[1] = fsAtom;
		xev.xclient.data.l[2] = 0;

		XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
	}

	void SetXWindowIcon(Display* display, Window window, const uint32_t* iconDataWithSize, uint32_t dataLength)
	{
		const char* errStr = "Failed to set X window icon. Reason: ";

		Atom netWmIcon = XInternAtom(display, "_NET_WM_ICON", False);
		Atom cardinal = XInternAtom(display, "CARDINAL", False);

		if (netWmIcon == None_ || cardinal == None_)
		{
			Log(LogType::Error, ToString(errStr, "Failed to get required atoms."));
			return;
		}

		int format = 32; // 32 bits per pixel
		Status status = XChangeProperty(display, window, netWmIcon, cardinal, format, PropModeReplace,
			(const unsigned char*)iconDataWithSize, dataLength);
		if (status == 0)
		{
			Log(LogType::Error, ToString(errStr, "XChangeProperty failed."));
		}
	}

	bool IsXWindowMinimized(Display* display, Window window)
	{
		Atom wmState = XInternAtom(display, "_NET_WM_STATE", True);
		Atom hiddenAtom = XInternAtom(display, "_NET_WM_STATE_HIDDEN", True);

		Atom actualType;
		int actualFormat;
		unsigned long nitems, bytesAfter;
		unsigned char* prop = nullptr;

		int status = XGetWindowProperty(display, window, wmState, 0, (~0L), False, XA_ATOM,
			&actualType, &actualFormat, &nitems, &bytesAfter, &prop);

		bool minimized = false;

		if (status == Success && prop)
		{
			Atom* atoms = (Atom*)prop;
			for (unsigned long i = 0; i < nitems; i++)
			{
				if (atoms[i] == hiddenAtom)
				{
					minimized = true;
					break;
				}
			}
			XFree(prop);
		}

		return minimized;
	}

	void SetXWindowTitle(Display* display, Window window, const std::string& title)
	{
		// Set UTF-8 title
		Atom utf8_string = XInternAtom(display, "UTF8_STRING", False);
		Atom net_wm_name = XInternAtom(display, "_NET_WM_NAME", False);

		XChangeProperty(display, window, net_wm_name, utf8_string, 8, PropModeReplace,
			(const unsigned char*)title.c_str(), title.length());
		XFlush(display);
	}

	void IGLOContext::Impl_SetWindowVisible(bool visible)
	{
		if (visible) XMapWindow(window.display, window.handle);
		else XUnmapWindow(window.display, window.handle);
	}
	void IGLOContext::Impl_SetWindowSize(uint32_t width, uint32_t height)
	{
		XMoveResizeWindow(window.display, window.handle, windowedMode.pos.x, windowedMode.pos.y, width, height);
	}
	void IGLOContext::Impl_SetWindowPosition(int32_t x, int32_t y)
	{
		XFlush(window.display);
		XMoveWindow(window.display, window.handle, x, y);
		XFlush(window.display);
	}
	void IGLOContext::Impl_SetWindowResizable(bool resizable)
	{
		SetXResizable(window.display, window.handle, resizable, windowedMode.size);
	}
	void IGLOContext::Impl_SetWindowBordersVisible(bool visible)
	{
		SetXDecorationsVisible(window.display, window.handle, visible);
	}

	void IGLOContext::Impl_SetWindowIconFromImage_BGRA(uint32_t width, uint32_t height, const uint32_t* iconPixels)
	{
		// Total length: 2 (width + height) + pixels
		uint32_t dataLength = 2 + (width * height);

		std::vector<uint32_t> iconData(dataLength);
		iconData[0] = width;
		iconData[1] = height;

		std::copy(iconPixels, iconPixels + (width * height), iconData.begin() + 2);

		SetXWindowIcon(window.display, window.handle, iconData.data(), dataLength);
	}

	void IGLOContext::SetWindowTitle(std::string title)
	{
		window.title = title;
		SetXWindowTitle(window.display, window.handle, title);
	}

	void IGLOContext::CenterWindow()
	{
		Extent2D res = GetActiveMonitorScreenResolution();
		int32_t x = (int32_t(res.width) / 2) - (windowedMode.size.width / 2);
		int32_t y = (int32_t(res.height) / 2) - (windowedMode.size.height / 2);
		SetWindowPosition(x, y);
	}

	Extent2D IGLOContext::GetActiveMonitorScreenResolution()
	{
		Extent2D out = {};
		if (window.screen)
		{
			out.width = window.screen->width;
			out.height = window.screen->height;
		}
		return out;
	}

	bool IGLOContext::GetEvent(Event& out_event)
	{
		if (window.display)
		{
			while (XPending(window.display) > 0)
			{
				XEvent e;
				XNextEvent(window.display, &e);
				ProcessX11Event(e);
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
			if (window.display)
			{
				fd_set fileDescSet;
				timeval duration;
				FD_ZERO(&fileDescSet);
				FD_SET(window.x11fileDescriptor, &fileDescSet);
				duration.tv_usec = timeoutMilliseconds * 1000; // Timer microseconds
				duration.tv_sec = 0; // Timer seconds

				// Waits until either the timer fires or an event is pending, whichever comes first.
				select(window.x11fileDescriptor + 1, &fileDescSet, NULL, NULL, &duration);

				while (XPending(window.display) > 0)
				{
					XEvent e;
					XNextEvent(window.display, &e);
					ProcessX11Event(e);
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

	void IGLOContext::EnterWindowedMode(uint32_t width, uint32_t height)
	{
		displayMode = DisplayMode::Windowed;
		windowedMode.size = Extent2D(width, height);

		SetXFullscreen(window.display, window.handle, false);
		SetXResizable(window.display, window.handle, windowedMode.resizable, windowedMode.size);
		SetXDecorationsVisible(window.display, window.handle, windowedMode.bordersVisible);
		XMoveResizeWindow(window.display, window.handle, windowedMode.pos.x, windowedMode.pos.y, width, height);

		if (windowedMode.visible) XMapWindow(window.display, window.handle);
		else XUnmapWindow(window.display, window.handle);

		XFlush(window.display);
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

		SetXResizable(window.display, window.handle, true, Extent2D());
		SetXDecorationsVisible(window.display, window.handle, false);
		XMoveResizeWindow(window.display, window.handle, 0, 0, res.width, res.height);
		XMapRaised(window.display, window.handle); // Ensure the window is raised and focused
		XFlush(window.display);

		// Ignore all Focus events until the transition is complete
		window.fullscreenCompleteTimer.Reset();
	}

	void IGLOContext::SetX11EventHookCallback(CallbackX11EventHook callback)
	{
		window.callbackX11EventHook = callback;
	}

	void IGLOContext::PrepareWindowPostGraphics(const WindowSettings& windowSettings)
	{
		if (windowedMode.visible) XMapRaised(window.display, window.handle);

		if (windowSettings.centered)
		{
			CenterWindow();
		}
	}

	void IGLOContext::Impl_UnloadWindow()
	{
		// Cleanup X11
		if (window.display)
		{
			if (window.xic) XDestroyIC(window.xic);
			if (window.xim) XCloseIM(window.xim);
			if (window.handle)
			{
				XFreeColormap(window.display, window.attributes.colormap);
				XDestroyWindow(window.display, window.handle);
			}
			XCloseDisplay(window.display);
		}

		window = Impl_WindowData();
	}

	DetailedResult IGLOContext::Impl_LoadWindow(const WindowSettings& windowSettings)
	{
		// X11 window creation code from public domain source:
		// https://github.com/gamedevtech/X11OpenGLWindow

		window.display = XOpenDisplay(0);
		if (!window.display)
		{
			return DetailedResult::MakeFail("Failed to open display.");
		}
		window.screen = DefaultScreenOfDisplay(window.display);
		window.screenId = DefaultScreen(window.display);
		window.x11fileDescriptor = ConnectionNumber(window.display);

		XSetErrorHandler(X11ErrorHandler);

		int depth = 24;
		int visualClass = TrueColor;

		if (!XMatchVisualInfo(window.display, window.screenId, depth, visualClass, &window.visualInfo))
		{
			return DetailedResult::MakeFail("No matching visual found.");
		}

		if (window.screenId != window.visualInfo.screen)
		{
			return DetailedResult::MakeFail("screenId does not match visualInfo.");
		}

		// Open the window
		window.attributes.border_pixel = BlackPixel(window.display, window.screenId);
		window.attributes.background_pixel = BlackPixel(window.display, window.screenId);
		window.attributes.override_redirect = True;
		window.attributes.colormap = XCreateColormap(window.display,
			RootWindow(window.display, window.screenId), window.visualInfo.visual, AllocNone);
		window.attributes.event_mask = ExposureMask;
		window.handle = XCreateWindow(window.display, RootWindow(window.display, window.screenId),
			0, 0, windowedMode.size.width, windowedMode.size.height, 0, window.visualInfo.depth, InputOutput,
			window.visualInfo.visual, CWBackPixel | CWColormap | CWBorderPixel | CWEventMask, &window.attributes);

		// Redirect Close
		window.atomWmDeleteWindow = XInternAtom(window.display, "WM_DELETE_WINDOW", False);
		XSetWMProtocols(window.display, window.handle, &window.atomWmDeleteWindow, 1);

		XSelectInput(window.display, window.handle,
			KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
			EnterWindowMask | LeaveWindowMask | PointerMotionMask | 0 |
			Button1MotionMask | Button2MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask | ButtonMotionMask |
			KeymapStateMask | ExposureMask | VisibilityChangeMask | StructureNotifyMask | 0 | 0 | SubstructureRedirectMask |
			FocusChangeMask | PropertyChangeMask | ColormapChangeMask | OwnerGrabButtonMask);

		// Disable fake key release events
		int autoRepeatIsSupported = 0;
		if (!XkbSetDetectableAutoRepeat(window.display, 1, &autoRepeatIsSupported))
		{
			Log(LogType::Warning, "Unable to disable X11 auto repeat for keys being held down.");
		}

		window.xim = XOpenIM(window.display, 0, 0, 0);
		if (!window.xim)
		{
			return DetailedResult::MakeFail("Failed to open input method.");
		}

		XIMStyles* ximStyles = nullptr;
		if (XGetIMValues(window.xim, XNQueryInputStyle, &ximStyles, NULL) || ximStyles == nullptr)
		{
			return DetailedResult::MakeFail("Failed to get input styles.");
		}

		XIMStyle bestStyle = 0;
		for (int i = 0; i < ximStyles->count_styles; i++)
		{
			XIMStyle style = ximStyles->supported_styles[i];
			if (style == (XIMPreeditNothing | XIMStatusNothing))
			{
				bestStyle = style;
				break;
			}
		}
		XFree(ximStyles);

		if (!bestStyle)
		{
			return DetailedResult::MakeFail("Failed to find matching input style.");
		}

		window.xic = XCreateIC(window.xim, XNInputStyle, bestStyle,
			XNClientWindow, window.handle,
			XNFocusWindow, window.handle, NULL);
		if (!window.xic)
		{
			return DetailedResult::MakeFail("Failed to create input context.");
		}

		SetXWindowTitle(window.display, window.handle, window.title);
		SetXResizable(window.display, window.handle, windowedMode.resizable, windowedMode.size);
		SetXDecorationsVisible(window.display, window.handle, windowedMode.bordersVisible);
		XClearWindow(window.display, window.handle);

		return DetailedResult::MakeSuccess();
	}

	void IGLOContext::ProcessX11Event(XEvent e)
	{
		if (window.callbackX11EventHook)
		{
			if (window.callbackX11EventHook(window.display, window.handle, e)) return;
		}

		Event event_out;

		switch (e.type)
		{
		case PropertyNotify:
		{
			bool oldMinimizedState = window.minimized;
			window.minimized = IsXWindowMinimized(window.display, window.handle);
			if (oldMinimizedState != window.minimized)
			{
				event_out.type = window.minimized ? EventType::Minimized : EventType::Restored;
				eventQueue.push(event_out);
				break;
			}
		}
		break;

		case FocusIn:
			event_out.type = EventType::GainedFocus;
			eventQueue.push(event_out);
			break;

		case FocusOut:
			event_out.type = EventType::LostFocus;
			eventQueue.push(event_out);

			if (displayMode == DisplayMode::BorderlessFullscreen)
			{
				if (window.fullscreenCompleteTimer.GetSeconds() > 1.0)
				{
					// Minimize if lost focus
					XIconifyWindow(window.display, window.handle, window.screenId);
				}
			}
			break;

		case ConfigureNotify:
		{
			XConfigureEvent xce = e.xconfigure;
			if (displayMode == DisplayMode::Windowed)
			{
				// Recording the window position is too buggy, leave this commented out
				//windowedMode.pos.x = xce.x;
				//windowedMode.pos.y = xce.y;

				windowedMode.size = Extent2D(xce.width, xce.height);
			}
			if (windowedMode.visible)
			{
				XMapWindow(window.display, window.handle);
			}
			else
			{
				XUnmapWindow(window.display, window.handle);
			}
		}
		break;

		case ButtonPress:
		{
			MouseButton button = MouseButton::Left; // Unknown button. Defaulting to left button.
			if (e.xbutton.button == 1) button = MouseButton::Left;
			else if (e.xbutton.button == 2) button = MouseButton::Middle;
			else if (e.xbutton.button == 3) button = MouseButton::Right;
			else if (e.xbutton.button == 8) button = MouseButton::XButton1;
			else if (e.xbutton.button == 9) button = MouseButton::XButton2;
			else if (e.xbutton.button == 4)
			{
				event_out.type = EventType::MouseWheel;
				event_out.mouse.scrollWheel = 1; // Scroll up
				event_out.mouse.button = MouseButton::None;
				event_out.mouse.x = mousePosition.x;
				event_out.mouse.y = mousePosition.y;
				eventQueue.push(event_out);
				break;
			}
			else if (e.xbutton.button == 5)
			{
				event_out.type = EventType::MouseWheel;
				event_out.mouse.scrollWheel = -1; // Scroll down
				event_out.mouse.button = MouseButton::None;
				event_out.mouse.x = mousePosition.x;
				event_out.mouse.y = mousePosition.y;
				eventQueue.push(event_out);
				break;
			}
			mouseButtonIsDown.SetTrue((uint64_t)button);
			mousePosition.x = e.xbutton.x;
			mousePosition.y = e.xbutton.y;
			event_out.type = EventType::MouseButtonDown;
			event_out.mouse.button = button;
			event_out.mouse.x = mousePosition.x;
			event_out.mouse.y = mousePosition.y;
			event_out.mouse.scrollWheel = 0;
			eventQueue.push(event_out);
		}
		break;

		case ButtonRelease:
		{
			// Ignore bogus button release for mouse wheel events
			if (e.xbutton.button == 4 || e.xbutton.button == 5) break;

			MouseButton button = MouseButton::Left; // Unknown button. Defaulting to left button.
			if (e.xbutton.button == 1) button = MouseButton::Left;
			else if (e.xbutton.button == 2) button = MouseButton::Middle;
			else if (e.xbutton.button == 3) button = MouseButton::Right;
			else if (e.xbutton.button == 8) button = MouseButton::XButton1;
			else if (e.xbutton.button == 9) button = MouseButton::XButton2;
			mouseButtonIsDown.SetFalse((uint64_t)button);
			mousePosition.x = e.xbutton.x;
			mousePosition.y = e.xbutton.y;
			event_out.type = EventType::MouseButtonUp;
			event_out.mouse.button = button;
			event_out.mouse.x = mousePosition.x;
			event_out.mouse.y = mousePosition.y;
			event_out.mouse.scrollWheel = 0;
			eventQueue.push(event_out);
		}
		break;

		case MotionNotify:
			mousePosition.x = e.xmotion.x;
			mousePosition.y = e.xmotion.y;
			event_out.type = EventType::MouseMove;
			event_out.mouse.x = e.xmotion.x;
			event_out.mouse.y = e.xmotion.y;
			event_out.mouse.button = MouseButton::None;
			event_out.mouse.scrollWheel = 0;
			eventQueue.push(event_out);
			break;

		case KeymapNotify:
			XRefreshKeyboardMapping(&e.xmapping);
			break;

		case KeyPress_:
		case KeyRelease:
		{

			switch (e.xkey.keycode)
			{
			default:
				event_out.key = Key::Unknown;
				break;

			case 38: event_out.key = Key::A; break;
			case 56: event_out.key = Key::B; break;
			case 54: event_out.key = Key::C; break;
			case 40: event_out.key = Key::D; break;
			case 26: event_out.key = Key::E; break;
			case 41: event_out.key = Key::F; break;
			case 42: event_out.key = Key::G; break;
			case 43: event_out.key = Key::H; break;
			case 31: event_out.key = Key::I; break;
			case 44: event_out.key = Key::J; break;
			case 45: event_out.key = Key::K; break;
			case 46: event_out.key = Key::L; break;
			case 58: event_out.key = Key::M; break;
			case 57: event_out.key = Key::N; break;
			case 32: event_out.key = Key::O; break;
			case 33: event_out.key = Key::P; break;
			case 24: event_out.key = Key::Q; break;
			case 27: event_out.key = Key::R; break;
			case 39: event_out.key = Key::S; break;
			case 28: event_out.key = Key::T; break;
			case 30: event_out.key = Key::U; break;
			case 55: event_out.key = Key::V; break;
			case 25: event_out.key = Key::W; break;
			case 53: event_out.key = Key::X; break;
			case 29: event_out.key = Key::Y; break;
			case 52: event_out.key = Key::Z; break;

			case 10: event_out.key = Key::Num1; break;
			case 11: event_out.key = Key::Num2; break;
			case 12: event_out.key = Key::Num3; break;
			case 13: event_out.key = Key::Num4; break;
			case 14: event_out.key = Key::Num5; break;
			case 15: event_out.key = Key::Num6; break;
			case 16: event_out.key = Key::Num7; break;
			case 17: event_out.key = Key::Num8; break;
			case 18: event_out.key = Key::Num9; break;
			case 19: event_out.key = Key::Num0; break;

			case 67: event_out.key = Key::F1; break;
			case 68: event_out.key = Key::F2; break;
			case 69: event_out.key = Key::F3; break;
			case 70: event_out.key = Key::F4; break;
			case 71: event_out.key = Key::F5; break;
			case 72: event_out.key = Key::F6; break;
			case 73: event_out.key = Key::F7; break;
			case 74: event_out.key = Key::F8; break;
			case 75: event_out.key = Key::F9; break;
			case 76: event_out.key = Key::F10; break;
			case 95: event_out.key = Key::F11; break;
			case 96: event_out.key = Key::F12; break;

			case 113: event_out.key = Key::Left; break;
			case 114: event_out.key = Key::Right; break;
			case 111: event_out.key = Key::Up; break;
			case 116: event_out.key = Key::Down; break;

			case 37: event_out.key = Key::LeftControl; break;
			case 105: event_out.key = Key::RightControl; break;
			case 50: event_out.key = Key::LeftShift; break;
			case 62: event_out.key = Key::RightShift; break;
			case 64: event_out.key = Key::LeftAlt; break;
			case 108: event_out.key = Key::RightAlt; break;
			case 133: event_out.key = Key::LeftSystem; break;
			case 134: event_out.key = Key::RightSystem; break;

			case 9: event_out.key = Key::Escape; break;
			case 22: event_out.key = Key::Backspace; break;
			case 23: event_out.key = Key::Tab; break;
			case 36: event_out.key = Key::Enter; break;
			case 65: event_out.key = Key::Space; break;
			case 66: event_out.key = Key::CapsLock; break;
			case 78: event_out.key = Key::ScrollLock; break;
			case 110: event_out.key = Key::Home; break;
			case 112: event_out.key = Key::PageUp; break;
			case 115: event_out.key = Key::End; break;
			case 117: event_out.key = Key::PageDown; break;
			case 118: event_out.key = Key::Insert; break;
			case 119: event_out.key = Key::Delete; break;
			case 127: event_out.key = Key::Pause; break;
			case 135: event_out.key = Key::Apps; break;

			case 90: event_out.key = Key::Numpad0; break;
			case 87: event_out.key = Key::Numpad1; break;
			case 88: event_out.key = Key::Numpad2; break;
			case 89: event_out.key = Key::Numpad3; break;
			case 83: event_out.key = Key::Numpad4; break;
			case 84: event_out.key = Key::Numpad5; break;
			case 85: event_out.key = Key::Numpad6; break;
			case 79: event_out.key = Key::Numpad7; break;
			case 80: event_out.key = Key::Numpad8; break;
			case 81: event_out.key = Key::Numpad9; break;
			case 77: event_out.key = Key::NumLock; break;
			case 106: event_out.key = Key::Divide; break;
			case 63: event_out.key = Key::Multiply; break;
			case 82: event_out.key = Key::Subtract; break;
			case 86: event_out.key = Key::Add; break;
			case 104: event_out.key = Key::Enter; break;
			case 91: event_out.key = Key::Decimal; break;

			case 20: event_out.key = Key::Plus; break;
			case 59: event_out.key = Key::Comma; break;
			case 61: event_out.key = Key::Minus; break;
			case 60: event_out.key = Key::Period; break;

			case 35: event_out.key = Key::OEM_1; break;
			case 51: event_out.key = Key::OEM_2; break;
			case 47: event_out.key = Key::OEM_3; break;
			case 21: event_out.key = Key::OEM_4; break;
			case 49: event_out.key = Key::OEM_5; break;
			case 34: event_out.key = Key::OEM_6; break;
			case 48: event_out.key = Key::OEM_7; break;
			case 94: event_out.key = Key::OEM_102; break;

			case 160: event_out.key = Key::VolumeMute; break;
			case 174: event_out.key = Key::VolumeDown; break;
			case 176: event_out.key = Key::VolumeUp; break;
			case 171: event_out.key = Key::MediaNextTrack; break;
			case 173: event_out.key = Key::MediaPrevTrack; break;
			case 172: event_out.key = Key::MediaStop; break;
			case 170: event_out.key = Key::MediaPlayPause; break;
			}

			// KeyPress
			if (e.type == KeyPress_)
			{
				// Check previous key state
				const bool wasDown =
					(event_out.key != Key::Unknown) &&
					((uint64_t)event_out.key < keyIsDown.GetSize()) &&
					keyIsDown.GetAt((uint64_t)event_out.key);

				// Update key state
				if (event_out.key != Key::Unknown && (uint64_t)event_out.key < keyIsDown.GetSize())
				{
					keyIsDown.SetTrue((uint64_t)event_out.key);
				}

				// Generate KeyDown for initial press
				if (!wasDown && event_out.key != Key::Unknown)
				{
					Event keyDownEvent = event_out;
					keyDownEvent.type = EventType::KeyDown;
					eventQueue.push(keyDownEvent);
				}

				// Always generate KeyPress
				event_out.type = EventType::KeyPress;
				eventQueue.push(event_out);
			}
			else // KeyRelease
			{
				if (event_out.key != Key::Unknown && (uint64_t)event_out.key < keyIsDown.GetSize())
				{
					keyIsDown.SetFalse((uint64_t)event_out.key);
				}

				event_out.type = EventType::KeyUp;
				eventQueue.push(event_out);
			}

			// TextEntered
			{
				XKeyPressedEvent* keyPressedEvent = (XKeyPressedEvent*)&e;
				char utf8Buffer[8] = {};
				Status status = 0;
				int len = Xutf8LookupString(window.xic, keyPressedEvent, utf8Buffer, sizeof(utf8Buffer), 0, &status);

				if (status == XBufferOverflow)
				{
					Log(LogType::Warning, "Buffer overflow in Xutf8LookupString.");
					break;
				}
				else if (status == XLookupChars || status == XLookupBoth)
				{
					std::string utf8Char(utf8Buffer, len);

					std::u32string u32_string = utf8_to_utf32(utf8Char);
					if (u32_string.size() >= 1)
					{
						Event textEvent;
						textEvent.type = EventType::TextEntered;
						textEvent.textEntered.character_utf32 = (uint32_t)u32_string[0];
						eventQueue.push(textEvent);
					}
				}
			}
		}
		break;

		case ClientMessage:
			if ((Atom)e.xclient.data.l[0] == window.atomWmDeleteWindow)
			{
				event_out.type = EventType::CloseRequest;
				eventQueue.push(event_out);
				break;
			}
			break;

		case DestroyNotify:
			break;

		default:
			break;
		}
	}

}

#endif
