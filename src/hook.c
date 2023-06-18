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
// and modified

typedef unsigned __int64 QWORD;

#define MakeError() (1LL << __COUNTER__)

enum {
	ERROR_NOT_CMDEXE = MakeError(), //1
	ERROR_CANNOT_GET_PARENT_PROC = MakeError(), //2
	ERROR_FILE_DOESNT_EXIST_OR_INVALID_ARGS = MakeError(), //4
	ERROR_CANNOT_OPEN_FILE = MakeError(), //16
	ERROR_EMPTY_FILE = MakeError(),//32
	ERROR_CANNOT_READ_FILE = MakeError(),//64
	ERROR_CANNOT_ALLOCATE = MakeError(),//128
	ERROR_CANNOT_FREE = MakeError(),//256
	ERROR_CANNOT_DECOMPRESS = MakeError(),//512
	ERROR_CANNOT_HOOK = MakeError(),//1024
	ERROR_CANNOT_WRITE_PROCESS_MEM = MakeError(),//2048
	ERROR_CANNOT_VIRTUAL_ALLOC = MakeError(),//4096
	ERROR_CANNOT_VIRTUAL_FREE = MakeError(),//8192
	ERROR_CANNOT_OPEN_PROCESS = MakeError(),//16384
	ERROR_CANNOT_OPEN_HEAP = MakeError(),//32768
	ERROR_CANNOT_GET_PARENT_PROC_PATH = MakeError(),//65536
	ERROR_INVALID_FILE = MakeError()
};

typedef struct {
	BYTE * imageBase;
	HMODULE(WINAPI* loadLibraryA)(PCSTR);
	QWORD(WINAPI* getProcAddress)(HMODULE, PCSTR);
	VOID(WINAPI* rtlZeroMemory)(PVOID, SIZE_T);
} LoaderData;

typedef DWORD(__stdcall* DllEntry)(HMODULE, DWORD, LPVOID);

#define dbg(message) MessageBoxA(NULL, message, "dbg", 0);

