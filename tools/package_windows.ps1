param(
    [string]$BuildDir = "build",
    [string]$OutDir = "dist",
    [string]$PackageName = "line-shooter-windows"
)

$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$BuildPath = Join-Path $Root $BuildDir
$Exe = Join-Path $BuildPath "fps_mvp.exe"
$MingwBin = "C:\msys64\mingw64\bin"
$QtPlatformDir = "C:\msys64\mingw64\share\qt6\plugins\platforms"
$Objdump = Join-Path $MingwBin "objdump.exe"

if (!(Test-Path $Exe)) { throw "Missing $Exe. Run build.bat first." }
if (!(Test-Path $Objdump)) { throw "Missing objdump.exe at $Objdump" }

$OutPath = Join-Path $Root $OutDir
$Stage = Join-Path $OutPath $PackageName
$Zip = Join-Path $OutPath "$PackageName.zip"

if (Test-Path $Stage) { Remove-Item -LiteralPath $Stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $Stage | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $Stage "platforms") | Out-Null

Copy-Item -LiteralPath $Exe -Destination $Stage -Force
if (Test-Path (Join-Path $BuildPath "settings.cfg")) {
    Copy-Item -LiteralPath (Join-Path $BuildPath "settings.cfg") -Destination $Stage -Force
}

$runPortable = @"
@echo off
setlocal
cd /d "%~dp0"
set PATH=%CD%;%PATH%
set QT_QPA_PLATFORM_PLUGIN_PATH=%CD%\platforms
fps_mvp.exe
"@
$runPortable | Set-Content -LiteralPath (Join-Path $Stage "run_portable.bat") -Encoding ASCII

$packageReadme = @"
Line Shooter - Windows portable package

Run:
  run_portable.bat

Controls:
  WASD move, mouse look, Shift sprint, J / left mouse fire, R reload, Esc menu/quit.

Notes:
  This package includes the MinGW/OpenCV/Qt runtime DLLs needed by fps_mvp.exe.
"@
$packageReadme | Set-Content -LiteralPath (Join-Path $Stage "README_PACKAGE.txt") -Encoding UTF8

$seen = @{}
$queue = New-Object System.Collections.Generic.Queue[string]
$queue.Enqueue($Exe)

function Add-DllIfNeeded([string]$dll) {
    $lower = $dll.ToLowerInvariant()
    $systemDlls = @(
        "kernel32.dll", "msvcrt.dll", "user32.dll", "winmm.dll", "gdi32.dll",
        "advapi32.dll", "shell32.dll", "ole32.dll", "oleaut32.dll",
        "comdlg32.dll", "comctl32.dll", "ws2_32.dll", "secur32.dll",
        "crypt32.dll", "bcrypt.dll", "rpcrt4.dll", "shlwapi.dll",
        "version.dll", "imm32.dll", "uxtheme.dll", "dwmapi.dll",
        "winspool.drv"
    )
    if ($systemDlls -contains $lower) { return }
    if ($seen.ContainsKey($lower)) { return }
    $src = Join-Path $MingwBin $dll
    if (!(Test-Path $src)) { return }
    $seen[$lower] = $true
    Copy-Item -LiteralPath $src -Destination $Stage -Force
    $queue.Enqueue($src)
}

function Add-DependenciesFromQueue {
    while ($queue.Count -gt 0) {
        $bin = $queue.Dequeue()
        $deps = & $Objdump -p $bin 2>$null |
            Select-String "DLL Name:" |
            ForEach-Object { ($_ -replace ".*DLL Name:\s*", "").Trim() }
        foreach ($d in $deps) { Add-DllIfNeeded $d }
    }
}

Add-DependenciesFromQueue

$qwindows = Join-Path $QtPlatformDir "qwindows.dll"
if (Test-Path $qwindows) {
    Copy-Item -LiteralPath $qwindows -Destination (Join-Path $Stage "platforms") -Force
    $queue.Enqueue($qwindows)
    Add-DependenciesFromQueue
}

if (Test-Path $Zip) { Remove-Item -LiteralPath $Zip -Force }
Compress-Archive -LiteralPath $Stage -DestinationPath $Zip -Force
Write-Host "Package created: $Zip"
