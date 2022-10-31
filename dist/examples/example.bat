@echo off
:: Licensed under conditions stated in
:: the documentation you recieved with this software

setlocal enabledelayedexpansion
title example

<nul set /p=[?25l

:: run from \examples directory
if "%1" NEQ "nohook" (
  set /a mousexpos=mouseypos=keypressed=0

  ..\batch_native.exe
  if !errorlevel! NEQ 0 (
    echo error while hooking into cmd %errorlevel%
    pause
    exit /b
  )
)

set kblast=0

for /f %%a in ('copy /Z "%~dpf0" nul') do set "CR=%%a"

echo displaying only last key because batch/getinput is too fast
echo format: kblast(emu) click mousx mousy
echo.

set kblast=0

:a
<nul set /p="!kblast!        !click!        !mousexpos!        !mouseypos!        !wheelDelta!        !CR!"
if !keypressed! NEQ 0 set kblast=!keypressed! & set keypressed=0
goto :a