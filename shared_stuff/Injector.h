#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tlhelp32.h>

#ifdef __cplusplus
#define NOMANGLE extern "C"
#else
#define NOMANGLE
#endif

#define BasicDllMainImpl(ThreadProcName) \
NOMANGLE __declspec(dllexport) BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {\
	char name[MAX_PATH];\
	GetModuleFileName(NULL, name, sizeof name);\
	if(lstrcmp("rundll32.exe", name + lstrlen(name) - 12) == 0) return TRUE;\
\
	if (dwReason == DLL_PROCESS_ATTACH) {\
		DisableThreadLibraryCalls(hInst);\
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProcName, NULL, 0, NULL);\
	}\
	return TRUE;\
}

DWORD getppid() {
	HANDLE hSnapshot;
	PROCESSENTRY32 pe32;
	DWORD ppid = 0, pid = GetCurrentProcessId();

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) goto cleanup;

	ZeroMemory(&pe32, sizeof(pe32));
	pe32.dwSize = sizeof(pe32);
	if (!Process32First(hSnapshot, &pe32)) goto cleanup;

	do {
		if (pe32.th32ProcessID == pid) {
			ppid = pe32.th32ParentProcessID;
			break;
		}
	} while (Process32Next(hSnapshot, &pe32));

cleanup:
	if (hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(hSnapshot);

	return ppid;
}


NOMANGLE __declspec(dllexport) void CALLBACK inject(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow) {
	HMODULE hSelf = NULL;
	char filename[MAX_PATH];
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&getppid, &hSelf);
	GetModuleFileName(hSelf, filename, MAX_PATH);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, getppid());

	int filename_len = lstrlen(filename);

	LPVOID lpBaseAddress = VirtualAllocEx(hProcess, NULL, filename_len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	WriteProcessMemory(hProcess, lpBaseAddress, filename, filename_len, NULL);

	LPTHREAD_START_ROUTINE startAddr = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, startAddr, lpBaseAddress, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	CloseHandle(hProcess);
	return;
}