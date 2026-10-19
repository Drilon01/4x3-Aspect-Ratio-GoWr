@echo off
rem Builds GoWR43-Patcher.exe and GoWR43-Undo.exe into bin\
rem Needs Zig on PATH (winget install zig.zig) or "pip install ziglang==0.17.0"
setlocal
cd /d "%~dp0"
if not exist bin mkdir bin

set ZIG=zig
where zig >nul 2>nul || set ZIG=python -m ziglang

%ZIG% cc -target x86_64-windows-gnu -O2 -s src\gowr43patch.c -lshell32 -ladvapi32 -o bin\GoWR43-Patcher.exe || goto :fail
%ZIG% cc -target x86_64-windows-gnu -O2 -s -DUNDO_BUILD src\gowr43patch.c -lshell32 -ladvapi32 -o bin\GoWR43-Undo.exe || goto :fail

echo.
echo Built:
dir /b bin\*.exe
certutil -hashfile bin\GoWR43-Patcher.exe SHA256 | findstr /v ":"
certutil -hashfile bin\GoWR43-Undo.exe SHA256 | findstr /v ":"
goto :eof

:fail
echo Build failed.
exit /b 1
