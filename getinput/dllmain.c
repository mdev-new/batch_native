//#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <shlwapi.h>
#include <shellapi.h>

//#define DISABLE_CONTROLLER
#ifndef DISABLE_CONTROLLER
	#include <xinput.h>
	#pragma comment(lib, "Xinput9_1_0") // xinput 1.3
#endif

#include <stdbool.h>
#include <stdio.h>

#include "Injector.h"

#define GETINPUT_SUB

#define str(x) #x
#define DbgMsgBox(msg) MessageBoxA(NULL, msg, "DbgMsgBox" str(__COUNTER__), MB_ICONWARNING | MB_OK)

// i am too lazy lmfao
#define CreateThreadS(funptr) CreateThread(0,0,funptr,0,0,0)
#define ENV SetEnvironmentVariable

signed wheelDelta = 0;

long RtlGetVersion(RTL_OSVERSIONINFOW * lpVersionInformation);

PCHAR GETINPUT_SUB itoa_(int i) {
	static char buffer[21] = { 0 };

	char* c = buffer + 20;
	int x = abs(i);

	do {
		*--c = 48 + x % 10;
	} while (x && (x /= 10));

	if (i < 0) *--c = 45;
	return c;
}

int GETINPUT_SUB my_ceil(float num) {
	int a = num;
	if ((float)a != num) {
		return a + 1;
	}

	return a;
}

