#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#include <xinput.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "Injector.h"
#include "Utilities.h"

#define GETINPUT_SUB

//#define DISABLE_SCALING

// i am too lazy lmfao
#define ENV SetEnvironmentVariable

// not downloading DDK for this :)
long RtlGetVersion(RTL_OSVERSIONINFOW * lpVersionInformation);

int GETINPUT_SUB my_ceil(float num) {
	int a = num;
	return ((float)a != num) ? a + 1 : a;
}

// i was way too lazy to check for values individually, so this was created
// this technically disallows key code 0xFF in some cases but once that becomes a problem
// i'll a) not care or b) not be maintaing this or c) will solve it (last resort)
BYTE conversion_table[256] = {
	//        0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	/* 0 */  -1, -1,  0,  0, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0, // exclude mouse buttons
	/* 1 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 2 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 3 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 4 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 5 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 6 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 7 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 8 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 9 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* A */  -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // exclude right/left ctrl,shift,etc.
	/* B */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* C */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* D */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* E */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* F */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

VOID GETINPUT_SUB process_keys() {
	static BYTE pressed[0x100] = { 0 };

	// 104 (# of keys on a standard keyboard) * 4 (max amount of chars emmited per key (-123)) + 1 (ending dash)
	static char buffer[104 * 4 + 1];

	int isAnyKeyDown = 0, actionHappened = 0;
	BOOL state = 0;
	WORD idx = 1;

	for (int i = 3; i < 0x100; ++i) {
		state = GetAsyncKeyState(i);

		if (!pressed[i] && (state & 0x8000) && conversion_table[i] != (BYTE)(-1)) {
			pressed[i] = TRUE;
			actionHappened = TRUE;
		}

		if (pressed[i] && !state) {
			pressed[i] = FALSE;
			actionHappened = TRUE;
		}

		isAnyKeyDown |= pressed[i];
	}

	if (!actionHappened) return;
	if (!isAnyKeyDown) {
		SetEnvironmentVariable("keyspressed", NULL);
		return;
	}

	ZeroMemory(buffer, sizeof(buffer));
	buffer[0] = '-';

	for (int i = 0; i < 0x100; ++i) {
		if (!pressed[i]) continue;

		int num = (conversion_table[i] != 0) ? conversion_table[i] : i;
		if (num == (BYTE)(-1)) continue;

		idx += sprintf(buffer + idx, "%d-", num);
	}

	SetEnvironmentVariable("keyspressed", buffer);
}

typedef struct _controller_value {
	WORD bitmask;
	char* str;
} controller_value;

controller_value ctrl_values[] = {
	{ 0x0001, "dpad_up" },
	{ 0x0002, "dpad_down" },
	{ 0x0004, "dpad_left" },
	{ 0x0008, "dpad_right" },
	{ 0x0010, "start" },
	{ 0x0020, "back" },
	{ 0x0040, "lthumb" },
	{ 0x0080, "rthumb" },
	{ 0x0100, "lshouldr" },
	{ 0x0200, "rshouldr" },
	{ 0x1000, "btn_a" },
	{ 0x2000, "btn_b" },
	{ 0x4000, "btn_x" },
	{ 0x8000, "btn_y" }
};

typedef struct _vec2i {
	int x, y;
} vec2i;

vec2i process_stick(vec2i axes, short deadzone) {
	const int deadzone_squared = deadzone * deadzone;

	if (axes.x * axes.x < deadzone_squared) {
		axes.x = 0;
	}

	if (axes.y * axes.y < deadzone_squared) {
		axes.y = 0;
	}

	return axes;
}

VOID GETINPUT_SUB PROCESS_CONTROLLER(float deadzone) {
	static char buffer[256] = { 0 };
	static char varName[16] = "controller";
	XINPUT_STATE state;
	DWORD dwResult, size;

	for (char i = 0; i < 4; i++) {
		ZeroMemory(&state, sizeof(XINPUT_STATE));
		dwResult = XInputGetState(i, &state);

		varName[10] = '0' + (i + 1);

		if (dwResult == ERROR_SUCCESS) { /* controller is connected */
			ZeroMemory(buffer, sizeof buffer);
			size = 0;

			int result;
			for (WORD var = 0; var < 14; var++) {
				if (result = (state.Gamepad.wButtons & ctrl_values[var].bitmask)) {
					if (size && result) buffer[size++] = ',';

					size += sprintf(buffer + size, "%s", ctrl_values[var].str);
				}
			}

			if (state.Gamepad.wButtons) {
				buffer[size++] = '|';
			}

			vec2i left_stick = process_stick((vec2i){ state.Gamepad.sThumbLX, state.Gamepad.sThumbLY }, (int)(deadzone * (float)(0x7FFF)));
			vec2i right_stick = process_stick((vec2i) { state.Gamepad.sThumbRX, state.Gamepad.sThumbRY }, (int)(deadzone * (float)(0x7FFF)));
			size += sprintf(buffer + size, "ltrig=%d,rtrig=%d,lthumbx=%d,lthumby=%d,rthumbx=%d,rthumby=%d", state.Gamepad.bLeftTrigger, state.Gamepad.bRightTrigger, left_stick.x, left_stick.y, right_stick.x, right_stick.y);

			SetEnvironmentVariable(varName, buffer);
		} else {
			SetEnvironmentVariable(varName, NULL);
		}
	}
}

LRESULT GETINPUT_SUB CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	MSLLHOOKSTRUCT* info = (MSLLHOOKSTRUCT*)lParam;
	short wheelDelta = GET_WHEEL_DELTA_WPARAM(info->mouseData);

	if (wheelDelta != 0) {
		wheelDelta /= WHEEL_DELTA;
		ENV("wheeldelta", itoa_(wheelDelta));
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

DWORD GETINPUT_SUB CALLBACK MouseMessageThread(void* data) {
	HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

DWORD GETINPUT_SUB CALLBACK Process(void* data) {
	ENV("wheeldelta", "0");
	ENV("mousexpos", "0");
	ENV("mouseypos", "0");
	ENV("click", "0");

	Sleep(250);

	const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	const HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	const HWND hCon = GetConsoleWindow();

	BYTE mouseclick;

	CONSOLE_FONT_INFO info = { 0 };
	COORD* fontSz = &info.dwFontSize;
	POINT pt;

#ifndef DISABLE_SCALING
	DWORD majorVer = 0;
	RtlGetNtVersionNumbers(&majorVer, NULL, NULL);

	bool bWin10 = majorVer >= 10;

	HRESULT(*GetScaleFactorForMonitorProc)(HMONITOR, int*) = NULL;
	HRESULT(*SetProcessDpiAwarenessProc)(int) = NULL;

	if (bWin10) {
		// for some odd reason loading the library first didn't work
		GetScaleFactorForMonitorProc = GetProcAddress(LoadLibraryA("shcore.dll"), "GetScaleFactorForMonitor");
		SetProcessDpiAwarenessProc = GetProcAddress(LoadLibraryA("shcore.dll"), "SetProcessDpiAwareness");

		if (GetScaleFactorForMonitorProc == NULL || SetProcessDpiAwarenessProc == NULL) {
			bWin10 = FALSE;
			MessageBox(NULL, "Scaling will not work.", "Happy little message", MB_ICONERROR | MB_OK);
		}
	}
#endif

	int lmx, lmy, rasterx, rastery;
	bool bLimitMouse, isRaster;

	const float deadzone = (float)getenvnum_ex("ctrl_deadzone", 24) / 100.f;

	CONSOLE_FONT_INFOEX cfi = {
		.cbSize = sizeof(CONSOLE_FONT_INFOEX),
		.nFont = 0,
		.dwFontSize = {0, 0},
		.FontFamily = FF_DONTCARE,
		.FontWeight = FW_NORMAL,
		.FaceName = L"Terminal"
	};

	if (getenvnum("noresize") == 1) {
		DWORD style = GetWindowLong(hCon, GWL_STYLE);
		style &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
		SetWindowLong(hCon, GWL_STYLE, style);
	}

#ifndef DISABLE_SCALING
	if (bWin10) {
		SetProcessDpiAwarenessProc(DPI_AWARENESS_UNAWARE);
	}
#endif

	int scale = 100, prevScale = scale, roundedScale;
	float fscalex = 1.0f, fscaley = fscalex;
	int mousx, mousy;
	WORD prevRasterX = -1;
	WORD prevRasterY = -1;

	DWORD mode = 0;
	GetConsoleMode(hIn, &mode);
	mode &= ~ENABLE_PROCESSED_INPUT;
	mode &= ENABLE_EXTENDED_FLAGS | (~ENABLE_QUICK_EDIT_MODE);

	CreateThread(NULL, 0, MouseMessageThread, NULL, 0, NULL);

	const int sleep = 1000 / getenvnum_ex("getinput_tps", 40);

	unsigned __int64 begin, took;

	while (TRUE) {
		begin = GetTickCount64();

		SetConsoleMode(hIn, mode);
		GetCurrentConsoleFont(hOut, FALSE, &info);

#ifndef DISABLE_SCALING
		if (bWin10) {
			HMONITOR monitor = MonitorFromWindow(hCon, MONITOR_DEFAULTTONEAREST);
			GetScaleFactorForMonitorProc(monitor, &scale);
		}
#endif

		GetPhysicalCursorPos(&pt);
		ScreenToClient(hCon, &pt);

		mouseclick =
			(GetKeyState(VK_LBUTTON) & 0x80) >> 7 |
			(GetKeyState(VK_RBUTTON) & 0x80) >> 6 |
			(GetKeyState(VK_MBUTTON) & 0x80) >> 5;

		if (mouseclick && GetSystemMetrics(SM_SWAPBUTTON)) {
			mouseclick |= mouseclick & 0b11;
		}

		lmx = getenvnum("limitMouseX");
		lmy = getenvnum("limitMouseY");
		bLimitMouse = lmx && lmy;

		rasterx = getenvnum("rasterx");
		rastery = getenvnum("rastery");
		isRaster = rasterx && rastery;

		// lets not set the font every frame
		if (isRaster && (prevRasterX != rasterx || prevRasterY != rastery)) {
			cfi.dwFontSize = (COORD){ rasterx, rastery };
			SetCurrentConsoleFontEx(hOut, FALSE, &cfi);
			prevRasterX = rasterx;
			prevRasterY = rastery;
		}

#ifndef DISABLE_SCALING
		if (bWin10 && prevScale != scale) {
			// this somehow (mostly) works, !!DO NOT TOUCH!!
			if (!isRaster) {
				fscalex = fscaley = (float)(scale) / 100.f; // this probably needs a little tweaking
			}
			else {
				roundedScale = (scale - 100 * (scale / 100));
				if (roundedScale < 50) {
					fscalex = fscaley = (float)(((scale + 50) * 100) / 10000L);
				}
				else if (roundedScale > 50 && scale % 100 != 0) {
					fscalex = (scale / 100L) + 1.f;
					fscaley = (float)(scale - scale % 50) / 100.f;
				}
			}

			prevScale = scale;
		}
#endif

		mousx = my_ceil((float)pt.x / ((float)fontSz->X * fscalex));
		mousy = my_ceil((float)pt.y / ((float)fontSz->Y * fscaley));

		ENV("mousexpos", (!bLimitMouse || (bLimitMouse && mousx <= lmx)) ? itoa_(mousx) : NULL);
		ENV("mouseypos", (!bLimitMouse || (bLimitMouse && mousy <= lmy)) ? itoa_(mousy) : NULL);

		if (hCon == GetForegroundWindow()) {
			ENV("click", itoa_(mouseclick));
			process_keys();
			PROCESS_CONTROLLER(deadzone);
		}

		took = GetTickCount64() - begin;
		Sleep(max(0, sleep - took));
	}
}

BasicDllMainImpl(Process);