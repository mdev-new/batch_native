@echo off
:: Licensed under conditions stated in
:: the documentation you recieved with this software

setlocal enabledelayedexpansion
title example

<nul set /p=[?25l

:: run from \examples directory
if "%1" NEQ "nohook" (
  set /a mousexpos=mouseypos=keypressed=0
set discordappid=1035630279416610856
set discordstate=Hello world
set discorddetails=Test
set discordlargeimg=canary-large
set discordlargeimgtxt=Test1
set discordsmallimg=ptb-small
set discordsmallimgtxt=Test2

	for %%x in (getinput discordrpc) do ..\dist\inject.exe ..\dist\%%x.dll
)

set kblast=0

for /f %%a in ('copy /Z "%~dpf0" nul') do set "CR=%%a"

echo format: keypressed click mousexpos mousypos wheeldelta
echo.

:a
<nul set /p="!keypressed!        !click!        !mousexpos!        !mouseypos!        !wheelDelta!        !CR!"
goto :a