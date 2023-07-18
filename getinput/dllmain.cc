#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#include <xinput.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <atomic>

#include "Injector.h"
#include "Utilities.h"

#define GETINPUT_SUB __declspec(noinline)

// i am too lazy lmfao
#define ENV SetEnvironmentVariable

extern "C" void RtlGetNtVersionNumbers(DWORD*, DWORD*, DWORD*);

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
	const char* str;
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

			vec2i left_stick = process_stick({ state.Gamepad.sThumbLX, state.Gamepad.sThumbLY }, (int)(deadzone * (float)(0x7FFF)));
			vec2i right_stick = process_stick({ state.Gamepad.sThumbRX, state.Gamepad.sThumbRY }, (int)(deadzone * (float)(0x7FFF)));
			size += sprintf(buffer + size, "ltrig=%d,rtrig=%d,lthumbx=%d,lthumby=%d,rthumbx=%d,rthumby=%d", state.Gamepad.bLeftTrigger, state.Gamepad.bRightTrigger, left_stick.x, left_stick.y, right_stick.x, right_stick.y);

			SetEnvironmentVariable(varName, buffer);
		} else {
			SetEnvironmentVariable(varName, NULL);
		}
	}
}

std::atomic_bool inFocus = true;

DWORD GETINPUT_SUB CALLBACK ModeThread(void* data) {
	// i don't like this. at all.
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	HWND hCon = GetConsoleWindow();

	DWORD mode = (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS) & ~(ENABLE_QUICK_EDIT_MODE);
	
	while (1) {
		if (hCon == GetForegroundWindow()) { // atleast a tiny optimization
			SetConsoleMode(hStdIn, mode);
		} else {
			Sleep(40);
		}
	}

	return 0;
}

void MouseEventProc(MOUSE_EVENT_RECORD& record) {
	switch (record.dwEventFlags) {
	case MOUSE_MOVED:
		ENV("mousexpos", itoa_(record.dwMousePosition.X + 1));
		ENV("mouseypos", itoa_(record.dwMousePosition.Y + 1));
		break;

	case MOUSE_WHEELED:
		ENV("wheeldelta", itoa_((signed short)(HIWORD(record.dwButtonState)) / WHEEL_DELTA));
		break;

	default: break;
	}
}

DWORD GETINPUT_SUB CALLBACK MousePosThread(void* data) {
	// i don't like this, but if it means completely working mouse, i'll do it

	INPUT_RECORD ir_buffer[256];
	DWORD read, i;

	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

	while (1) {
		ReadConsoleInput(hStdIn, ir_buffer, 256, &read);

		for (i = 0; i < read; i++) {
			switch (ir_buffer[i].EventType) {
			case MOUSE_EVENT:
				MouseEventProc(ir_buffer[i].Event.MouseEvent);
				break;

			case FOCUS_EVENT:
				inFocus = ir_buffer[i].Event.FocusEvent.bSetFocus;
				break;

			case WINDOW_BUFFER_SIZE_EVENT:
			case MENU_EVENT:
			case KEY_EVENT: // keys are processed async
			default:
				break;
			}
		}
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

	short lmx, lmy, rasterx, rastery;
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
		DrawMenuBar(hCon);
	}

	WORD prevRasterX = -1;
	WORD prevRasterY = -1;

	CreateThread(NULL, 0, ModeThread, NULL, 0, NULL);
	CreateThread(NULL, 0, MousePosThread, NULL, 0, NULL);

	const int sleep = 1000 / getenvnum_ex("getinput_tps", 40);

	unsigned __int64 begin, took;

	while (TRUE) {
		begin = GetTickCount64();

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
			cfi.dwFontSize = { rasterx, rastery };
			SetCurrentConsoleFontEx(hOut, FALSE, &cfi);
			prevRasterX = rasterx;
			prevRasterY = rastery;
		}

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