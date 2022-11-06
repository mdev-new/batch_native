@echo off
setlocal enabledelayedexpansion

gcc -shared -o dist\getinput.dll src\getinput.c -Isrc -std=gnu99 -m64 -msse4.1 -Os -flto -fno-pic -fno-pie -fno-plt -fvisibility=hidden -fcompare-debug-second -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -fno-asynchronous-unwind-tables -nostartfiles -nodefaultlibs -nostdlib -nolibc -luser32 -lkernel32 -Wl,-s,-e,DllMain,--gc-sections,--reduce-memory-overheads,--no-seh,--disable-reloc-section,--build-id=none

set sources= connection_win.cpp discord_register_win.cpp discord_rpc.cpp rpc_connection.cpp serialization.cpp
g++ -shared -static-libgcc -static-libstdc++ -static -o dist\discord.dll src\discord.cpp %sources: = src/extern/discord-rpc/src/% -Isrc -Isrc\extern\discord-rpc\include -Isrc\extern\rapidjson\include -mconsole -m64 -Os -flto -fno-pic -fno-pie -fno-plt -fvisibility=hidden -fcompare-debug-second -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -fno-asynchronous-unwind-tables -Wl,-s,-e,DllMain,--gc-sections,--reduce-memory-overheads,--no-seh,--disable-reloc-section,--build-id=none -DDISCORD_WINDOWS -DDISCORD_DISABLE_IO_THREAD -DDISCORD_DYNAMIC_LIB -DDISCORD_BUILDING_SDK

strip --strip-unneeded -s -R .comment -R .gnu.version -R .note %TEMP%\getinput.dll
strip --strip-unneeded -s -R .comment -R .gnu.version -R .note %TEMP%\discord.dll

gcc -o dist\batch_native.exe src\hook.c -Isrc -Ibin -std=gnu99 -m64 -mconsole -Os -flto -fno-pic -fno-pie -fno-plt -fvisibility=hidden -fcompare-debug-second -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -fno-asynchronous-unwind-tables -nostartfiles -nodefaultlibs -nostdlib -nolibc -Wl,-s,-e,WinMain,--gc-sections,--reduce-memory-overheads,--no-seh,--disable-reloc-section,--build-id=none -lntdll -lkernel32 -luser32 -lpsapi -lshell32

strip --strip-unneeded -s -R .comment -R .gnu.version dist\batch_native.exe
