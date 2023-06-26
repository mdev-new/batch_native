#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Injector.h"

DWORD CALLBACK Process(LPVOID data) {
	UNREFERENCED_PARAMETER(data);
	Sleep(100); // Let's let Rundll32 die

	while (TRUE) {
		Sleep(1000 / 40);
	}
}

BasicDllMainImpl(Process);