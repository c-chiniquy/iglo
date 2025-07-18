
#pragma once

#define Font Font_ // Conflicts with ig::Font
#define Cursor Cursor_

#include <X11/Xutil.h>
#include <X11/Xlib.h>

#ifdef Always
#undef Always // Conflicts with iglo.h enum elements
#define Always_ 2
#endif

#ifdef None
#undef None // Conflicts with iglo.h enum elements
#define None_ 0L
#endif

#ifdef KeyPress
#undef KeyPress // Conflicts with iglo.h ig::EventType::KeyPress
#define KeyPress_ 2
#endif

namespace ig
{
	using CallbackX11EventHook = std::function<bool(Display* display, Window window, const XEvent& e)>;

	struct Impl_WindowData
	{
		std::string title = "";
		bool minimized = false; // Whether the window is currently minimized.
		bool enableDragAndDrop = false; // If true, files can be drag and dropped onto the window.
		Timer fullscreenCompleteTimer; // To ignore focus events until fullscreen transition is complete

		Display* display = nullptr;
		Window handle = 0;
		Screen* screen = nullptr;
		XVisualInfo visualInfo;
		int screenId = 0;
		XSetWindowAttributes attributes;
		Atom atomWmDeleteWindow = 0;
		int x11fileDescriptor = 0;
		XIC xic = 0;
		XIM xim = 0;
		CallbackX11EventHook callbackX11EventHook = nullptr;
	};

	void SetXDecorationsVisible(Display* display, Window window, bool enable);
	void SetXResizable(Display* display, Window window, bool resizable, Extent2D resizableExtent);
	void SetXFullscreen(Display* display, Window window, bool fullscreen);
	void SetXWindowIcon(Display* display, Window window, const uint32_t* iconDataWithSize, uint32_t dataLength);
	void SetXWindowTitle(Display* display, Window window, const std::string& title);
	bool IsXWindowMinimized(Display* display, Window window);

}
