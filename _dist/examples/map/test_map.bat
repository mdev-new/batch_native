@echo off

setlocal enabledelayedexpansion

mode 40,20

SET BATCHNATIVE_BIN_HOME=..\..

set /a levelWidth=113
set /a levelHeight=78
set /a viewYoff=0
set /a viewXoff=0
set mapFile=map.txt

rundll32 %BATCHNATIVE_BIN_HOME%\map_rndr.dll,inject
set /a timeout=1000

:l

for /l %%x in (1,1,60) do (
	set /a viewXoff+=1
	for /l %%x in (1,1,!timeout!) do break
)

for /l %%x in (60,-1,0) do (
	set /a viewXoff-=1
	for /l %%x in (1,1,!timeout!) do break
)

for /l %%x in (1,1,40) do (
	set /a viewYoff+=1
	for /l %%x in (1,1,!timeout!) do break
)

for /l %%x in (40,-1,0) do (
	set /a viewYoff-=1
	for /l %%x in (1,1,!timeout!) do break
)

goto :l
