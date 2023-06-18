@echo off
set OPTLEVEL=3
set dllname=testdll

:: im sorry for this, this is what i use for getinput and it just works, so dont touch it if possible please
set "flags=-m64 -O%optlevel% -ffreestanding -flto -fwhole-program -fno-pic -fno-pie -fno-plt -msse -mavx -mmmx -funsafe-math-optimizations -ftree-vectorize -ffast-math -ftree-slp-vectorize -fassociative-math -fvisibility=hidden -fcompare-debug-second -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -fno-asynchronous-unwind-tables -nostartfiles -static -static-libgcc -Wl,-s,-e,DllMain,--gc-sections,--as-needed,--reduce-memory-overheads,--no-seh,--disable-reloc-section,--build-id=none -DNDEBUG"

:: compile the sources into native dll
gcc -o temp.dll dllmain.c %flags%
pause

comptool 2 temp.dll %dllname%.dll

del temp.dll
