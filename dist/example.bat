@echo off
setlocal enabledelayedexpansion

:: Licensed under conditions stated in
:: the documentation you recieved with this software
:: Parts of this stolen from yeshi (particularly the sprite drawing routines) and modified by me

>nul chcp 65001
mode 80,33
::color 70
<nul set /p=[?25l

set /a mousexpos=mouseypos=wheeldelta=0
set discordappid=1035630279416610856
set discordstate=Hello world
set discorddetails=Test ^| Test1
set discordlargeimg=nobitches
set discordlargeimgtxt=Test1
set discordsmallimg=ptb-small
set discordsmallimgtxt=Test2

:: inject consoleutils.dll
inject discordrpc.dll
inject getinput.dll

set sprite_unselected_line1=xxxxxxxx
set sprite_unselected_line2=xoooooox
set sprite_unselected_line3=xoooooox
set sprite_unselected_line4=xoooooox
set sprite_unselected_line5=xxxxxxxx

set sprite_selected_line1=xxxxxxxx
set sprite_selected_line2=xxxxxxxx
set sprite_selected_line3=xxxxxxxx
set sprite_selected_line4=xxxxxxxx
set sprite_selected_line5=xxxxxxxx

set /a notimes=test=0
<nul set /p=[3;0Hnot clicked

:main
:: color 70
set /a mx=iconshover=0
<nul set /p=[1;0Hmclick:	!click!  	mx:	!mousexpos!  	my:	!mouseypos!  	wheel:!wheeldelta!  	
<nul set /p=[2;0Hkeys pressed: %keyspressed%                     & :: leave some space
:: title %keyspressed%
:: title %time%
::title %test%

if !mouseypos! GEQ 9 if !mouseypos! LEQ 13 if !mousexpos! GEQ 13 if !mousexpos! LEQ 21 (
  call :sprite selected 14 10 8 5
  set iconshover=1
  if !click! EQU 1 ( <nul set /p=[3;0Hclicked     ) else <nul set /p=[3;0Hnot clicked
)
if !iconshover! EQU 0 call :sprite unselected 14 10 8 5

goto :main

:sprite
set file=%1
set /a spritey_end=%3+%5-1,spriterendercharamount=%2+%4-1
if %spriterendercharamount% GEQ 148 set /a spriterendercharamount=148
set /a spriterendercharamount-=%2,spriterendercharamount+=1,spriterenderstartoffset=0,spriterenderxpos=%2
if %spriterenderxpos% LEQ -1 set /a spriterenderstartoffset=%spriterenderxpos:~1%+1,spriterenderxpos=0
if NOT "%6"=="nodraw" set disp=
if NOT "!disp!"=="" if NOT "!disp:~7000!"=="" (
	echo.!disp!
	set disp=
)
for /l %%a in (%3,1,%spritey_end%) do (
	set /a linetorender=spritey_end-%%a+1
	if %%a GEQ 1 if %%a LEQ 40 for %%b in (!linetorender!) do set disp=!disp![%%a;!spriterenderxpos!H!sprite_%file%_line%%b:~%spriterenderstartoffset%,%spriterendercharamount%!
)
if NOT "%6"=="nodraw" (
	if NOT "!disp!"=="" echo(!disp!
	set disp=
)
exit /b
