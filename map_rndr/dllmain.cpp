#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdio.h>

#include <vector>
#include <algorithm>
#include <tuple>

#include "Injector.h"
#include "Utilities.h"

template<typename T>
void remove(std::vector<T>** v, const T& target)
{
	(*v)->erase(std::remove((*v)->begin(), (*v)->end(), target), (*v)->end());
}

int load_map(std::vector<char>** vec, char* fname) {
	DWORD read;

	HANDLE hFile = CreateFile(fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	DWORD size = GetFileSize(hFile, NULL);

	char *tempFileBuf = (char *)alloca(size);
	ReadFile(hFile, tempFileBuf, size, &read, NULL);

	if (*vec != nullptr) delete *vec; // delete the old map
	*vec = new std::vector<char>((char*)tempFileBuf, (char*)(tempFileBuf + size));

	CloseHandle(hFile);

	// remove cr, lf
	remove(vec, '\x0D');
	remove(vec, '\x0A');

	return size;
}

DWORD CALLBACK Process(LPVOID data) {
	Sleep(250);

	HANDLE hStdOut;
	DWORD written;
	DWORD fileSize;

	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	std::vector<char>* map_data = nullptr;

	int w, h;
	int viewportX = 0, viewportY = 0;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hStdOut, &csbi);

	// this is a binding so the variables will be updated automatically
	const auto& [viewportSzX, viewportSzY] = csbi.dwSize;

	char fnameBuf_1[MAX_PATH] = { 0 };
	char fnameBuf_2[MAX_PATH] = { 0 };

	bool index = 0;
	char* buffers[2] = {
		fnameBuf_1, fnameBuf_2
	};

	char* scrBuf = (char*) malloc(1024 * 1024); // surely nobody has this big screen

	GetEnvironmentVariable("mapFile", buffers[0], MAX_PATH);
	fileSize = load_map(&map_data, buffers[0]);

	register int y, x;
	while (true) {
		GetEnvironmentVariable("mapFile", buffers[index = !index], MAX_PATH);

		if (strncmp(buffers[0], buffers[1], MAX_PATH) != 0) {
			fileSize = load_map(&map_data, buffers[index]);
		}

		GetConsoleScreenBufferInfo(hStdOut, &csbi);
		w = getenvnum("levelWidth");
		h = getenvnum("levelHeight");
		viewportX = getenvnum("viewXoff");
		viewportY = getenvnum("viewYoff");

		if (viewportX + viewportSzX > w) viewportX = w - viewportSzX;
		if (viewportY + viewportSzY > h) viewportY = h - viewportSzY;
		if (viewportX < 0) viewportX = 0;
		if (viewportY < 0) viewportY = 0;

		for (y = 0; y < viewportSzY; y++) {
			for (x = 0; x < viewportSzX; x++) {
				scrBuf[y * viewportSzX + x] = (x < w && y < h) ? map_data->data()[(y + viewportY) * w + (x + viewportX)] : ' ';
			}
		}

		WriteConsoleOutputCharacter(hStdOut, scrBuf, viewportSzY * viewportSzX, { 0,0 }, &written);
		Sleep(1000 / 40);
	}
}

BasicDllMainImpl(Process);