// not even gonna attempt to error check that
// also this runs inside cmd.exe
DWORD WINAPI loadLibrary(LoaderData* loaderData) {
	IMAGE_NT_HEADERS * ntHeaders = (IMAGE_NT_HEADERS *)(loaderData->imageBase + ((IMAGE_DOS_HEADER *)loaderData->imageBase)->e_lfanew);
	IMAGE_BASE_RELOCATION * relocation = (IMAGE_BASE_RELOCATION *)(loaderData->imageBase + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
	QWORD delta = (QWORD)(loaderData->imageBase - ntHeaders->OptionalHeader.ImageBase);

	while (relocation->VirtualAddress) {
		PWORD relocationInfo = (PWORD)(relocation + 1);
		for (int i = 0, count = (relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD); i < count; i++) {
			if (relocationInfo[i] >> 12 == IMAGE_REL_BASED_HIGHLOW) {
				*(QWORD *)(loaderData->imageBase + (relocation->VirtualAddress + (relocationInfo[i] & 0xFFF))) += delta;
			}
		}

		relocation = (IMAGE_BASE_RELOCATION *)((BYTE *)relocation + relocation->SizeOfBlock);
	}

	IMAGE_IMPORT_DESCRIPTOR * importDirectory = (IMAGE_IMPORT_DESCRIPTOR *)(loaderData->imageBase + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	while (importDirectory->Characteristics) {
		IMAGE_THUNK_DATA * originalFirstThunk = (IMAGE_THUNK_DATA *)(loaderData->imageBase + importDirectory->OriginalFirstThunk);
		IMAGE_THUNK_DATA * firstThunk = (IMAGE_THUNK_DATA *)(loaderData->imageBase + importDirectory->FirstThunk);

		HMODULE module = loaderData->loadLibraryA((LPCSTR)loaderData->imageBase + importDirectory->Name);

		if (!module) {
			return FALSE;
		}

		while (originalFirstThunk->u1.AddressOfData) {
			char * funcName =
			(originalFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
				? (LPCSTR)(originalFirstThunk->u1.Ordinal & 0xFFFF)
				: ((PIMAGE_IMPORT_BY_NAME)((LPBYTE)loaderData->imageBase + originalFirstThunk->u1.AddressOfData))->Name;

			QWORD Function = (QWORD)loaderData->getProcAddress(module, funcName);

			if (!Function) {
				return FALSE;
			}

			firstThunk->u1.Function = Function;
			originalFirstThunk++;
			firstThunk++;
		}

		importDirectory++;
	}

	if (ntHeaders->OptionalHeader.AddressOfEntryPoint) {
		DllEntry entry = (loaderData->imageBase + ntHeaders->OptionalHeader.AddressOfEntryPoint);
		DWORD result = entry((HMODULE)loaderData->imageBase, DLL_PROCESS_ATTACH, NULL);
		return result;
	}

	return TRUE;
}

// only serves as a marker
void stub(void) {}

INT HookDll(HANDLE hProcess, LPVOID dllcode) {
	dbg("0");
	IMAGE_NT_HEADERS * ntHeaders = (IMAGE_NT_HEADERS *)(dllcode + ((IMAGE_DOS_HEADER *)dllcode)->e_lfanew);
	BYTE * executableImage = VirtualAllocEx(hProcess, NULL, ntHeaders->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if(executableImage == NULL) return ERROR_CANNOT_VIRTUAL_ALLOC;
	if(WriteProcessMemory(hProcess, executableImage, dllcode, ntHeaders->OptionalHeader.SizeOfHeaders, NULL) == 0) return ERROR_CANNOT_WRITE_PROCESS_MEM;

	dbg("1");

	PIMAGE_SECTION_HEADER sectionHeaders = (PIMAGE_SECTION_HEADER)(ntHeaders + 1);
	for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
		// maybe err chk
		WriteProcessMemory(hProcess, executableImage + sectionHeaders[i].VirtualAddress, dllcode + sectionHeaders[i].PointerToRawData, sectionHeaders[i].SizeOfRawData, NULL);
	}

	dbg("2");

	LoaderData* loaderMemory = VirtualAllocEx(hProcess, NULL, sizeof(LoaderData), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if(loaderMemory == NULL) return ERROR_CANNOT_VIRTUAL_ALLOC;

	void* loadLibInCmd = VirtualAllocEx(hProcess, NULL, (QWORD)stub - (QWORD)loadLibrary, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if(loadLibInCmd == NULL) return ERROR_CANNOT_VIRTUAL_ALLOC;

	dbg("3");

	LoaderData loaderParams = {
		.imageBase = executableImage,
		.loadLibraryA = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"),
		.getProcAddress = GetProcAddress(GetModuleHandle("kernel32.dll"), "GetProcAddress"),
		.rtlZeroMemory = GetProcAddress(GetModuleHandle("ntdll.dll"), "RtlZeroMemory")
	};

	if(!WriteProcessMemory(hProcess, loaderMemory, &loaderParams, sizeof(LoaderData), NULL)) return ERROR_CANNOT_WRITE_PROCESS_MEM;
	dbg("4");
	if(!WriteProcessMemory(hProcess, loadLibInCmd, loadLibrary, (QWORD)stub - (QWORD)loadLibrary, NULL)) return ERROR_CANNOT_WRITE_PROCESS_MEM;
	dbg("5");

	WaitForSingleObject(CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)(loadLibInCmd), loaderMemory, 0, NULL), INFINITE);
	dbg("6");

	if(!VirtualFreeEx(hProcess, loaderMemory, 0, MEM_RELEASE)) return ERROR_CANNOT_VIRTUAL_FREE;

	dbg("we survived");
	return 0;
}

INT LoadAndHook(HANDLE hProcHeap, HANDLE hProcess, LPCWSTR name) {
	HANDLE hFile = CreateFileW(name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == NULL) return ERROR_CANNOT_OPEN_FILE;

	DWORD fileSize = GetFileSize(hFile, NULL), numOfBytesRead = 0;
	if(fileSize == 0) return ERROR_EMPTY_FILE;

	BYTE *buffer = HeapAlloc(hProcHeap, HEAP_ZERO_MEMORY, fileSize);
	if(!ReadFile(hFile, buffer, fileSize, &numOfBytesRead, NULL)) return ERROR_CANNOT_READ_FILE;

	if(buffer[0] == 'M' && buffer[1] == 'Z') {
		MessageBox(NULL, "Injector doesn't accept plain PE DLL files.", "You should've read the documentation!", MB_ICONWARNING | MB_OK);
		return ERROR_INVALID_FILE;
	}

	Header *header = (Header *)buffer;
	if(header->magic != MAGIC) return ERROR_INVALID_FILE;
	int retcode = 0;

	switch(header->compression & 0b1111) {
	case UNCOMPRESSED: {
		retcode = HookDll(hProcess, buffer+header->sizeOfSelf);
		break;
	};
	case ALGO_LZ77: {
		BYTE *decompBuffer = HeapAlloc(hProcHeap, HEAP_ZERO_MEMORY, header->uncompressed_file_size+2);
		if(decompBuffer == NULL) return ERROR_CANNOT_ALLOCATE;

		int decomp = lz77_decompress(buffer+header->sizeOfSelf, fileSize-header->sizeOfSelf, decompBuffer, header->uncompressed_file_size+1, header->compression >> 27);
		if(decomp != header->uncompressed_file_size) return ERROR_CANNOT_DECOMPRESS;

		retcode = HookDll(hProcess, decompBuffer);
		if(retcode) break;

		if(!HeapFree(hProcHeap, 0, decompBuffer)) retcode = ERROR_CANNOT_FREE;
		break;
	};
	case ALGO_DEFLATE: {
		BYTE *decompBuffer = HeapAlloc(hProcHeap, HEAP_ZERO_MEMORY, header->uncompressed_file_size+2);
		if(decompBuffer == NULL) return ERROR_CANNOT_ALLOCATE;

		int decomp = sinflate(decompBuffer, header->uncompressed_file_size+1, buffer+header->sizeOfSelf, fileSize-header->sizeOfSelf);
		if(decomp != header->uncompressed_file_size) return ERROR_CANNOT_DECOMPRESS;

		retcode = HookDll(hProcess, decompBuffer);
		if(retcode) break;

		if(!HeapFree(hProcHeap, 0, decompBuffer)) retcode = ERROR_CANNOT_FREE;
		break;
	};
	}

	if(!HeapFree(hProcHeap, 0, buffer)) retcode = ERROR_CANNOT_FREE;
	CloseHandle(hFile);
	return retcode;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	int argc = 0, retcode = 0;
	BOOL bForce = FALSE;
	LPCWSTR *args = CommandLineToArgvW(GetCommandLineW(), &argc);

	if(argc < 1) {
		return ERROR_FILE_DOESNT_EXIST_OR_INVALID_ARGS;
	}

	if(lstrcmpW(args[1], L"-force") == 0) bForce = TRUE;

	if(bForce && argc < 2) return ERROR_FILE_DOESNT_EXIST_OR_INVALID_ARGS;

	TCHAR parentPath[MAX_PATH] = {0};

	NTSTATUS(*NtQueryInformationProcessProc)(HANDLE, PROCESSINFOCLASS, void*, ULONG, ULONG*) = GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationProcess");
	if(NtQueryInformationProcessProc == NULL) return ERROR_CANNOT_GET_PARENT_PROC;

	PROCESS_BASIC_INFORMATION ProcessInfo;
	if (!NT_SUCCESS(NtQueryInformationProcessProc(GetCurrentProcess(), ProcessBasicInformation, &ProcessInfo, sizeof(ProcessInfo), NULL)))
		return ERROR_CANNOT_GET_PARENT_PROC;

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, ProcessInfo.InheritedFromUniqueProcessId);
	if(hProcess == NULL) return ERROR_CANNOT_OPEN_PROCESS;

	if(!bForce) {
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

	return retcode;
}
