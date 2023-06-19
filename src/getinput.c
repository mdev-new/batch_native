/*
 * dll.c (getinput.dll)
 * By mousiedev & kenan238
 * ---------------------------
 * Original getinput.exe by kenan238
 * Big big rewrite by mousiedev
 *
 * Licensed under conditions stated in the
	 "getinput.txt" document you recieved with this software.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#include <shellscalingapi.h>
#include <math.h>

#include <xinput.h>

#include <stdbool.h>

#include "extern/printf.h"

// compiler, i know what im doing, now shut up
#pragma GCC diagnostic ignored "-Wimplicit-int"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#pragma omp simd reduction(+: I)

#define GETINPUT_SUB __attribute__((noinline))

#define str(x) #x
#define DbgMsgBox(msg) MessageBoxA(NULL, msg, "DbgMsgBox" str(__COUNTER__), MB_ICONWARNING | MB_OK)

// i am too lazy lmfao
#define CreateThreadS(funptr) CreateThread(0,0,funptr,0,0,0)
#define ENV SetEnvironmentVariable

signed wheelDelta = 0;

PCHAR GETINPUT_SUB itoa_(int i) {
	static char buffer[21] = {0};

	char *c = buffer+20;
	int x = abs(i);

	do {
		*--c = 48 + x % 10;
	} while(x && (x/=10));

	if(i < 0) *--c = 45;
	return c;
}

int GETINPUT_SUB my_ceil(float num) {
	int a = num;
	if ((float)a != num) {
		return a+1;
	}

	return a;
}

#define isdigit(x) (x >= '0' && x <= '9' ? 1 : 0)
long GETINPUT_SUB atol(const char *num) {
	long value = 0;
	bool neg = 0;
	if (num[0] == '-') {
		neg = 1;
		++num;
	}

	while (*num && isdigit(*num)) {
		value = (value * 10) + (*num++ - '0');
	}

	return neg? -value : value;
}

long GETINPUT_SUB getenvnum(char *name) {
	static char buffer[32] = {0};
	return
		GetEnvironmentVariable(name, buffer, sizeof(buffer))
		? atol(buffer)
		: 0;
}

// i was way too lazy to check for values individually
// so i just made this
// this technically disallows key code 0xFF but once that becomes a problem
// i'll a) not care or b) not be maintaing this or c) will solve it (last resort)
// zeroes compress better
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

char *controller[] = {
	"DPAD_UP",
	"DPAD_DOWN",
	"DPAD_LEFT",
	"DPAD_RIGHT",
	"START",
	"BACK",
	"LTHUMB",
	"RTHUMB",
	"LSHOULDER",
	"RSHOULDER",
	"BTN_A",
	"BTN_B",
	"BTN_X",
	"BTN_Y"
};

BYTE m[0x100] = {0};
char buffer[0x300] = {0};
/* TODO optimize */
VOID GETINPUT_SUB process_keys() {
	int isAnyKeyDown = 0, actionHappened = 0;
	BOOL state = 0;

	for(int i = 3; i < 0x100; ++i) {
		state = GetAsyncKeyState(i);
		// todo optimize
		if (!m[i] && state & 0x8000) { m[i] = TRUE; actionHappened = TRUE; }
		if (m[i] && !state) { m[i] = FALSE; actionHappened = TRUE; }

		isAnyKeyDown |= m[i];
	}

	if(!actionHappened) return;
	if(!isAnyKeyDown) {
		SetEnvironmentVariable("keyspressed", NULL);
		return;
	}

	ZeroMemory(buffer, 0x300);

	for(int i = 0; i < 0x100; ++i) {
		if(m[i]) {
			int num = (conversion_table[i] == 0) ? i : conversion_table[i];
			if(num != (BYTE)(-1)) {
				if(buffer[0] == 0) {
					buffer[0] = '-';
#ifdef IS_YESHI_ANNOYING_AGAIN // never was defined and never will
					ENV("keypressed", itoa_(num));
#endif
				}

				lstrcat(buffer, itoa_(num));
				lstrcat(buffer, "-");
			}
		}
	}

	SetEnvironmentVariable("keyspressed",buffer);
}

