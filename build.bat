@echo off
REM ---- Build the FPS MVP with the MSYS2 mingw64 toolchain ----
setlocal
set PATH=C:\msys64\mingw64\bin;%PATH%
cd /d "%~dp0"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release || goto :err
REM Use --clean-first because Ninja can otherwise trust a stale/corrupt exe if
REM its timestamp is newer than the object files. This project previously had a
REM 36-byte invalid build\fps_mvp.exe, which made "ninja: no work to do" produce
REM a non-runnable program.
cmake --build build --clean-first || goto :err
echo.
echo Build OK - build\fps_mvp.exe   (run it with run.bat)
goto :eof
:err
echo.
echo BUILD FAILED. Ensure MSYS2 packages are installed (see README.md).
exit /b 1
