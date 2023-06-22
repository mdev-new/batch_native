@echo off
setlocal enabledelayedexpansion

SET BATCHNATIVE_BIN_HOME=..\..

rundll32 %BATCHNATIVE_BIN_HOME%\getinput.dll,inject

:a
title !click! !mousexpos! !mouseypos! !wheeldelta! ; !keyspressed! ; !controller1! ; !controller2! ; !controller3! ; !controller4!
goto :a