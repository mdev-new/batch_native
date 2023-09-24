#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

#include <atomic>
#include <thread>
#include <timeapi.h>

#include "Injector.h"
#include "Utilities.h"

#define GETINPUT_SUB __declspec(noinline)

//#define WIN2K_BUILD

#ifndef WIN2K_BUILD
#	define ENABLE_CONTROLLER
#endif

#ifdef ENABLE_CONTROLLER
#	pragma comment(lib, "XInput9_1_0.lib")
#	include <xinput.h>
#endif

// i am too lazy lmfao
#define ENV SetEnvironmentVariable

extern "C" void RtlGetNtVersionNumbers(DWORD*, DWORD*, DWORD*);

inline int _max(int a, int b) {
	return ((a > b) ? a : b);
}

std::atomic_bool inTextInput = false;

bool inFocus = true;
volatile int sleep_time = 1000;

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

#ifdef ENABLE_CONTROLLER

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

#ifndef CONTROLLER_NORMAL_INPUT

// too lazy to concat strings
const char* ControllerEnvNames[] = {
	"controller1_ltrig",
	"controller1_rtrig",
	"controller1_lthumbx",
	"controller1_lthumby",
	"controller1_rthumbx",
	"controller1_rthumby",
	"controller2_ltrig",
	"controller2_rtrig",
	"controller2_lthumbx",
	"controller2_lthumby",
	"controller2_rthumbx",
	"controller2_rthumby",
	"controller3_ltrig",
	"controller3_rtrig",
	"controller3_lthumbx",
	"controller3_lthumby",
	"controller3_rthumbx",
	"controller3_rthumby",
	"controller4_ltrig",
	"controller4_rtrig",
	"controller4_lthumbx",
	"controller4_lthumby",
	"controller4_rthumbx",
	"controller4_rthumby"
};

const char* ControllerBtnEnvNames[] = {
	"controller1_btns",
	"controller2_btns",
	"controller3_btns",
	"controller4_btns"
};

#endif

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

#ifdef CONTROLLER_NORMAL_INPUT
		varName[10] = '0' + (i + 1);
#endif
		 
		if (dwResult == ERROR_SUCCESS) { /* controller is connected */
			ZeroMemory(buffer, sizeof buffer);

			vec2i left_stick = process_stick({ state.Gamepad.sThumbLX, state.Gamepad.sThumbLY }, (int)(deadzone * (float)(0x7FFF)));
			vec2i right_stick = process_stick({ state.Gamepad.sThumbRX, state.Gamepad.sThumbRY }, (int)(deadzone * (float)(0x7FFF)));

#ifdef CONTROLLER_NORMAL_INPUT
			size = sprintf(buffer + size, "ltrig=%d,rtrig=%d,lthumbx=%d,lthumby=%d,rthumbx=%d,rthumby=%d", state.Gamepad.bLeftTrigger, state.Gamepad.bRightTrigger, left_stick.x, left_stick.y, right_stick.x, right_stick.y);

			if (state.Gamepad.wButtons) {
				buffer[size++] = '|';
			}

			int origSize = size;

#else
// YESHIS INSANE, NOT PRETTY FUCKERY BEGINS HERE
// I DONT WANT TO DO THIS

			ENV(ControllerEnvNames[i * 6 + 0], itoa_(state.Gamepad.bLeftTrigger));
			ENV(ControllerEnvNames[i * 6 + 1], itoa_(state.Gamepad.bRightTrigger));
			ENV(ControllerEnvNames[i * 6 + 2], itoa_(left_stick.x));
			ENV(ControllerEnvNames[i * 6 + 3], itoa_(left_stick.y));
			ENV(ControllerEnvNames[i * 6 + 4], itoa_(right_stick.x));
			ENV(ControllerEnvNames[i * 6 + 5], itoa_(right_stick.y));

#endif

			int result;
			for (WORD var = 0; var < 14; var++) {
				if (result = (state.Gamepad.wButtons & ctrl_values[var].bitmask)) {
					if (
#ifdef CONTROLLER_NORMAL_INPUT
						(size != origSize) &&
#endif
					result) {
						buffer[size++] = ',';
					}

#ifdef CONTROLLER_NORMAL_INPUT
					size += sprintf(buffer + size, "%s", ctrl_values[var].str);
#else
					sprintf(buffer + size, "%s", ctrl_values[var].str);
#endif
				}
			}

#ifdef CONTROLLER_NORMAL_INPUT
			SetEnvironmentVariable(varName, buffer);
#else
			SetEnvironmentVariable(ControllerBtnEnvNames[i], buffer);
#endif

		} else {
			SetEnvironmentVariable(varName, NULL);
		}
	}
}

#endif

