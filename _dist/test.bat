@echo off
setlocal enabledelayedexpansion
rundll32 getinput.dll,inject
:a
rem for /l %%x in (1,1,10000) do title !keyspressed!
rem title !keyspressed!
set /p=hello? 
goto :a