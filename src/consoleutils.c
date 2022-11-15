#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

wchar_t *wcscpy(wchar_t * restrict s1, const wchar_t * restrict s2) {
	wchar_t *cp = s1;
	while ((*cp++ = *s2++) != L'\0');
	return s1;
}

DWORD CALLBACK Process(void *) {
  // todo maybe close these
  HANDLE 
    hOut = GetStdHandle(STD_OUTPUT_HANDLE),
    hIn = GetStdHandle(STD_INPUT_HANDLE);
  HWND hCon = GetConsoleWindow();

	CONSOLE_FONT_INFOEX cfi;
  cfi.cbSize = sizeof(cfi);
  cfi.nFont = 0;
  cfi.dwFontSize.X = 8;
  cfi.dwFontSize.Y = 8;
  cfi.FontFamily = FF_DONTCARE;
  cfi.FontWeight = FW_NORMAL;
  wcscpy(cfi.FaceName, L"Terminal"); // Any invalid face name will do
  SetCurrentConsoleFontEx(hOut, FALSE, &cfi);

	DWORD style = GetWindowLong(hCon, GWL_STYLE);
	SetWindowLong(hCon, GWL_STYLE, style & ~(WS_SIZEBOX | WS_MAXIMIZEBOX));

	return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
  if (dwReason == DLL_PROCESS_ATTACH) {
	  DisableThreadLibraryCalls(hInst);
    CloseHandle(CreateThread(0,0,Process,0,0,0));
  }
  return TRUE;
}
