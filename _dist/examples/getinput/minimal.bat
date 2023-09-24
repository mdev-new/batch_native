@echo off
setlocal enabledelayedexpansion

mode 125,55

SET BATCHNATIVE_BIN_HOME=..\..
set /a rasterx=rastery=8
set /a limitMouseX=limitMouseY=40
rundll32 %BATCHNATIVE_BIN_HOME%\getinput.dll,inject

echo Variables are displayed in the title bar
echo The format is: click mousexpos mouseypos wheeldelta ; keyspressed ; controller1 ; controller2 ; controller3 ; controller4

:a
title !click! !mousexpos! !mouseypos! !wheeldelta! ; !keyspressed! ; !controller1! ; !controller2! ; !controller3! ; !controller4!

if "!keyspressed!"=="-32-" (
	rem set /a rasterx=10,rastery=18
	rem set /a screenx=5,screeny=5
	set /p test=Type text: 
)

goto :a