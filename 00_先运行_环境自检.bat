@echo off
setlocal EnableExtensions
cd /d "%~dp0"

set "UV4="
if defined KEIL5_ROOT if exist "%KEIL5_ROOT%\UV4\UV4.exe" set "UV4=%KEIL5_ROOT%\UV4\UV4.exe"
if not defined UV4 if exist "D:\keil5\UV4\UV4.exe" set "UV4=D:\keil5\UV4\UV4.exe"
if not defined UV4 if exist "C:\Keil_v5\UV4\UV4.exe" set "UV4=C:\Keil_v5\UV4\UV4.exe"

echo [1/4] Keil uVision
if defined UV4 (
    echo OK: %UV4%
) else (
    echo ERROR: Keil5 not found. Set KEIL5_ROOT and retry.
)

echo [2/4] Project
if exist "keil\CarMatch_MSPM0G3507.uvprojx" (echo OK) else (echo ERROR)

echo [3/4] Prebuilt STABLE V2 HEX
if exist "firmware\CarMatch_MSPM0G3507_STABLE_V2.hex" (echo OK) else (echo ERROR)

echo [4/4] Offline device pack
if exist "tools\offline_install\TexasInstruments.MSPM0G1X0X_G3X0X_DFP.1.3.1.pack" (echo OK) else (echo ERROR)

echo.
echo This package is STABLE LINE FOLLOWING V2.
echo After flashing, the OLED first line must show V2.
echo Read README_Keil5_先看.md before the first powered test.
pause
