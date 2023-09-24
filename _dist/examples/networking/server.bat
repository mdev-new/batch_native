@echo off

SET BATCHNATIVE_BIN_HOME=..\..
echo Initializing batch_native
rundll32 %BATCHNATIVE_BIN_HOME%\networking.dll,inject
echo Creating socket
ECHO tcpcreate;0;2456 > \\.\pipe\BatNetCmd
echo Press key when client sent an input.
pause >nul
echo Reading out pipe
ECHO \\.\pipe\BatNetOut
ECHO tcpdestroy;0