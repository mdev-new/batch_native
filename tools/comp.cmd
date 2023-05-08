@echo off
setlocal enableextensions enabledelayedexpansion

:: someone please fix this build system

if not exist bin\comptool.exe call :recompileTool
if "%1"=="fullbuild" call :recompileTool

set optargs=-pipe -static -Isrc -std=gnu99 -m64 -Os -flto -fno-pic -fno-pie -fno-plt -msse -mavx -mmmx -funsafe-math-optimizations -ftree-vectorize -ffast-math -ftree-slp-vectorize -fassociative-math -fvisibility=hidden -fcompare-debug-second -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -fno-asynchronous-unwind-tables -nostartfiles -nodefaultlibs -nostdlib -nolibc -Wl,-s,--gc-sections,--reduce-memory-overheads,--no-seh,--disable-reloc-section,--build-id=none -DNDEBUG

call :compile_C_Dll "src\getinput.c" "bin\getinput.dll" "-lshcore -lgdi32 -lntdll"
call :compile_Cpp_Dll "src\map_renderer.cpp" "bin\map_rndr.dll" ""

if "%1"=="fullbuild" (
	set sources= connection_win.cpp discord_register_win.cpp discord_rpc.cpp rpc_connection.cpp serialization.cpp
	g++ -shared -static-libgcc -static-libstdc++ -o bin\discordrpc.dll src\discord.cpp !sources: = src/extern/discord-rpc/src/! -Isrc -Isrc\extern\discord-rpc\include -Isrc\extern\rapidjson\include -mconsole -m64 -Os -flto -fno-pic -fno-pie -fno-plt -fvisibility=hidden -fcompare-debug-second -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -fno-asynchronous-unwind-tables -Wl,-s,-e,DllMain,--gc-sections,--reduce-memory-overheads,--no-seh,--disable-reloc-section,--build-id=none -DDISCORD_WINDOWS -DDISCORD_DISABLE_IO_THREAD -DDISCORD_DYNAMIC_LIB -DDISCORD_BUILDING_SDK
)

for %%a in (getinput discordrpc map_rndr) do (
	strip --strip-unneeded -s -R .comment -R .gnu.version -R .note bin\%%a.dll
	bin\comptool 2 bin\%%a.dll dist\%%a.dll
)

if "%1"=="fullbuild" (
	gcc -o dist\inject.exe src\hook.c src\extern\chkstk.S -Isrc -Ibin -std=gnu99 -m64 -mwindows %optargs% -Wl,-e,WinMain -lkernel32 -luser32 -lpsapi -lshlwapi -lshell32
	strip --strip-unneeded -s -R .comment -R .gnu.version dist\inject.exe
)

exit /b

:recompileTool
mkdir bin
gcc -pipe -Os -flto -s -o bin\comptool.exe tools\comptool.c
exit /b

:compile_C_Dll
set name=%1
set outpt=%2
set expOpt=%3
set expOpt=%expOpt:"=%
gcc -w -shared -o %outpt% %name% %optargs% -luser32 -lkernel32 -Wl,-e,DllMain %expOpt%
exit /b

:compile_Cpp_Dll
set name=%1
set outpt=%2
set expOpt=%3
set expOpt=%expOpt:"=%

set optargs=%optargs:gnu99=gnu++23%
set optargs=%optargs:-nostdlib=%
set optargs=%optargs:-nolibc=%
set optargs=%optargs:-nodefaultlibs=%

g++ -shared -static-libstdc++ -static-libgcc -w -fpermissive -o %outpt% %name% %optargs% -luser32 -lkernel32 -Wl,-e,DllMain %expOpt%
exit /b
