#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <stdlib.h>

#include "discord_rpc.h"

#include <atomic>
#include <malloc.h>

#include "Injector.h"
#include "Utilities.h"

std::atomic<bool> shouldShutdown = FALSE;

BOOL WINAPI ConsoleCloseHandler(DWORD dwCtrlType) {
	if (dwCtrlType >= 1 && dwCtrlType <= 6) {
		shouldShutdown = true;
	}
	return TRUE;
}

WNDPROC origWndProc = NULL;

// this is most likely broken.
LRESULT WndHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_CLOSE) {
		MessageBox(hWnd, "Closing!", "Closing", 0);
		shouldShutdown = TRUE;
	}

	return CallWindowProc(origWndProc, hWnd, message, wParam, lParam);
}

DWORD CALLBACK Process(void *data) {
	Sleep(250);

	origWndProc = (WNDPROC)SetWindowLongPtr(GetConsoleWindow(), -4, (LONG_PTR)WndHook);

	if (GetEnvironmentVariable("discordappid", NULL, 0) == 0) return TRUE;
	SetConsoleCtrlHandler(ConsoleCloseHandler, TRUE);

	DiscordEventHandlers handlers;
	ZeroMemory(&handlers, sizeof(DiscordEventHandlers));

	Discord_Initialize(readenv("discordappid"), &handlers, 1, NULL);

	DiscordRichPresence discordPresence;
	ZeroMemory(&discordPresence, sizeof(discordPresence));

	discordPresence.startTimestamp = time(0);
	discordPresence.endTimestamp = 0;
	discordPresence.instance = 0;

	while (1) {
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

		if (shouldShutdown == TRUE) break;

#ifdef DISCORD_DISABLE_IO_THREAD
		Discord_UpdateConnection();
#endif
		Discord_RunCallbacks();
		Sleep(500);
	}

	Discord_ClearPresence();
	Discord_Shutdown();
	return 0;
}

BasicDllMainImpl(Process);