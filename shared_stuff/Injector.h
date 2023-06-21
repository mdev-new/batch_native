#ifdef __cplusplus
extern "C" {
#endif

#include <winternl.h>
#include <psapi.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <tlhelp32.h>

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

__declspec(dllexport) void CALLBACK inject(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow) {
	HMODULE hSelf = NULL;
	char filename[MAX_PATH];
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&getppid, &hSelf);
	GetModuleFileName(hSelf, filename, MAX_PATH);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, getppid());

	LPVOID lpBaseAddress = VirtualAllocEx(hProcess, NULL, lstrlen(filename), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	WriteProcessMemory(hProcess, lpBaseAddress, filename, lstrlen(filename), NULL);

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"), lpBaseAddress, 0, NULL);
	CloseHandle(hThread);
	CloseHandle(hProcess);
	return;
}

#ifdef __cplusplus
} // extern "C"
#endif
