/* Licensed under conditions stated in the
   "getinput.txt" document you recieved with this software.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <psapi.h>
#include "../bin/dllcode.h"
#include "extern/lz77.c"

// This is extremely ugly
// However its a) not mine and b) not getting edited again soon

// I somehow ported it to x64 and it works

typedef unsigned __int64 QWORD;

char unpackmem_getinput[2556] = {0};

typedef struct {
    PBYTE imageBase;
    HMODULE(WINAPI* loadLibraryA)(PCSTR);
    FARPROC(WINAPI* getProcAddress)(HMODULE, PCSTR);
    VOID(WINAPI* rtlZeroMemory)(PVOID, SIZE_T);
} LoaderData;

QWORD WINAPI loadLibrary(LoaderData* loaderData) {
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(loaderData->imageBase + ((PIMAGE_DOS_HEADER)loaderData->imageBase)->e_lfanew);
    PIMAGE_BASE_RELOCATION relocation = (PIMAGE_BASE_RELOCATION)(loaderData->imageBase + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
    QWORD delta = (QWORD)(loaderData->imageBase - ntHeaders->OptionalHeader.ImageBase);
    while (relocation->VirtualAddress) {
        PWORD relocationInfo = (PWORD)(relocation + 1);
        for (int i = 0, count = (relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD); i < count; i++)
            if (relocationInfo[i] >> 12 == IMAGE_REL_BASED_HIGHLOW)
                *(QWORD *)(loaderData->imageBase + (relocation->VirtualAddress + (relocationInfo[i] & 0xFFF))) += delta;

        relocation = (PIMAGE_BASE_RELOCATION)((LPBYTE)relocation + relocation->SizeOfBlock);
    }

    PIMAGE_IMPORT_DESCRIPTOR importDirectory = (PIMAGE_IMPORT_DESCRIPTOR)(loaderData->imageBase + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    while (importDirectory->Characteristics) {
        PIMAGE_THUNK_DATA originalFirstThunk = (PIMAGE_THUNK_DATA)(loaderData->imageBase + importDirectory->OriginalFirstThunk);
        PIMAGE_THUNK_DATA firstThunk = (PIMAGE_THUNK_DATA)(loaderData->imageBase + importDirectory->FirstThunk);

        HMODULE module = loaderData->loadLibraryA((LPCSTR)loaderData->imageBase + importDirectory->Name);

        if (!module)
            return FALSE;

        while (originalFirstThunk->u1.AddressOfData) {
            QWORD Function = (QWORD)loaderData->getProcAddress(module, originalFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG ? (LPCSTR)(originalFirstThunk->u1.Ordinal & 0xFFFF) : ((PIMAGE_IMPORT_BY_NAME)((LPBYTE)loaderData->imageBase + originalFirstThunk->u1.AddressOfData))->Name);

            if (!Function)
                return FALSE;

            firstThunk->u1.Function = Function;
            originalFirstThunk++;
            firstThunk++;
        }
        importDirectory++;
    }

    if (ntHeaders->OptionalHeader.AddressOfEntryPoint) {
        QWORD result = ((DWORD(__stdcall*)(HMODULE, DWORD, LPVOID))
            (loaderData->imageBase + ntHeaders->OptionalHeader.AddressOfEntryPoint))
            ((HMODULE)loaderData->imageBase, DLL_PROCESS_ATTACH, NULL);

        loaderData->rtlZeroMemory(loaderData->imageBase + ntHeaders->OptionalHeader.AddressOfEntryPoint, 32);
        loaderData->rtlZeroMemory(loaderData->imageBase, ntHeaders->OptionalHeader.SizeOfHeaders);
        return result;
    }
    return TRUE;
}

// only serves as a marker
void stub(void) {}

void HookDll(HANDLE hProcess, LPVOID dllcode) {
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(dllcode + ((PIMAGE_DOS_HEADER)dllcode)->e_lfanew);
    PBYTE executableImage = VirtualAllocEx(hProcess, NULL, ntHeaders->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    WriteProcessMemory(hProcess, executableImage, dllcode, ntHeaders->OptionalHeader.SizeOfHeaders, NULL);

    PIMAGE_SECTION_HEADER sectionHeaders = (PIMAGE_SECTION_HEADER)(ntHeaders + 1);
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
        WriteProcessMemory(hProcess, executableImage + sectionHeaders[i].VirtualAddress, dllcode + sectionHeaders[i].PointerToRawData, sectionHeaders[i].SizeOfRawData, NULL);

    LoaderData* loaderMemory = VirtualAllocEx(hProcess, NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READ);

    LoaderData loaderParams = {
    	.imageBase = executableImage,
    	.loadLibraryA = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"),
    	.getProcAddress = GetProcAddress,
    	.rtlZeroMemory = (VOID(NTAPI*)(PVOID, SIZE_T))GetProcAddress(GetModuleHandle("ntdll.dll"), "RtlZeroMemory")
    };

    WriteProcessMemory(hProcess, loaderMemory, &loaderParams, sizeof(LoaderData), NULL);
    WriteProcessMemory(hProcess, loaderMemory + 1, loadLibrary, (QWORD)stub - (QWORD)loadLibrary, NULL);
    WaitForSingleObject(CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)(loaderMemory + 1), loaderMemory, 0, NULL), INFINITE);
    VirtualFreeEx(hProcess, loaderMemory, 0, MEM_RELEASE);
}

CHAR b[21], *c;
PCHAR itoa_(i,x) {
  c=b+21,x=abs(i);
  do *--c = 48 + x % 10; while(x/=10); if(i<0) *--c = 45;
  return c;
}

// TODO error checking
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nShowCmd) {
	PROCESS_BASIC_INFORMATION ProcessInfo;
	if (!NT_SUCCESS(NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &ProcessInfo, sizeof(ProcessInfo), NULL)))
		return -3;

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, ProcessInfo.InheritedFromUniqueProcessId);

	TCHAR parentPath[MAX_PATH];
	GetModuleFileNameEx(hProcess, NULL, parentPath, MAX_PATH);
	if(lstrcmp("cmd.exe", parentPath + lstrlen(parentPath) - 7) != 0) {
		MessageBox(0, "Parrent process must be cmd.exe!", "", MB_OK | MB_ICONERROR);
		return -2;
	}

	//char *unpackmem_getinput = LocalAlloc(2556, LPTR);
	//char *unpackmem_discord = LocalAlloc(264772, LPTR);

	//MessageBox(NULL, "Here", "Here", MB_OK);
	lz77_decompress(getinput_dll_code, 1648, unpackmem_getinput, 2556);
	//WriteFile(CreateFile("test.bin", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL), unpackmem_getinput, 2556, NULL, NULL);
	//MessageBox(NULL, "Here", "Here", MB_OK);
	//lz77_decompress(discord_dll_code, 132075, unpackmem_discord, 264772);
	//MessageBox(NULL, "Here", "Here", MB_OK);

	HookDll(hProcess, unpackmem_getinput);
	//HookDll(hProcess, (LPVOID)unpackmem_discord);

	//LocalFree(unpackmem_getinput);
	//LocalFree(unpackmem_discord);
    return 0;
}
