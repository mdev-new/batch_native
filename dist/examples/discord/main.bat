@echo off

SET BATCHNATIVE_BIN_HOME=..\..

set discordappid=1035630279416610856
set discordstate=Hello world
set /a discorddetails=1
set discordlargeimg=canary-large
set discordlargeimgtxt=Test1
set discordsmallimg=ptb-small
set discordsmallimgtxt=Test2

%BATCHNATIVE_BIN_HOME%\inject %BATCHNATIVE_BIN_HOME%\discordrpc.dll

:l
set /a discorddetails+=1
ping -n 1 localhost >nul
goto :l