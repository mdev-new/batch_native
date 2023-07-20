#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tlhelp32.h>
#include "Utilities.h"

#ifdef __cplusplus
#define NOMANGLE extern "C"
#else
#define NOMANGLE
#endif

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

DWORD getconhost() {
	HANDLE hSnapshot, hProc, hSelf;
	PROCESSENTRY32 pe32;
	SYSTEMTIME sConhostTime, sSelfTime;
	FILETIME fProcessTime, ftExit, ftKernel, ftUser;
	DWORD pid = 0;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) goto cleanup;

	ZeroMemory(&pe32, sizeof(pe32));
	pe32.dwSize = sizeof(pe32);
	if (!Process32First(hSnapshot, &pe32)) goto cleanup;

	hSelf = GetCurrentProcess();
	GetProcessTimes(hSelf, &fProcessTime, &ftExit, &ftKernel, &ftUser);
	FileTimeToSystemTime(&fProcessTime, &sSelfTime);

	do {
		if (lstrcmp(pe32.szExeFile, "conhost.exe") == 0) {
			hProc = OpenProcess(PROCESS_QUERY_INFORMATION, TRUE, pe32.th32ProcessID);
			GetProcessTimes(hProc, &fProcessTime, &ftExit, &ftKernel, &ftUser);
			FileTimeToSystemTime(&fProcessTime, &sConhostTime);
			CloseHandle(hProc);

			if (sConhostTime.wSecond == sSelfTime.wSecond) {
				pid = pe32.th32ProcessID;
				break;
			}
		}
	} while (Process32Next(hSnapshot, &pe32));

cleanup:
	if (hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(hSnapshot);

	return pid;
}

void hookSelfToProcess(DWORD pid) {
	HMODULE hSelf = NULL;
	char filename[MAX_PATH];
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&getppid, &hSelf);
	GetModuleFileName(hSelf, filename, MAX_PATH);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);

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

#define BasicDllMainImpl(ThreadProcName) \
NOMANGLE __declspec(dllexport) BOOL APIENTRY DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved) {\
	char name[MAX_PATH];\
	GetModuleFileName(NULL, name, sizeof name);\
	if(lstrcmp("rundll32.exe", name + lstrlen(name) - 12) == 0) return TRUE;\
	else if (dwReason == DLL_PROCESS_ATTACH) {\
		if(lstrcmp("conhost.exe", name + lstrlen(name) - 11) == 0) {\
			MessageBox(NULL, "in conhost", "info", 0);\
			MessageBox(NULL, itoa_(GetCurrentProcessId()), "pid", 0);\
			DisableThreadLibraryCalls(hMod);\
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProcName, NULL, 0, NULL);\
		} else {\
			/* 2nd stage injector */ \
			DWORD conhost = getconhost();\
			MessageBox(NULL, itoa_(conhost), "2nd stage, in cmd", 0);\
			hookSelfToProcess(conhost);\
		}\
	}\
	return TRUE;\
}

HMODULE getDllInstanceInProcess(DWORD pid, LPCSTR name) {
	HANDLE hSnapshot;
	MODULEENTRY32 me32;

	HMODULE result = NULL;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (hSnapshot == INVALID_HANDLE_VALUE) goto cleanup;

	ZeroMemory(&me32, sizeof(me32));
	me32.dwSize = sizeof(me32);

	if (!Module32First(hSnapshot, &me32)) goto cleanup;

	do {
		if (lstrcmp(me32.szModule, name) == 0) {
			result = me32.hModule;
		}
	} while (Module32Next(hSnapshot, &me32));

cleanup:
	if (hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(hSnapshot);

	return result;
}

NOMANGLE __declspec(dllexport) void CALLBACK inject(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow) {
	hookSelfToProcess(getppid());
}