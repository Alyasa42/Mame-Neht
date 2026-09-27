@echo off

set MINGW_ROOT=F:\mingw\mingw64-w32
set PATH=%MINGW_ROOT%\bin;%MINGW_ROOT%\opt\bin;%PATH%

gcc -v

make -j3
pause
