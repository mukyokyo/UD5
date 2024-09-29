@echo off
cd %~dp0
SET ORGPATH=%PATH%
REM **** path to msys2 commands
SET MSYSROOT="%ProgramFiles%\BestTech\GCC Developer Lite\msys2"
IF NOT EXIST %MSYSROOT% SET MSYSROOT="%ProgramFiles(x86)%\BestTech\GCC Developer Lite\msys2"
IF NOT EXIST %MSYSROOT% GOTO FAIL
REM **** path to GCC
SET GCCROOT="%ProgramData%\BestTech\GCC Developer Lite\GCC\ARM"
IF NOT EXIST %GCCROOT% SET GCCROOT="%ProgramFiles%\BestTech\GCC Developer Lite\GCC\ARM"
IF NOT EXIST %GCCROOT% SET GCCROOT="%ProgramFiles(x86)%\BestTech\GCC Developer Lite\GCC\ARM"
IF NOT EXIST %GCCROOT% GOTO FAIL
REM **** TARGET folder
SET TARGETROOT="%ProgramData%\BestTech\GCC Developer Lite\TARGET"
IF NOT EXIST %TARGETROOT% SET TARGETROOT="%ProgramFiles%\BestTech\GCC Developer Lite\TARGET"
IF NOT EXIST %TARGETROOT% SET TARGETROOT="%ProgramFiles(x86)%\BestTech\GCC Developer Lite\TARGET"
IF NOT EXIST %TARGETROOT%\LPC84x GOTO FAIL
IF NOT EXIST %TARGETROOT%\LPC84x_EXTRA GOTO FAIL
IF NOT EXIST %TARGETROOT%\FREERTOS_CM0P GOTO FAIL
REM **** Create semantic link
mklink /j ".\GCCROOT" %GCCROOT%
mklink /j ".\MSYSROOT" %MSYSROOT%
mklink /j ".\TARGETROOT" %TARGETROOT%
SET PATH=.\MSYSROOT;.\GCCROOT\bin;%ORGPATH%
make clean
make %~n1
IF ERRORLEVEL 1 GOTO FAIL
GOTO SUCCESS
:FAIL
ECHO ERROR !!!
PAUSE
GOTO END
:SUCCESS
ECHO SUCCESS !!!
timeout 2 > nul
:END
SET PATH=%ORGPATH%
rmdir .\GCCROOT
rmdir .\MSYSROOT
rmdir .\TARGETROOT
