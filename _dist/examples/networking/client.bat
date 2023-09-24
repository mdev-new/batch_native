@echo off

SET BATCHNATIVE_BIN_HOME=..\..
rundll32 %BATCHNATIVE_BIN_HOME%\networking.dll,inject

ECHO tcpconnect;0;127.0.0.1;2456 > \\.\pipe\BatNetCmd
ECHO 0;hello > \\.\pipe\BatNetIn
pause >nul
echo tcpdestroy;0