/* Licensed under conditions stated in the
   "getinput.txt" document you recieved with this software.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <psapi.h>

/* return codes:
  -1: thread not created
  -2: parent not cmd.exe
  -3: cant query parent proc info
*/

VOID HookDll(HANDLE hProcHeap, HANDLE hProcess, LPCWSTR name) {
  char *output = HeapAlloc(hProcHeap, HEAP_ZERO_MEMORY, lstrlenW(name)+2);
  WideCharToMultiByte(CP_ACP, 0, name, lstrlenW(name), output, lstrlenW(name)+1, NULL, NULL);

  LPVOID lpBaseAddress = VirtualAllocEx(hProcess, NULL, lstrlen(output), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  WriteProcessMemory(hProcess, lpBaseAddress, output, lstrlen(output), NULL);

  CloseHandle(CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"), lpBaseAddress, 0, NULL));
  HeapFree(hProcHeap, 0, output);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
  TCHAR parentPath[MAX_PATH] = {0};

  PROCESS_BASIC_INFORMATION ProcessInfo;
  if (!NT_SUCCESS(NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &ProcessInfo, sizeof(ProcessInfo), NULL)))
    return -3;

  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, ProcessInfo.InheritedFromUniqueProcessId);

  GetModuleFileNameEx(hProcess, NULL, parentPath, MAX_PATH);
  if(lstrcmp("cmd.exe", parentPath + lstrlen(parentPath) - 7) != 0) {
    MessageBox(0, "Parrent process must be cmd.exe!", "", MB_OK | MB_ICONINFORMATION);
    return -2;
  }

  HANDLE hProcHeap = GetProcessHeap();

  int argc = 0;
  LPCWSTR *args = CommandLineToArgvW(GetCommandLineW(), &argc);
  //WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), args[1], lstrlenW(args[1]), NULL, NULL);

  HookDll(hProcHeap, hProcess, args[1]);

  CloseHandle(hProcess);
  CloseHandle(hProcHeap);

  // return 0 if no errors
  return 0; // error checking on batch side
}
