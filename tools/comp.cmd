@echo off

::cl /Os /D_USRDLL /D_WINDLL src\getinput.c /link /ENTRY:DllMain /NODEFAULTLIB /SUBSYSTEM:WINDOWS /ALIGN:1 /DLL /OUT:%TEMP%\getinput.dll

gcc -shared -o %TEMP%\getinput.dll src\getinput.c -Isrc -std=gnu99 -m64 -msse4.1 -Os -flto -fvisibility=hidden -fcompare-debug-second -fno-asynchronous-unwind-tables -nostartfiles -nodefaultlibs -nostdlib -nolibc -luser32 -lkernel32 -Wl,-s,-e,DllMain,--gc-sections,--reduce-memory-overheads,--file-alignment,1,--no-seh,--disable-reloc-section -DNDEBUG

if "%1"=="withdiscordrpc" (
	pushd src\extern
	call comp.cmd
	popd
)

:: cl /Os /D_USRDLL /D_WINDLL /DDISCORD_WINDOWS /DDISCORD_DISABLE_IO_THREAD /DDISCORD_DYNAMIC_LIB /DDISCORD_BUILDING_SDK /DNDEBUG src\discord.c src\extern\*.obj /link /ENTRY:DllMain /NODEFAULTLIB /SUBSYSTEM:CONSOLE /ALIGN:1 /DLL /OUT:%TEMP%\discord.dll

g++ -shared -static-libgcc -static-libstdc++ -static -o %TEMP%\discord.dll src\discord.c src\extern\*.o -Isrc -Isrc\extern\discord-rpc\include -m64 -Os -flto -fvisibility=hidden -fcompare-debug-second -fno-rtti -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -Wl,-s,-e,DllMain,--gc-sections,--reduce-memory-overheads,--file-alignment,1,--no-seh,--disable-reloc-section,--build-id=none -DDISCORD_WINDOWS -DDISCORD_DISABLE_IO_THREAD -DDISCORD_DYNAMIC_LIB -DDISCORD_BUILDING_SDK -DNDEBUG

strip --strip-unneeded -s -R .comment -R .gnu.version -R .note %TEMP%\getinput.dll
strip --strip-unneeded -s -R .comment -R .gnu.version -R .note %TEMP%\discord.dll

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

gcc -o dist\batch_native.exe src\hook.c -Isrc -Ibin -std=gnu99 -m64 -mconsole -Os -flto -fvisibility=hidden -fcompare-debug-second -fno-asynchronous-unwind-tables -nostartfiles -nodefaultlibs -nostdlib -nolibc -Wl,-s,-e,WinMain,--gc-sections -lntdll -lkernel32 -luser32 -lpsapi -DNDEBUG

:: cl /Isrc /Ibin /Os /D_USRDLL /D_WINDLL src\hook.c /link /ENTRY:WinMain /NODEFAULTLIB /SUBSYSTEM:WINDOWS /ALIGN:1 /DLL /OUT:dist\native.exe

strip --strip-unneeded -s -R .comment -R .gnu.version dist\batch_native.exe