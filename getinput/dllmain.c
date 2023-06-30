#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <xinput.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "Injector.h"
#include "Utilities.h"

#define GETINPUT_SUB

// i am too lazy lmfao
#define ENV SetEnvironmentVariable

// not downloading DDK for this :)
long RtlGetVersion(RTL_OSVERSIONINFOW * lpVersionInformation);

int GETINPUT_SUB my_ceil(float num) {
	int a = num;
	if ((float)a != num) {
		return a + 1;
	}

	return a;
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

char* number_to_controller[] = {
	"dpad_up",
	"dpad_down",
	"dpad_left",
	"dpad_right",
	"start",
	"back",
	"lthumb",
	"rthumb",
	"lshouldr",
	"rshouldr",
	"btn_a",
	"btn_b",
	"btn_x",
	"btn_y"
};

typedef struct _pair {
	int first, second;
} pair;

pair process_stick(pair axes, int deadzone) {
	int dx = deadzone * abs(axes.first);
	int dy = deadzone * abs(axes.second);

	pair result = axes;
	if (abs(result.first) < dx) result.first = 0;
	if (abs(result.second) < dy) result.second = 0;

	return result;
}

pair old_process_stick(pair axes, int deadzone) {
	int x = 0, y = 0;
	int abs_inx = abs(axes.first);
	int abs_iny = abs(axes.second);

	if (abs_inx && abs_inx > deadzone) x = axes.first;
	if (abs_iny && abs_iny > deadzone) y = axes.second;

#ifdef divide
	const int divider = 61;
	x = x / (deadzone / divider);
	y = y / (deadzone / divider);
#endif

	return (pair) { x, y };
}

pair process_stick2(pair axes, short deadzone) {
	if ((axes.first < deadzone && axes.first > -deadzone) && (axes.second < deadzone && axes.second > -deadzone))
	{
		axes.first = 0;
		axes.second = 0;
	}

	return axes;
}

pair process_stick3(pair axes, short deadzone) {

	if (axes.first * axes.first < deadzone * deadzone) {
		axes.first = 0;
	}

	if (axes.second * axes.second < deadzone * deadzone) {
		axes.second = 0;
	}


	return axes;
}

VOID GETINPUT_SUB PROCESS_CONTROLLER(float deadzone) {
	static char buffer[256] = { 0 };
	static char varName[16] = { 'c', 'o', 'n', 't', 'r', 'o', 'l', 'l', 'e', 'r', 0, 0, 0, 0, 0, 0 };
	XINPUT_STATE state;
	DWORD dwResult, size;

	// pretend like this doesn't exist
	const static int list[] = {
		0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040,
		0x0080, 0x0100, 0x0200, 0x1000, 0x2000, 0x4000, 0x8000
	};

	for (char i = 0; i < 4; i++) {
		ZeroMemory(&state, sizeof(XINPUT_STATE));
		dwResult = XInputGetState(i, &state);

		varName[10] = '0' + (i + 1);

		if (dwResult == ERROR_SUCCESS) { /* controller is connected */
			ZeroMemory(buffer, sizeof buffer);
			size = 0;

			int result;
			for (WORD var = 0; var < 14; var++) {
				if (result = (state.Gamepad.wButtons & list[var])) {
					if (size && result) buffer[size++] = ',';

					char* ptr = number_to_controller[var];
					size += sprintf(buffer + size, "%s", ptr);
				}
			}

			if (state.Gamepad.wButtons) {
				buffer[size++] = '|';
			}

			pair left_stick = process_stick3((pair){ state.Gamepad.sThumbLX, state.Gamepad.sThumbLY }, (int)(deadzone * (float)(0x7FFF)));
			pair right_stick = process_stick3((pair) { state.Gamepad.sThumbRX, state.Gamepad.sThumbRY }, (int)(deadzone * (float)(0x7FFF)));
			size += sprintf(buffer + size, "ltrig=%d,rtrig=%d,lthumbx=%d,lthumby=%d,rthumbx=%d,rthumby=%d", state.Gamepad.bLeftTrigger, state.Gamepad.bRightTrigger, left_stick.first, left_stick.second, right_stick.first, right_stick.second);

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
}

DWORD GETINPUT_SUB CALLBACK Process(void* data) {
	ENV("wheeldelta", "0");
	ENV("mousexpos", "0");
	ENV("mouseypos", "0");
	ENV("click", "0");

	Sleep(250);

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HWND hCon = GetConsoleWindow();

	BYTE mouseclick;

	CONSOLE_FONT_INFO info = { 0 };
	COORD* fontSz = &info.dwFontSize;
	POINT pt;

	RTL_OSVERSIONINFOW osVersionInfo = { 0 };
	osVersionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

	// ntdll.dll
	RtlGetVersion(&osVersionInfo);
	const bool bWin10 = osVersionInfo.dwMajorVersion >= 10;

	HRESULT(*GetScaleFactorForMonitorProc)(HMONITOR, int*) = NULL;
	HRESULT(*SetProcessDpiAwarenessProc)(int) = NULL;

	if (bWin10) {
		HMODULE shcore = LoadLibraryA("shcore.dll");
		GetScaleFactorForMonitorProc = GetProcAddress(shcore, "GetScaleFactorForMonitor");
		SetProcessDpiAwarenessProc = GetProcAddress(shcore, "SetProcessDpiAwareness");
		CloseHandle(shcore);
	}

	int lmx = getenvnum("limitMouseX");
	int lmy = getenvnum("limitMouseY");
	const bool bLimitMouse = lmx && lmy;

	int rasterx = getenvnum("rasterx");
	int rastery = getenvnum("rastery");
	const bool isRaster = rasterx && rastery;

	const float deadzone = (float)getenvnum_ex("ctrl_deadzone", 24) / 100.f;

	if (isRaster) {
		CONSOLE_FONT_INFOEX cfi = {
			.cbSize = sizeof(CONSOLE_FONT_INFOEX),
			.nFont = 0,
			.dwFontSize = { rasterx, rastery },
			.FontFamily = FF_DONTCARE,
			.FontWeight = FW_NORMAL,
			.FaceName = L"Terminal"
		};

		SetCurrentConsoleFontEx(hOut, FALSE, &cfi);
	}

	if (getenvnum("noresize") == 1) {
		DWORD style = GetWindowLong(hCon, GWL_STYLE);
		style &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
		SetWindowLong(hCon, GWL_STYLE, style);
	}

	if (bWin10) {
		SetProcessDpiAwarenessProc(DPI_AWARENESS_UNAWARE);
	}

	int scale = 100, prevScale = scale, roundedScale;
	float fscalex = 1.0, fscaley = fscalex;
	int mousx, mousy;

	DWORD mode = 0;
	GetConsoleMode(hIn, &mode);
	mode &= ~ENABLE_PROCESSED_INPUT;
	mode &= ENABLE_EXTENDED_FLAGS | (~ENABLE_QUICK_EDIT_MODE);

	CreateThread(NULL, 0, MouseMessageThread, NULL, 0, NULL);

	const int fps = getenvnum_ex("getinput_tps", 40);

	unsigned __int64 begin, end;

	while (TRUE) {
		begin = GetTickCount64();

		SetConsoleMode(hIn, mode);
		GetCurrentConsoleFont(hOut, FALSE, &info);

		if (bWin10) {
			HMONITOR monitor = MonitorFromWindow(hCon, MONITOR_DEFAULTTONEAREST);
			GetScaleFactorForMonitorProc(monitor, &scale);
		}

		GetPhysicalCursorPos(&pt);
		ScreenToClient(hCon, &pt);

		mouseclick =
			(GetKeyState(VK_LBUTTON) & 0x80) >> 7 |
			(GetKeyState(VK_RBUTTON) & 0x80) >> 6 |
			(GetKeyState(VK_MBUTTON) & 0x80) >> 5;

		if (mouseclick && GetSystemMetrics(SM_SWAPBUTTON)) {
			mouseclick |= mouseclick & 0b11;
		}

		if (bWin10 && prevScale != scale) {
			// this somehow (mostly) works, !!DO NOT TOUCH!!
			if (!isRaster) fscalex = fscaley = (float)(scale) / 100.f; // this probably needs a little tweaking
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

		mousx = my_ceil((float)pt.x / ((float)fontSz->X * fscalex) - 1.f);
		mousy = my_ceil((float)pt.y / ((float)fontSz->Y * fscaley) - 1.f);

		ENV("mousexpos", (!bLimitMouse || (bLimitMouse && mousx <= lmx)) ? itoa_(mousx) : NULL);
		ENV("mouseypos", (!bLimitMouse || (bLimitMouse && mousy <= lmy)) ? itoa_(mousy) : NULL);

		if (hCon == GetForegroundWindow()) {
			ENV("click", itoa_(mouseclick));
			process_keys();
			PROCESS_CONTROLLER(deadzone);
		}

		end = GetTickCount64();
		Sleep((1000 / fps) - max(end - begin, 0));
	}
}

BasicDllMainImpl(Process);