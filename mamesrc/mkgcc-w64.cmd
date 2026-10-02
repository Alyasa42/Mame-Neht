@echo off

set MINGW_ROOT=F:\mingw\mingw64-w64
set PATH=%MINGW_ROOT%\bin;%MINGW_ROOT%\opt\bin;%PATH%

gcc -v

make -j4
pause
