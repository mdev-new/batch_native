@echo off
:: Licensed under conditions stated in
:: the documentation you recieved with this software

setlocal enabledelayedexpansion
title example

<nul set /p=[?25l

:: run from \examples directory
if "%1" NEQ "nohook" (
  set /a mousexpos=mouseypos=keypressed=0

	for %%x in (getinput discord) do ..\dist\batch_native.exe ..\dist\%%x.dll
  if !errorlevel! NEQ 0 (
    echo error while hooking into cmd %errorlevel%
    pause
    exit /b
  )
)

set kblast=0

for /f %%a in ('copy /Z "%~dpf0" nul') do set "CR=%%a"

echo format: keypressed click mousexpos mousypos wheeldelta
echo.

:a
<nul set /p="!keypressed!        !click!        !mousexpos!        !mouseypos!        !wheelDelta!        !CR!"
goto :a