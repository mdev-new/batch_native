@echo off
setlocal enabledelayedexpansion

goto :hook

:: del %temp%\getinput.dll %temp%\discord.dll

gcc -shared -o %TEMP%\getinput.dll src\getinput.c -Isrc -std=gnu99 -m64 -msse4.1 -Os -flto -fno-pic -fno-pie -fno-plt -fvisibility=hidden -fcompare-debug-second -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -fno-asynchronous-unwind-tables -nostartfiles -nodefaultlibs -nostdlib -nolibc -luser32 -lkernel32 -Wl,-s,-e,DllMain,--gc-sections,--reduce-memory-overheads,--no-seh,--file-alignment,2,--section-alignment,2,--disable-reloc-section,--build-id=none

set sources= connection_win.cpp discord_register_win.cpp discord_rpc.cpp rpc_connection.cpp serialization.cpp
g++ -shared -static-libgcc -static-libstdc++ -static -o %TEMP%\discord.dll src\discord.cpp %sources: = src/extern/discord-rpc/src/% -Isrc -Isrc\extern\discord-rpc\include -Isrc\extern\rapidjson\include -mconsole -m64 -Os -flto -fno-pic -fno-pie -fno-plt -fvisibility=hidden -fcompare-debug-second -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -fno-asynchronous-unwind-tables -Wl,-s,-e,DllMain,--gc-sections,--reduce-memory-overheads,--no-seh,--file-alignment,2,--section-alignment,2,--disable-reloc-section,--build-id=none -DDISCORD_WINDOWS -DDISCORD_DISABLE_IO_THREAD -DDISCORD_DYNAMIC_LIB -DDISCORD_BUILDING_SDK

strip --strip-unneeded -s -R .comment -R .gnu.version -R .note %TEMP%\getinput.dll
strip --strip-unneeded -s -R .comment -R .gnu.version -R .note %TEMP%\discord.dll

:hook

:convtool
if not exist bin\convtool.exe (
	mkdir bin
	gcc -O2 -s -o bin\convtool.exe tools\converttool.c
	gcc -O2 -s -o bin\comptool.exe tools\compresslz77.c
	rem cl /O2 tools\converttool.c /Fe:bin\convtool.exe
	goto :convtool
)
echo #pragma once > bin\dllcode.h

for %%I in (!temp!\getinput.dll) do (
	echo const unsigned long long getinput_real_size = %%~zI; >> bin\dllcode.h
	bin\comptool.exe %temp%\getinput.dll %temp%\getinput.lz1 %%~zI
)
for %%I in (!temp!\discord.dll) do (
	echo const unsigned long long discord_real_size = %%~zI; >> bin\dllcode.h
	bin\comptool.exe %temp%\discord.dll %temp%\discord.lz1 %%~zI
)

echo char getinput_dll_code[] = { >> bin\dllcode.h
bin\convtool.exe -b %TEMP%\getinput.lz1 >> bin\dllcode.h
echo }; >> bin\dllcode.h

echo char discord_dll_code[] = { >> bin\dllcode.h
bin\convtool.exe -b %TEMP%\discord.lz1 >> bin\dllcode.h
echo }; >> bin\dllcode.h

echo const unsigned long long getinput_size = sizeof(getinput_dll_code); >> bin\dllcode.h
echo const unsigned long long discord_size = sizeof(discord_dll_code); >> bin\dllcode.h

gcc -o dist\batch_native.exe src\hook.c -Isrc -Ibin -std=gnu99 -m64 -mconsole -Os -flto -fno-pic -fno-pie -fno-plt -fvisibility=hidden -fcompare-debug-second -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -fno-asynchronous-unwind-tables -nostartfiles -nodefaultlibs -nostdlib -nolibc -Wl,-s,-e,WinMain,--gc-sections,--reduce-memory-overheads,--no-seh,--disable-reloc-section,--build-id=none -lntdll -lkernel32 -luser32 -lpsapi

strip --strip-unneeded -s -R .comment -R .gnu.version dist\batch_native.exe