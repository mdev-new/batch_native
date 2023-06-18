#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <math.h>

#define CreateThreadS(funptr) CreateThread(0,0,funptr,0,0,0)

#define TICKS_PER_SECOND 40

DWORD CALLBACK Process(LPVOID data) {
	(void)data; // mark as not used

	double cosN;
	char buffer[64] = {0};

	while(TRUE) {
		cosN = 0.;
		snprintf(buffer, 63, "%f", cos(cosN));

		SetEnvironmentVariable("cosO", buffer);

		Sleep(1000 / TICKS_PER_SECOND);
	}

	__builtin_unreachable();
}

DWORD WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpReserved) {
	if(fdwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hInstance);
		CreateThreadS(Process);
	}

	return TRUE;
}
