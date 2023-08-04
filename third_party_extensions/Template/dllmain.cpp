#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "Injector.h"

DWORD CALLBACK Process(LPVOID data) {
	UNREFERENCED_PARAMETER(data);

	while (TRUE) {
		Sleep(1000 / 40);
	}

	return 0;
}

BasicDllMainImpl(Process);