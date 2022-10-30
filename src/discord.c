#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <stdlib.h>

#include "discord_rpc.h"

#include <Windows.h>

char *readenv(const char *name) {
	static TCHAR buffer[32];
	GetEnvironmentVariable(name, buffer, 32);
	return strdup(buffer); // memory leaks go brr
}

DWORD Process() {
	if(GetEnvironmentVariable("discordappid", NULL, 0) == 0) return -1;

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
	Discord_UpdateConnection();

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

		Discord_UpdateConnection();
		Sleep(500);
	}

	Discord_ClearPresence();
	Discord_Shutdown();
    return 0;
}

int APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH) CreateThread(0,0,(LPTHREAD_START_ROUTINE)Process,0,0,0);
	return TRUE;
}
