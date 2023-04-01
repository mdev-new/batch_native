@echo off
setlocal enabledelayedexpansion

if not exist bin\comptool.exe (
    mkdir bin
    gcc -o bin\comptool.exe tools\comptool.c
)

::-msse4.1 -mavx -mmmx -ftree-vectorize -ffast-math -ftree-slp-vectorize -fassociative-math
set optargs=-Isrc -std=gnu99 -m64 -Os -flto -fno-pic -fno-pie -fno-plt -msse -mavx -mmmx -funsafe-math-optimizations -ftree-vectorize -ffast-math -ftree-slp-vectorize -fassociative-math -fvisibility=hidden -fcompare-debug-second -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -fno-asynchronous-unwind-tables -nostartfiles -nodefaultlibs -nostdlib -nolibc -Wl,-s,--gc-sections,--reduce-memory-overheads,--no-seh,--disable-reloc-section,--build-id=none -DNDEBUG

windres --input src\resources\dll.rc --output bin\dll.res --output-format=coff
gcc -shared -static -o bin\getinput.dll src\getinput.c %optargs% -luser32 -lkernel32 -lshcore -lgdi32 -lntdll -Wl,-e,DllMain

if "%1"=="fullbuild" (
	set sources= connection_win.cpp discord_register_win.cpp discord_rpc.cpp rpc_connection.cpp serialization.cpp
	g++ -shared -static-libgcc -static-libstdc++ -static -o bin\discordrpc.dll src\discord.cpp !sources: = src/extern/discord-rpc/src/! -Isrc -Isrc\extern\discord-rpc\include -Isrc\extern\rapidjson\include -mconsole -m64 -Os -flto -fno-pic -fno-pie -fno-plt -fvisibility=hidden -fcompare-debug-second -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -fno-asynchronous-unwind-tables -Wl,-s,-e,DllMain,--gc-sections,--reduce-memory-overheads,--no-seh,--disable-reloc-section,--build-id=none -DDISCORD_WINDOWS -DDISCORD_DISABLE_IO_THREAD -DDISCORD_DYNAMIC_LIB -DDISCORD_BUILDING_SDK
)

	strip --strip-unneeded -s -R .comment -R .gnu.version -R .note bin\%%a.dll
	bin\comptool bin\%%a.dll dist\%%a.dll
)

	strip --strip-unneeded -s -R .comment -R .gnu.version dist\inject.exe
