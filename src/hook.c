/* Licensed under conditions stated in the
   "getinput.txt" document you recieved with this software.
*/

#pragma GCC diagnostic ignored "-Wmultichar"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <psapi.h>
#include <shlwapi.h>
#include <shellapi.h>
#include "extern/lz77.h"

#define SINFL_IMPLEMENTATION
#include "extern/sinfl.h"
#include "shared.h"

// stolen from some random ass tutorial or yt video
// and ported to x64

// todo broken (atleast on win7)

typedef unsigned __int64 QWORD;

enum {
  ERROR_NOT_CMDEXE = -2,
  ERROR_CANNOT_GET_PARENT_PROC = -3,
  ERROR_FILE_DOESNT_EXIST_OR_INVALID_ARGS = -4,
  ERROR_CANNOT_OPEN_FILE = -5,
  ERROR_EMPTY_FILE = -6,
  ERROR_CANNOT_READ_FILE = -7,
  ERROR_CANNOT_ALLOCATE = -8,
  ERROR_CANNOT_FREE = -9,
  ERROR_CANNOT_DECOMPRESS = -10,
  ERROR_CANNOT_HOOK = -11,
  ERROR_CANNOT_WRITE_PROCESS_MEM = -12,
  ERROR_CANNOT_VIRTUAL_ALLOC = -13,
  ERROR_CANNOT_VIRTUAL_FREE = -14,
  ERROR_CANNOT_OPEN_PROCESS = -15,
  ERROR_CANNOT_OPEN_HEAP = -15,
  ERROR_CANNOT_GET_PARENT_PROC_PATH = -16,
  ERROR_INVALID_FILE = -17
};

typedef struct {
    PBYTE imageBase;
    HMODULE(WINAPI* loadLibraryA)(PCSTR);
    FARPROC(WINAPI* getProcAddress)(HMODULE, PCSTR);
    VOID(WINAPI* rtlZeroMemory)(PVOID, SIZE_T);
} LoaderData;

// not even gonna attempt to error check that
// also this runs inside cmd.exe
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

        //loaderData->rtlZeroMemory(loaderData->imageBase + ntHeaders->OptionalHeader.AddressOfEntryPoint, 32);
        //loaderData->rtlZeroMemory(loaderData->imageBase, ntHeaders->OptionalHeader.SizeOfHeaders);
        return result;
    }
    return TRUE;
}

// only serves as a marker
void stub(void) {}

// todo error checking?
INT HookDll(HANDLE hProcess, LPVOID dllcode) {
  PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(dllcode + ((PIMAGE_DOS_HEADER)dllcode)->e_lfanew);
  PBYTE executableImage = VirtualAllocEx(hProcess, NULL, ntHeaders->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if(executableImage == NULL) return ERROR_CANNOT_VIRTUAL_ALLOC;
  if(WriteProcessMemory(hProcess, executableImage, dllcode, ntHeaders->OptionalHeader.SizeOfHeaders, NULL) == 0) return ERROR_CANNOT_WRITE_PROCESS_MEM;

  PIMAGE_SECTION_HEADER sectionHeaders = (PIMAGE_SECTION_HEADER)(ntHeaders + 1);
  for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
  		// this is weird. printf doesnt print however return does return.
  	// i have no idea whats going on
  	// the compiler is on -Og and still optimizes away this branch????
  	// seems like error checking brings more harm than good
      //if(
        WriteProcessMemory(hProcess, executableImage + sectionHeaders[i].VirtualAddress, dllcode + sectionHeaders[i].PointerToRawData, sectionHeaders[i].SizeOfRawData, NULL);
      //== 0) printf("%d\n", GetLastError());
    }

  //MessageBox(NULL, "before virtualalloce", "", 0);

  LoaderData* loaderMemory = VirtualAllocEx(hProcess, NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if(loaderMemory == NULL) return ERROR_CANNOT_VIRTUAL_ALLOC;

  LoaderData loaderParams = {
    .imageBase = executableImage,
    .loadLibraryA = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"),
    .getProcAddress = GetProcAddress(GetModuleHandle("kernel32.dll"), "GetProcAddress"),
    .rtlZeroMemory = GetProcAddress(GetModuleHandle("ntdll.dll"), "RtlZeroMemory")
  };

  if(WriteProcessMemory(hProcess, loaderMemory, &loaderParams, sizeof(LoaderData), NULL) == 0) return ERROR_CANNOT_WRITE_PROCESS_MEM;
  if(WriteProcessMemory(hProcess, loaderMemory + 1, loadLibrary, (QWORD)stub - (QWORD)loadLibrary, NULL) == 0) return ERROR_CANNOT_WRITE_PROCESS_MEM;
  WaitForSingleObject(CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)(loaderMemory + 1), loaderMemory, 0, NULL), INFINITE);
  if(!VirtualFreeEx(hProcess, loaderMemory, 0, MEM_RELEASE)) return ERROR_CANNOT_VIRTUAL_FREE;
}

