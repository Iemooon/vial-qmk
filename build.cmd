@echo off
REM Build a Vial-QMK target with the AT32/ChibiOS toolchain.
REM
REM Why this wrapper exists: QMK's makefiles shell out through /bin/sh, and when
REM sh is launched directly by a Win32 parent (plain PowerShell or cmd) the
REM `-c` command line is truncated at roughly 8 kB. The AT32 chain expands a
REM single gcc invocation past 12 kB, so a bare `qmk compile` dies with
REM   /bin/sh: -c: line 1: unexpected EOF while looking for matching `"'
REM followed by "Platform not defined" - which looks like a broken board config
REM but is not. Running make under bash avoids the Win32 command-line limit.
REM
REM Usage:  build.cmd <keyboard> <keymap>      e.g.  build.cmd ortho75 vial
setlocal
if "%1"=="" ( echo usage: build.cmd ^<keyboard^> ^<keymap^> & exit /b 2 )
set MSYS2_PATH_TYPE=inherit
set PATH=C:\msys64\usr\bin;C:\msys64\mingw64\bin;C:\Python\Python313;C:\Python\Python313\Scripts;D:\Projects\arm-none-eabi\bin;%PATH%
for %%I in ("%~dp0.") do set TREE=%%~fsI
C:\msys64\usr\bin\bash.exe -lc "export PATH=/c/msys64/usr/bin:/c/msys64/mingw64/bin:/c/Python/Python313:/c/Python/Python313/Scripts:/d/Projects/arm-none-eabi/bin:\$PATH; cd '%TREE:\=/%' && qmk compile -kb %1 -km %2"
set RC=%errorlevel%
endlocal
exit /b %RC%
