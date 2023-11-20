#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Injector.h"
#include "Utilities.h"

DWORD CALLBACK Process(LPVOID data) {
	UNREFERENCED_PARAMETER(data);
	HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);

	DWORD mode;

	while (TRUE) {
		SetConsoleTitle(itoa_((GetConsoleMode(hConsole, &mode), mode)));
	}

	return 0;
}

BasicDllMainImpl(Process);