@echo off

gcc -shared -o %TEMP%\getinput.dll src\getinput.c -Isrc -std=gnu99 -m64 -msse4.1 -Os -Wl,-s,-e,DllMain -fno-asynchronous-unwind-tables -nostartfiles -nodefaultlibs -nostdlib -nolibc -luser32 -lkernel32 -Wl,-s,-e,DllMain

set sources= connection_win.cpp discord_register_win.cpp discord_rpc.cpp rpc_connection.cpp serialization.cpp
g++ -shared -static-libgcc -static-libstdc++ -static -o %TEMP%\discord.dll src\discord.c %sources: = src/extern/discord-rpc/src/% -Isrc -Isrc\extern\discord-rpc\include -Isrc\extern\rapidjson\include -m64 -Os -Wl,-s,-e,DllMain -DDISCORD_WINDOWS -DDISCORD_DISABLE_IO_THREAD -DDISCORD_DYNAMIC_LIB -DDISCORD_BUILDING_SDK

:: strip --strip-unneeded -s -R .comment -R .gnu.version -R .note %TEMP%\getinput.dll
:: strip --strip-unneeded -s -R .comment -R .gnu.version -R .note %TEMP%\discord.dll

:convtool
if not exist bin\convtool.exe (
	mkdir bin
	::gcc -O2 -s -o bin\convtool.exe tools\converttool.c
	cl /O2 tools\converttool.c /Fe:bin\convtool.exe
	goto :convtool
) else (
	echo #pragma once > bin\dllcode.h
	echo static char getinput_dll_code[] = { >> bin\dllcode.h
	bin\convtool.exe -b %TEMP%\getinput.dll >> bin\dllcode.h
	echo }; >> bin\dllcode.h

	echo static char discord_dll_code[] = { >> bin\dllcode.h
	bin\convtool.exe -b %TEMP%\discord.dll >> bin\dllcode.h
	echo }; >> bin\dllcode.h
)

gcc -o dist\batch_native.exe src\hook.c -Isrc -Ibin -std=gnu99 -m64 -mconsole -Os -nostartfiles -nodefaultlibs -nostdlib -nolibc -Wl,-s,-e,WinMain -lntdll -lkernel32 -luser32 -lpsapi

:: strip --strip-unneeded -s -R .comment -R .gnu.version dist\batch_native.exe