// todo definitely error check
INT LoadAndHook(HANDLE hProcHeap, HANDLE hProcess, LPCWSTR name) {
  HANDLE hFile = CreateFileW(name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if(hFile == NULL) return ERROR_CANNOT_OPEN_FILE;

  DWORD fileSize = GetFileSize(hFile, NULL), numOfBytesRead = 0;
  if(fileSize == 0) return ERROR_EMPTY_FILE;

  BYTE *buffer = HeapAlloc(hProcHeap, HEAP_ZERO_MEMORY, fileSize);
  if(!ReadFile(hFile, buffer, fileSize, &numOfBytesRead, NULL)) return ERROR_CANNOT_READ_FILE;

	Header *header = (Header *)buffer;
	if(header->magic != MAGIC) return ERROR_INVALID_FILE;
	int retcode = 0;

	switch(header->compression & 0b1111) {
	case UNCOMPRESSED: HookDll(hProcess, buffer+header->sizeOfSelf); break;
	case ALGO_LZ77: {
		BYTE *decompBuffer = HeapAlloc(hProcHeap, HEAP_ZERO_MEMORY, header->uncompressed_file_size+2);
		if(decompBuffer == NULL) return ERROR_CANNOT_ALLOCATE;

		int decomp = lz77_decompress(buffer+header->sizeOfSelf, fileSize-header->sizeOfSelf, decompBuffer, header->uncompressed_file_size+1, header->compression >> 27);
		if(decomp != header->uncompressed_file_size) return ERROR_CANNOT_DECOMPRESS;
		retcode = HookDll(hProcess, decompBuffer) == header->uncompressed_file_size ?:ERROR_CANNOT_HOOK;
		HeapFree(hProcHeap, 0, decompBuffer);
	}; break;
	case ALGO_DEFLATE: {
		BYTE *decompBuffer = HeapAlloc(hProcHeap, HEAP_ZERO_MEMORY, header->uncompressed_file_size+2);
		if(decompBuffer == NULL) return ERROR_CANNOT_ALLOCATE;

		int decomp = sinflate(decompBuffer, header->uncompressed_file_size+1, buffer+header->sizeOfSelf, fileSize-header->sizeOfSelf);
		if(decomp != header->uncompressed_file_size) return ERROR_CANNOT_DECOMPRESS;
		retcode = HookDll(hProcess, decompBuffer) == header->uncompressed_file_size ?:ERROR_CANNOT_HOOK;

		HeapFree(hProcHeap, 0, decompBuffer);
	}; break;
	}

  if(HeapFree(hProcHeap, 0, buffer) == 0) return ERROR_CANNOT_FREE;
  CloseHandle(hFile);
  return retcode;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
  int argc = 0, retcode = 0;
  LPCWSTR *args = CommandLineToArgvW(GetCommandLineW(), &argc);

  if(argc <= 0) {
    return ERROR_FILE_DOESNT_EXIST_OR_INVALID_ARGS;
  }

  TCHAR parentPath[MAX_PATH] = {0};

  NTSTATUS(*NtQueryInformationProcessProc)(HANDLE, PROCESSINFOCLASS, void*, ULONG, ULONG*) = GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationProcess");
  if(NtQueryInformationProcessProc == NULL) return ERROR_CANNOT_GET_PARENT_PROC;

  PROCESS_BASIC_INFORMATION ProcessInfo;
  if (!NT_SUCCESS(NtQueryInformationProcessProc(GetCurrentProcess(), ProcessBasicInformation, &ProcessInfo, sizeof(ProcessInfo), NULL)))
    return ERROR_CANNOT_GET_PARENT_PROC;

  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, ProcessInfo.InheritedFromUniqueProcessId);
  if(hProcess == NULL) return ERROR_CANNOT_OPEN_PROCESS;

  BOOL bForce = FALSE;
  if(lstrcmpW(args[1], L"-force") == 0) bForce = TRUE;

  if(bForce == FALSE) {
    if(GetModuleFileNameEx(hProcess, NULL, parentPath, MAX_PATH) == 0) return ERROR_CANNOT_GET_PARENT_PROC_PATH;
    if(lstrcmp("cmd.exe", parentPath + lstrlen(parentPath) - 7) != 0) {
      MessageBox(0, "Parrent process must be cmd.exe!", "Error", MB_OK | MB_ICONINFORMATION);
      return ERROR_NOT_CMDEXE;
    }
  }

  HANDLE hProcHeap = GetProcessHeap();
  if(hProcHeap == NULL) return ERROR_CANNOT_OPEN_HEAP;

  if(PathFileExistsW(args[bForce? 2 : 1]))
    retcode = LoadAndHook(hProcHeap, hProcess, args[bForce? 2 : 1]);
  else retcode = ERROR_FILE_DOESNT_EXIST_OR_INVALID_ARGS;

  CloseHandle(hProcess);
  CloseHandle(hProcHeap);

  return retcode; // error checking on batch side
}
