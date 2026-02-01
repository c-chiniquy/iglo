
#pragma once

#define NOMINMAX // Conflicts with std::min std::max

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _WIN64
#ifndef WIN64_LEAN_AND_MEAN
#define WIN64_LEAN_AND_MEAN
#endif
#endif

#include <windows.h>
#include <shellapi.h>

// Conflicts with ig::CreateDirectory
#ifdef CreateDirectory
#undef CreateDirectory
#endif

namespace ig
{
	using CallbackWndProcHook = std::function<bool(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)>;
	
	struct Impl_WindowData
	{
		std::string title = "";
		bool minimized = false; // Whether the window is currently minimized.
		bool enableDragAndDrop = false; // If true, files can be drag and dropped onto the window.
		bool activeResizing = false; // If true, window has entered size move (user is moving or resizing window)
		bool activeMenuLoop = false; // If true, user created a context menu on this window.
		bool ignoreSizeMsg = false; // To ignore all WM_SIZE messages sent by SetWindowLongPtr().

		HWND hwnd = nullptr; // Window handle
		HICON iconOwned = nullptr; // This icon should be released with DestroyIcon() when it stops being used.
		HCURSOR defaultCursor = nullptr;
		uint16_t pendingHighSurrogate = 0; // Used in WM_CHAR
		CallbackWndProcHook callbackWndProcHook = nullptr;
	};

	struct Impl_Cursor
	{
		HCURSOR handle = nullptr;
		bool ownsHandle = false;
	};

	bool IsMouseInsideWindow_Win32(HWND windowHWND);
	void SetWindowState_Win32(Impl_WindowData& window, const WindowState& state, bool topMost, bool gainFocus);
	void CaptureMouse_Win32(Impl_WindowData& window, bool captured);

}