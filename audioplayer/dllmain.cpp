#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "Injector.h"

#include "audio.hpp"

#define MAXLEN 512

DWORD CALLBACK RunAudioQueue(LPVOID data) {
    HANDLE hPipe;
    DWORD dwRead;

    Audio::SndOutStream snd;

    std::string buffer;
    buffer.reserve(MAXLEN);

    auto audio = Audio::AudioFile();

    hPipe = CreateNamedPipe((LPCSTR)data,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, // | FILE_FLAG_FIRST_PIPE_INSTANCE,
        1,
        0,
        MAXLEN,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL);

    while (hPipe != INVALID_HANDLE_VALUE) {
        if (ConnectNamedPipe(hPipe, NULL) != FALSE) {   // wait for someone to connect to the pipe
            while (ReadFile(hPipe, buffer.data(), MAXLEN - 1, &dwRead, NULL) != FALSE) {
                /* add terminating zero */
				if(dwRead >= 1) {
					buffer[dwRead-1] = '\0';

					if(strncmp(buffer, "play", 4) == 0) {
						audio.SetFile(buffer+5);
						snd.Play(audio);
					}

					if(strncmp(buffer, "stop", 4) == 0) {
						snd.Stop();
					}

					if(strncmp(buffer, "pause", 5) == 0) {} // todo pause
					if(strncmp(buffer, "resume", 6) == 0) {} // todo resume
				}
            }
        }

        DisconnectNamedPipe(hPipe);
    }

    return 0;
}

// manual dllmain since i dont need one process thread, i need many of them
extern "C" __declspec(dllexport) BOOL __stdcall DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
    hDll = hInst;
    char name[260];
    GetModuleFileNameA(0, name, sizeof name);

    if (lstrcmpA("rundll32.exe", name + lstrlenA(name) - 12) == 0) {
        return TRUE;
    }

    if (dwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hInst);

        CreateThread(NULL, 0, RunAudioQueue, (LPVOID)TEXT("\\\\.\\pipe\\BatAudQ1"), 0, NULL);
        CreateThread(NULL, 0, RunAudioQueue, (LPVOID)TEXT("\\\\.\\pipe\\BatAudQ2"), 0, NULL);
        CreateThread(NULL, 0, RunAudioQueue, (LPVOID)TEXT("\\\\.\\pipe\\BatAudQ3"), 0, NULL);
        CreateThread(NULL, 0, RunAudioQueue, (LPVOID)TEXT("\\\\.\\pipe\\BatAudQ4"), 0, NULL);
        CreateThread(NULL, 0, RunAudioQueue, (LPVOID)TEXT("\\\\.\\pipe\\BatAudQ5"), 0, NULL);
        CreateThread(NULL, 0, RunAudioQueue, (LPVOID)TEXT("\\\\.\\pipe\\BatAudQ6"), 0, NULL);

    }

    return TRUE;
};