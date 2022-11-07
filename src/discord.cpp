#undef UNICODE

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <stdlib.h>

#include "discord_rpc.h"

#include <Windows.h>
#include <mutex>

char *readenv(const char *name) {
	static TCHAR buffer[127];
	GetEnvironmentVariable(name, buffer, 127);
	return strdup(buffer); // memory leaks go brr
}

volatile BOOL shouldShutdown = FALSE;
std::mutex m;

BOOL WINAPI ConsoleCloseHandler(DWORD dwCtrlType) {
	if(dwCtrlType >= 1 && dwCtrlType <= 6) {
		m.lock();
		shouldShutdown = TRUE;
		m.unlock();
		Sleep(550); // possibly let the last loop run
	}
	return TRUE;
}

DWORD Process() {
	if(GetEnvironmentVariable("discordappid", NULL, 0) == 0) return TRUE;
	SetConsoleCtrlHandler(ConsoleCloseHandler, TRUE);

    DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	Discord_Initialize(readenv("discordappid"), &handlers, 1, NULL);

    DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.state = readenv("discordstate");
	discordPresence.details = readenv("discorddetails");
	discordPresence.startTimestamp = time(0);
	discordPresence.endTimestamp = NULL;
	discordPresence.largeImageKey = readenv("discordlargeimg");
	discordPresence.largeImageText = readenv("discordlargeimgtxt");
	discordPresence.smallImageKey = readenv("discordsmallimg");
	discordPresence.smallImageText = readenv("discordsmallimgtxt");
	discordPresence.instance = 1;
	Discord_UpdatePresence(&discordPresence);

    while(1) {
		if (GetEnvironmentVariable("discordupdate", NULL, 0)) {
			SetEnvironmentVariable("discordupdate", NULL);
			discordPresence.state = readenv("discordstate");
			discordPresence.details = readenv("discorddetails");
			discordPresence.largeImageKey = readenv("discordlargeimg");
			discordPresence.largeImageText = readenv("discordlargeimgtxt");
			discordPresence.smallImageKey = readenv("discordsmallimg");
			discordPresence.smallImageText = readenv("discordsmallimgtxt");
			Discord_UpdatePresence(&discordPresence);
		}

		m.lock();
		if(shouldShutdown == TRUE) break;
		m.unlock();

		Discord_UpdateConnection();
		Sleep(500);
	}

	Discord_ClearPresence();
	Discord_Shutdown();
    return 0;
}

int APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hInst);
		CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Process,NULL,0,NULL);
	}
	return TRUE;
}
