@echo off

SET BATCHNATIVE_BIN_HOME=..\..

set discordappid=1122614146534482151
set discordstate=Hello world
set /a discorddetails=1
set discordlargeimg=canary-large
set discordlargeimgtxt=Test1
set discordsmallimg=ptb-small
set discordsmallimgtxt=Test2
set discordupdate=1

rundll32 %BATCHNATIVE_BIN_HOME%\discord_rpc.dll,inject

:l
set /a discorddetails+=1
set discordupdate=1
ping -n 2 localhost >nul
goto :l