LRESULT GETINPUT_SUB CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	MSLLHOOKSTRUCT *info = (MSLLHOOKSTRUCT *)lParam;
	wheelDelta = GET_WHEEL_DELTA_WPARAM(info->mouseData);
	if (wheelDelta != 0) {
		wheelDelta /= WHEEL_DELTA;
		ENV("wheeldelta", itoa_(wheelDelta));
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

DWORD GETINPUT_SUB CALLBACK Process(void *data) {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HWND hCon = GetConsoleWindow();

	BYTE mouseclick;
	
	CONSOLE_FONT_INFO info;
	COORD * fontSz = &info.dwFontSize;
	POINT pt;

	RTL_OSVERSIONINFOW osVersionInfo = {0};
	osVersionInfo.dwOSVersionInfoSize = sizeof (RTL_OSVERSIONINFOW);

	// ntdll.dll
	RtlGetVersion(&osVersionInfo);
	const bool bWin10 = osVersionInfo.dwMajorVersion >= 10;

	HRESULT(*GetScaleFactorForMonitorProc)(HMONITOR, int *) = NULL;
	HRESULT(*SetProcessDpiAwarenessProc)(int) = NULL;

	if(bWin10) {
		HANDLE shcore = LoadLibraryA("shcore.dll");
		GetScaleFactorForMonitorProc = GetProcAddress(shcore, "GetScaleFactorForMonitor");
		SetProcessDpiAwarenessProc = GetProcAddress(shcore, "SetProcessDpiAwareness");
		CloseHandle(shcore);
	}

	int rasterx = getenvnum("rasterx");
	int rastery = getenvnum("rastery");

	const bool isRaster = rasterx && rastery;

	if(isRaster) {
		CONSOLE_FONT_INFOEX cfi = {
			.cbSize = sizeof(CONSOLE_FONT_INFOEX),
			.nFont = 0,
			.dwFontSize = {rasterx, rastery},
			.FontFamily = FF_DONTCARE,
			.FontWeight = FW_NORMAL,
			.FaceName = L"Terminal"
		};

		SetCurrentConsoleFontEx(hOut, FALSE, &cfi);
	}

	int lmx = getenvnum("limitMouseX");
	int lmy = getenvnum("limitMouseY");

	const bool bLimit = lmx && lmy;

	if(getenvnum("noresize") == 1) {
		DWORD style = GetWindowLong(hCon, GWL_STYLE);
		SetWindowLong(hCon, GWL_STYLE, style & ~(WS_SIZEBOX | WS_MAXIMIZEBOX));
	}

	HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);

	if(bWin10) {
		SetProcessDpiAwarenessProc(DPI_AWARENESS_UNAWARE);
	}

	MSG mouseMsg;
	int scale = 100, prevScale = scale, roundedScale;
	float fscalex = 1.0, fscaley = fscalex;
	register int mousx, mousy;

	char counter = 0;

	DWORD mode = 0;
	GetConsoleMode(hIn, &mode);
	mode &= ~ENABLE_PROCESSED_INPUT;
	mode &= ENABLE_EXTENDED_FLAGS | (~ENABLE_QUICK_EDIT_MODE);

	char buffer[1024]; // a preallocated temp buffer ain't killing noone :)
	char buffer1[128]; // and another one :)

	while(TRUE) {
		Sleep(1000 / 125);
		PeekMessage(&mouseMsg, hCon, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE);
		if(((++counter) % 5) != 0) continue;

		SetConsoleMode(hIn, mode);
		GetCurrentConsoleFont(hOut, FALSE, &info);

		if(bWin10) GetScaleFactorForMonitorProc(MonitorFromWindow(hCon, MONITOR_DEFAULTTONEAREST), &scale);

		GetPhysicalCursorPos(&pt);
		ScreenToClient(hCon,&pt);

		mouseclick = 
			(GetKeyState(VK_LBUTTON) & 0x80) >> 7 |
			(GetKeyState(VK_RBUTTON) & 0x80) >> 6 |
			(GetKeyState(VK_MBUTTON) & 0x80) >> 5;

		if(mouseclick && GetSystemMetrics(SM_SWAPBUTTON)) {
			mouseclick ^= mouseclick & 3; // todo
		}

		if(bWin10 && prevScale != scale) {
			// this somehow (mostly) works, !!DO NOT TOUCH!!
			if(!isRaster) fscalex = fscaley = (float)(scale) / 100.f; // this probably needs a little tweaking
			else {
				roundedScale = (scale - 100 * (scale / 100));
				if(roundedScale < 50) {
					fscalex = fscaley = ((scale + 50) * 100) / 10000L;
				} else if(roundedScale > 50 && scale % 100 != 0) {
					fscalex = (scale / 100L) + 1.f;
					fscaley = (float)(scale - scale % 50) / 100.f;
				}
			}

			prevScale = scale;
		}

		for (DWORD i = 0; i < 4; i++) {
			XINPUT_STATE state;
			ZeroMemory(&state, sizeof(XINPUT_STATE));

			DWORD dwResult = XInputGetState(i, &state);

			ZeroMemory(buffer1, 128);
			sprintf(buffer1, "controller%d", i+1);

			if(dwResult == ERROR_SUCCESS) { /* controller is connected */
				ZeroMemory(buffer, 1024);
				buffer[0] = '-';

				WORD size = 0;

				for(int j = 1; j <= 0x8000; j <<= 1) {
					if(state.Gamepad.wButtons & j) {
						size += sprintf(buffer+size, "%s-", controller[j]);
					}
				}

				size += sprintf(buffer+size, "ltrig=%d-", state.Gamepad.bLeftTrigger);
				size += sprintf(buffer+size, "rtrig=%d-", state.Gamepad.bRightTrigger);
				size += sprintf(buffer+size, "lthumbx=%d-", state.Gamepad.sThumbLX);
				size += sprintf(buffer+size, "lthumby=%d-", state.Gamepad.sThumbLY);
				size += sprintf(buffer+size, "rthumbx=%d-", state.Gamepad.sThumbRX);
				size += sprintf(buffer+size, "rthumby=%d", state.Gamepad.sThumbRY);

				SetEnvironmentVariable(buffer1, buffer);
			} else {
				SetEnvironmentVariable(buffer1, NULL);
			}
		}

		// this would get messy
		mousx = my_ceil((float)pt.x / ((float)fontSz->X * fscalex) - 1.f);
		mousy = my_ceil((float)pt.y / ((float)fontSz->Y * fscaley) - 1.f);

		ENV("mousexpos", (!bLimit || (bLimit && mousx <= lmx)) ? itoa_(mousx) : NULL);
		ENV("mouseypos", (!bLimit || (bLimit && mousy <= lmy)) ? itoa_(mousy) : NULL);

		if(hCon == GetForegroundWindow()) {
			ENV("click", itoa_(mouseclick));
			process_keys();
		}
	}
}

BOOL GETINPUT_SUB APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
	DbgMsgBox("running!");
	if (dwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hInst);
		CreateThreadS(Process);
	}
	return TRUE;
}