DWORD GETINPUT_SUB CALLBACK ModeThread(void* data) {
	// i don't like this function. at all.
	// basically we're just chugging cpu because microsoft
	// decided to make windows utter trash.

	HANDLE hStdIn =  GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	DWORD modeIn, modeOut, inputModeRead;

#ifndef WIN2K_BUILD
	modeIn = (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS) & ~(ENABLE_QUICK_EDIT_MODE);
	modeOut = ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;
#else
#	ifndef ENABLE_EXTENDED_FLAGS
#		define ENABLE_EXTENDED_FLAGS 0x0080
#	endif

	modeIn = (ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);
	modeOut = 0;
#endif

	DWORD textInputMode = modeIn | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT;

	DWORD partial_cmdInMode = ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT;

	while (1) {
		GetConsoleMode(hStdIn, &inputModeRead);
		if (inputModeRead & partial_cmdInMode) {
			inTextInput = true;
		}
		else { inTextInput = false; }

		SetConsoleMode(hStdIn, inTextInput ? textInputMode : modeIn);
		SetConsoleMode(hStdOut, modeOut);

#ifndef WIN2K_BUILD
		usleep(500);
#else
		Sleep(1);
#endif
	}

	return 0;
}

void MouseEventProc(MOUSE_EVENT_RECORD& record) {
	if (!inFocus) return;

	switch (record.dwEventFlags) {
	case MOUSE_MOVED: {
		int lmx = getenvnum("limitMouseX"), lmy = getenvnum("limitMouseY");

		int mouseX = record.dwMousePosition.X + 1;
		int mouseY = record.dwMousePosition.Y + 1;
		if (lmx && mouseX > lmx) mouseX = lmx;
		if (lmy && mouseY > lmy) mouseY = lmy;

		ENV("mousexpos", itoa_(mouseX));
		ENV("mouseypos", itoa_(mouseY));
		break;
	}

	case MOUSE_WHEELED:
		ENV("wheeldelta", itoa_((signed short)(HIWORD(record.dwButtonState)) / WHEEL_DELTA));
		break;

	default: break;
	}
}

void processEvnt(INPUT_RECORD ir) {
	switch (ir.EventType) {
	case MOUSE_EVENT: {
		MouseEventProc(ir.Event.MouseEvent);
		break;
	}

	case KEY_EVENT: { // keys are processed async
		if (inTextInput) {
			DWORD w;
			WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &w);
		}
		break;
	}

	case FOCUS_EVENT: // handled in main loop, this is quite buggy
	case WINDOW_BUFFER_SIZE_EVENT:
	case MENU_EVENT:
	default:
		break;
	}
}

DWORD GETINPUT_SUB CALLBACK MousePosThread(void* data) {
	// i don't like this, but if it means completely working mouse, i'll do it

	INPUT_RECORD ir[64];
	DWORD read, written;

	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

	while (1) {
		ReadConsoleInput(hStdIn, ir, 64, &read);

		for (int i = 0; i < read; i++) {
			processEvnt(ir[i]);
		}
	}

	return 0;
}

// maybe doesn't work on subsequent calls
void resizeConsoleIfNeeded(int *lastScreenX, int *lastScreenY) {
	int screenx = getenvnum("screenx");
	int screeny = getenvnum("screeny");

	if (screenx && screeny && (screenx != *lastScreenX) && (screeny != *lastScreenY)) {
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

		COORD consoleBufferSize = { screenx, screeny };
		SMALL_RECT windowInfo = { 0, 0, screenx - 1, screeny - 1 };

		SetConsoleWindowInfo(hOut, TRUE /* this has to be TRUE on Windows 11 */, &windowInfo);
		SetConsoleScreenBufferSize(hOut, consoleBufferSize);

		*lastScreenX = screenx;
		*lastScreenY = screeny;
	}

	return;
}

DWORD GETINPUT_SUB CALLBACK Process(void*) {
	ENV("wheeldelta", "0");
	ENV("mousexpos", "0");
	ENV("mouseypos", "0");
	ENV("click", "0");
	ENV("getinputInitialized", "1");

	const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	const HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	const HWND hCon = GetConsoleWindow();

	BYTE mouseclick;

#ifndef WIN2K_BUILD

	short rasterx, rastery;
	bool isRaster;

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

#endif

	timeBeginPeriod(1);

	sleep_time = 1000 / getenvnum_ex("getinput_tps", 40);

	HANDLE hModeThread    = CreateThread(NULL, 0, ModeThread    , NULL, 0, NULL);
	HANDLE hReadThread    = CreateThread(NULL, 0, MousePosThread, NULL, 0, NULL);

	unsigned __int64 begin, took;

	int lastscreenx = -1, lastscreeny = -1;
	resizeConsoleIfNeeded(&lastscreenx, &lastscreeny);

	while (TRUE) {
		begin = GetTickCount64();

		mouseclick =
			(GetKeyState(VK_LBUTTON) & 0x80) >> 7 |
			(GetKeyState(VK_RBUTTON) & 0x80) >> 6 |
			(GetKeyState(VK_MBUTTON) & 0x80) >> 5;

		if (mouseclick && GetSystemMetrics(SM_SWAPBUTTON)) {
			mouseclick |= mouseclick & 0b11;
		}

#ifndef WIN2K_BUILD

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

#endif

		inFocus = GetForegroundWindow() == hCon;

		if (inFocus) {
			ENV("click", itoa_(mouseclick));

			if (!inTextInput) {
				process_keys();
			}

#ifdef ENABLE_CONTROLLER
			PROCESS_CONTROLLER(deadzone);
#endif
		}

		took = GetTickCount64() - begin;
		Sleep(_max(0, sleep_time - took));
	}
}

BasicDllMainImpl(Process);