@echo off
REM ---- Run the FPS MVP ----
REM OpenCV highgui here is built against Qt6, so we put mingw64\bin on PATH and
REM point Qt at its platform plugin. Adjust the paths if MSYS2 is elsewhere.
setlocal
set PATH=C:\msys64\mingw64\bin;%PATH%
set QT_QPA_PLATFORM_PLUGIN_PATH=C:\msys64\mingw64\share\qt6\plugins\platforms
cd /d "%~dp0build"
if not exist fps_mvp.exe (
  echo fps_mvp.exe not found. Run build.bat first.
  exit /b 1
)
fps_mvp.exe