long GETINPUT_SUB getenvnum(const char* name) {
	static char buffer[32] = { 0 };
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

BYTE m[0x100] = { 0 };
char buffer[0x300] = { 0 };
/* TODO optimize */
VOID GETINPUT_SUB process_keys() {
	int isAnyKeyDown = 0, actionHappened = 0;
	BOOL state = 0;

	for (int i = 3; i < 0x100; ++i) {
		state = GetAsyncKeyState(i);
		// todo optimize
		if (!m[i] && state & 0x8000) { m[i] = TRUE; actionHappened = TRUE; }
		if (m[i] && !state) { m[i] = FALSE; actionHappened = TRUE; }

		isAnyKeyDown |= m[i];
	}

	if (!actionHappened) return;
	if (!isAnyKeyDown) {
		SetEnvironmentVariable("keyspressed", NULL);
		return;
	}

	ZeroMemory(buffer, 0x300);

	for (int i = 0; i < 0x100; ++i) {
		if (m[i]) {
			int num = (conversion_table[i] == 0) ? i : conversion_table[i];
			if (num != (BYTE)(-1)) {
				if (buffer[0] == 0) {
					buffer[0] = '-';
#ifdef IS_YESHI_ANNOYING_AGAIN
					ENV("keypressed", itoa_(num));
#endif
				}

				lstrcat(buffer, itoa_(num));
				lstrcat(buffer, "-");
			}
		}
	}

	SetEnvironmentVariable("keyspressed", buffer);
}

#ifndef DISABLE_CONTROLLER
char* number_to_controller(WORD num) {
	switch (num) {
	case 0x0001: return "DPAD_UP";
	case 0x0002: return "DPAD_DOWN";
	case 0x0004: return "DPAD_LEFT";
	case 0x0008: return "DPAD_RIGHT";
	case 0x0010: return "START";
	case 0x0020: return "BACK";
	case 0x0040: return "LTHUMB";
	case 0x0080: return "RTHUMB";
	case 0x0100: return "LSHOULDR";
	case 0x0200: return "RSHOULDR";
	case 0x1000: return "BTN_A";
	case 0x2000: return "BTN_B";
	case 0x4000: return "BTN_X";
	case 0x8000: return "BTN_Y";
	default:     return NULL;
	}
	return NULL;
}

VOID GETINPUT_SUB PROCESS_CONTROLLER() {
	char buffer[256] = { 0 };
	char buffer1[16] = { 'c', 'o', 'n', 't', 'r', 'o', 'l', 'l', 'e', 'r', 0, 0, 0, 0, 0, 0 };
	XINPUT_STATE state = { 0 };
	DWORD dwResult = 0;
	int size = 0;

	// pretend like this doesn't exist
	const static int list[] = {
		0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040,
		0x0080, 0x0100, 0x0200, 0x1000, 0x2000, 0x4000, 0x8000
	};

	for (DWORD i = 0; i < 4; i++) {
		ZeroMemory(&state, sizeof(XINPUT_STATE));
		dwResult = XInputGetState(i, &state);

		buffer1[10] = '0' + (i + 1);

		if (dwResult == ERROR_SUCCESS) { /* controller is connected */
			ZeroMemory(buffer, 256);
			size = 0;

			for (WORD var = 0; var < 14; var++) {
				if (state.Gamepad.wButtons & list[var]) {
					char* ptr = number_to_controller(list[var]);
					size += sprintf(buffer + size, "-%s", ptr);
				}
			}

			size += sprintf(buffer + size, "-ltrig=%d-", state.Gamepad.bLeftTrigger);
			size += sprintf(buffer + size, "rtrig=%d-", state.Gamepad.bRightTrigger);
			size += sprintf(buffer + size, "lthumbx=%d-", state.Gamepad.sThumbLX);
			size += sprintf(buffer + size, "lthumby=%d-", state.Gamepad.sThumbLY);
			size += sprintf(buffer + size, "rthumbx=%d-", state.Gamepad.sThumbRX);
			size += sprintf(buffer + size, "rthumby=%d-", state.Gamepad.sThumbRY);

			SetEnvironmentVariable(buffer1, buffer);
		}
		else {
			SetEnvironmentVariable(buffer1, NULL);
		}
	}
}
#endif

LRESULT GETINPUT_SUB CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	MSLLHOOKSTRUCT* info = (MSLLHOOKSTRUCT*)lParam;
	wheelDelta = GET_WHEEL_DELTA_WPARAM(info->mouseData);

	if (wheelDelta != 0) {
		wheelDelta /= WHEEL_DELTA;
		ENV("wheeldelta", itoa_(wheelDelta));
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

DWORD GETINPUT_SUB CALLBACK Process(void* data) {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HWND hCon = GetConsoleWindow();

	BYTE mouseclick;

	CONSOLE_FONT_INFO info;
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

	int rasterx = getenvnum("rasterx");
	int rastery = getenvnum("rastery");

	const bool isRaster = rasterx && rastery;

	if (isRaster) {
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

	if (getenvnum("noresize") == 1) {
		DWORD style = GetWindowLong(hCon, GWL_STYLE);
		SetWindowLong(hCon, GWL_STYLE, style & ~(WS_SIZEBOX | WS_MAXIMIZEBOX));
	}

	HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);

	if (bWin10) {
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

	while (TRUE) {
		Sleep(1000 / 125);
		PeekMessage(&mouseMsg, hCon, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE);
		if (((++counter) % 5) != 0) continue;

		SetConsoleMode(hIn, mode);
		GetCurrentConsoleFont(hOut, FALSE, &info);

		if (bWin10) GetScaleFactorForMonitorProc(MonitorFromWindow(hCon, MONITOR_DEFAULTTONEAREST), &scale);

		GetPhysicalCursorPos(&pt);
		ScreenToClient(hCon, &pt);

		mouseclick =
			(GetKeyState(VK_LBUTTON) & 0x80) >> 7 |
			(GetKeyState(VK_RBUTTON) & 0x80) >> 6 |
			(GetKeyState(VK_MBUTTON) & 0x80) >> 5;

		if (mouseclick && GetSystemMetrics(SM_SWAPBUTTON)) {
			mouseclick ^= mouseclick & 3; // todo
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

		ENV("mousexpos", (!bLimit || (bLimit && mousx <= lmx)) ? itoa_(mousx) : NULL);
		ENV("mouseypos", (!bLimit || (bLimit && mousy <= lmy)) ? itoa_(mousy) : NULL);

		if (hCon == GetForegroundWindow()) {
			ENV("click", itoa_(mouseclick));
			process_keys();
#ifndef DISABLE_CONTROLLER
			PROCESS_CONTROLLER();
#endif
		}
	}
}

BOOL GETINPUT_SUB APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH) {

		//char runningProcessName[MAX_PATH] = { 0 };
		//GetModuleFileName(NULL, runningProcessName, MAX_PATH);
		//if (lstrcmp("rundll32.exe", runningProcessName + lstrlen(runningProcessName) - 12) != 0) {
		//	return TRUE;
		//}

		//MessageBox(NULL, "Running", "a", 0);
		DisableThreadLibraryCalls(hInst);
		CreateThreadS(Process);
	}

	return TRUE;
}