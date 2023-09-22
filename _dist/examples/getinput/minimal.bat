@echo off
setlocal enabledelayedexpansion

mode 125,55

SET BATCHNATIVE_BIN_HOME=..\..
set /a rasterx=rastery=8
rem set /a limitMouseX=limitMouseY=40
rundll32 %BATCHNATIVE_BIN_HOME%\getinput.dll,inject

echo Variables are displayed in the title bar
echo The format is: click mousexpos mouseypos wheeldelta ; keyspressed ; controller1 ; controller2 ; controller3 ; controller4

:a
title !click! !mousexpos! !mouseypos! !wheeldelta! ; !keyspressed! ; !controller1! ; !controller2! ; !controller3! ; !controller4!

for /l %%c in (1,1,4) do (
	for /f "tokens=1,2 delims=^|" %%a in ("!controller%%c!") do (
		if not "%%b"=="" (
			set controller%%c_btns=%%a
			set analograw=%%b
			set /a "controller%%c_!analograw:,=,controller%%c_!"
		)
	)
)

if "!keyspressed!"=="-32-" (
	rem set /a rasterx=10,rastery=18
	rem set /a screenx=5,screeny=5
	<nul set/p"=begin_text_input" > \\.\pipe\GetinputCmd
	set /p test=Type text: 
	<nul set/p"=end_text_input" > \\.\pipe\GetinputCmd
)

goto :a