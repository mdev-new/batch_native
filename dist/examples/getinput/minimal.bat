@echo off
setlocal enabledelayedexpansion

SET BATCHNATIVE_BIN_HOME=..\..

%BATCHNATIVE_BIN_HOME%\inject.exe %BATCHNATIVE_BIN_HOME%\getinput.dll

:a
title !click! !mousexpos! !mouseypos! !wheeldelta!
echo !keyspressed!
goto :a