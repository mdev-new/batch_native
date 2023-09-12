@echo off
SET BATCHNATIVE_BIN_HOME=..\..

rundll32 %BATCHNATIVE_BIN_HOME%\audioplayer.dll,inject

<nul set/p"=test.mp3" > \\.\pipe\BatAudQ0
timeout /T 1 /NOBREAK

<nul set/p"=test.mp3" > \\.\pipe\BatAudQ1
timeout /T 1 /NOBREAK

<nul set/p"=test.mp3" > \\.\pipe\BatAudQ2
timeout /T 1 /NOBREAK

<nul set/p"=test.mp3" > \\.\pipe\BatAudQ3
timeout /T 1 /NOBREAK
