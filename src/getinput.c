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
#include <shellscalingapi.h>

// compiler, i know what im doing, now shut up
#pragma GCC diagnostic ignored "-Wimplicit-int"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#define ENV SetEnvironmentVariable

// i am too lazy lmfao
#define CreateThreadS(funptr) CreateThread(0,0,funptr,0,0,0)
#define InRange(x, b, t) (b < x && x < t)

const int UPS = 120; // Input polls per second
const int WAIT_TIME = 1000 / UPS;
signed wheelDelta;

typedef struct {
	BOOL isHooked;
	void* FunctionAddress;
	void* Hook;
	char  Jmp[6];
	char  APIBytes[6];
	void* APIFunction;
} hook_t;

typedef HDC(*fBeginPaint)(HWND hWnd, LPPAINTSTRUCT lpPaintStruct);
typedef unsigned __int64 QWORD;

BOOL InitializeHook(hook_t* Hook, char* ModuleName, char* FunctionName, void* HookFunction)
	{
		HMODULE hModule;
		ULONG OrigFunc, FuncAddr;
		char opcodes[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0xe9, 0x00, 0x00, 0x00, 0x00 };

		if (Hook->isHooked) {
			return FALSE;
		}
		hModule = GetModuleHandle(ModuleName);
		if (hModule == NULL) {
			Hook->isHooked = FALSE;
			return FALSE;
		}
		Hook->FunctionAddress = GetProcAddress(hModule, FunctionName);
		if (Hook->FunctionAddress == NULL) {
			Hook->isHooked = FALSE;
			return FALSE;
		}
		Hook->Jmp[0] = 0xe9;
		*(QWORD*)&Hook->Jmp[1] = (QWORD)HookFunction - (QWORD)Hook->FunctionAddress - 5;
		CopyMemory(Hook->APIBytes, Hook->FunctionAddress, 5);
		Hook->APIFunction = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (Hook->APIFunction == NULL) {
			return FALSE;
		}
		CopyMemory(Hook->APIFunction, Hook->APIBytes, 5);
		OrigFunc = (QWORD)Hook->APIFunction + 5;
		FuncAddr = (QWORD)Hook->FunctionAddress + 5;
		*(LPBYTE)((LPBYTE)Hook->APIFunction + 5) = 0xe9;
		*(QWORD*)((LPBYTE)Hook->APIFunction + 6) = (QWORD)FuncAddr - (QWORD)OrigFunc - 5;
		Hook->isHooked = TRUE;
		return TRUE;
	}

	BOOL InsertHook(hook_t* Hook)
	{
		DWORD op;
		if (!Hook->isHooked) {
			return FALSE;
		}
		VirtualProtect(Hook->FunctionAddress, 5, PAGE_EXECUTE_READWRITE, &op);
		memcpy(Hook->FunctionAddress, Hook->Jmp, 5);
		VirtualProtect(Hook->FunctionAddress, 5, op, &op);
		return TRUE;
	}

	BOOL Unhook(hook_t* Hook)
	{
		DWORD op;
		if (!Hook->isHooked) {
			return FALSE;
		}
		VirtualProtect(Hook->FunctionAddress, 5, PAGE_EXECUTE_READWRITE, &op);
		memcpy(Hook->FunctionAddress, Hook->APIBytes, 5);
		VirtualProtect(Hook->FunctionAddress, 5, op, &op);

		Hook->isHooked = FALSE;
		return TRUE;
	}

	BOOL FreeHook(hook_t* Hook)
	{
		if (Hook->isHooked) {
			return FALSE;
		}
		VirtualFree(Hook->APIFunction, 0, MEM_RELEASE);
		memset(Hook, 0, sizeof(hook_t*));
		return TRUE;
	}

CHAR b[21], *c;
PCHAR itoa_(i,x) {
  c=b+21,x=abs(i);
  do *--c = 48 + x % 10; while(x/=10); if(i<0) *--c = 45;
  return c;
}

BYTE m[0x100];
CHAR my_getch() {
  int i = 2;

  while(++i < 0x100) {
    if (GetAsyncKeyState(i) == -32768) m[i] = TRUE;
    else if (m[i]) { m[i] = FALSE; return MapVirtualKey(i, MAPVK_VK_TO_CHAR); }
  }

  // no key was pressed
  return 0;
}

LRESULT MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  MSLLHOOKSTRUCT *info = (MSLLHOOKSTRUCT *)lParam;
  wheelDelta = GET_WHEEL_DELTA_WPARAM(info->mouseData);

  if (wheelDelta != 0) wheelDelta /= WHEEL_DELTA;
  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

DWORD CALLBACK Process(void *data) {
  // todo maybe close these
  HANDLE 
    hOut = GetStdHandle(STD_OUTPUT_HANDLE),
    hIn = GetStdHandle(STD_INPUT_HANDLE);
  HWND hCon = GetConsoleWindow();

  BYTE mouseclick;
  
  CONSOLE_FONT_INFO info;
  POINT pt;

  COORD * fontSz = &info.dwFontSize;
  UCHAR key;

  __m128d vec;

  HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
  MSG mouseMsg;

	// TODO set raster font
	// 

  while(TRUE) {
    SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS & ~ENABLE_QUICK_EDIT_MODE);
    GetCurrentConsoleFont(hOut, FALSE, &info);

    GetCursorPos(&pt);
    ScreenToClient(hCon,&pt);

    // Get mouse click
    mouseclick = 
        GetKeyState(VK_LBUTTON) & 0x80 ? 1
      : GetKeyState(VK_RBUTTON) & 0x80 ? 2
      : 0;

    // swap mouse buttons if needed
    if(mouseclick && GetSystemMetrics(SM_SWAPBUTTON))
      mouseclick = mouseclick == 1? 1 : 2;

    vec = __builtin_ia32_roundpd(_mm_setr_pd(pt.x, pt.y) / _mm_setr_pd(fontSz->X, fontSz->Y), _MM_FROUND_CEIL) -1;

    ENV("mousexpos", itoa_((int)vec[0]));
    ENV("mouseypos", itoa_((int)vec[1]));

    if(hCon == GetForegroundWindow()) {
      ENV("click", itoa_(mouseclick));
      ENV("wheeldelta", itoa_(wheelDelta));

      key = my_getch();
      !key ?: ENV("keypressed", itoa_(InRange(key,0x40,0x5B)? key+0x20 : key)); // kill those caps
    }

    // No idea how it works but it just somehow does
    PeekMessage(&mouseMsg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE);
    Sleep(WAIT_TIME);
  }

  // Never returns
  return TRUE;
}

hook_t Hook;
fBeginPaint hookBp;
HFONT hfnt;

HDC HookBeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint) {
	HDC origReturn = hookBp(hWnd, lpPaint);
	MessageBox(NULL, "HookBeginPaint", "", MB_OK);
	SelectObject(origReturn, hfnt);
}

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
  if (dwReason == DLL_PROCESS_ATTACH) {
	DisableThreadLibraryCalls(hInst);
	hfnt = CreateFont(-8, -8, 0, 0, 0, 0, 0, 0, OEM_CHARSET, OUT_DEVICE_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Terminal");
	InitializeHook(&Hook, "user32.dll", "BeginPaint", HookBeginPaint);

	hookBp = (fBeginPaint)Hook.APIFunction;
	InsertHook(&Hook);
    CloseHandle(CreateThreadS(Process));
  }  else if(dwReason == DLL_PROCESS_DETACH) {
		Unhook(&Hook);
		FreeHook(&Hook);
	}
  return TRUE;
}
