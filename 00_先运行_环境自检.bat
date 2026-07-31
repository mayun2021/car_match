@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set "FAILED=0"

set "UV4="
if defined KEIL5_ROOT if exist "%KEIL5_ROOT%\UV4\UV4.exe" set "UV4=%KEIL5_ROOT%\UV4\UV4.exe"
if not defined UV4 if exist "D:\keil5\UV4\UV4.exe" set "UV4=D:\keil5\UV4\UV4.exe"
if not defined UV4 if exist "C:\Keil_v5\UV4\UV4.exe" set "UV4=C:\Keil_v5\UV4\UV4.exe"

echo [1/7] Keil uVision
if defined UV4 (
    echo OK: %UV4%
) else (
    echo ERROR: Keil5 not found. Set KEIL5_ROOT and retry.
    set "FAILED=1"
)

echo [2/7] Keil project
if exist "keil\CarMatch_MSPM0G3507.uvprojx" (echo OK) else (echo ERROR & set "FAILED=1")

echo [3/7] Final Q1-Q3 AXF
if exist "firmware\CarMatch_MSPM0G3507_Q1_Q3_20260731.axf" (echo OK) else (echo ERROR & set "FAILED=1")

echo [4/7] Final Q1-Q3 HEX
if exist "firmware\CarMatch_MSPM0G3507_Q1_Q3_20260731.hex" (echo OK) else (echo ERROR & set "FAILED=1")

echo [5/7] K230 scripts
if exist "companion_k230\main.py" if exist "companion_k230\config.py" (
    echo OK
) else (
    echo ERROR
    set "FAILED=1"
)

echo [6/7] Materialized SDK
if exist "sdk\source\ti\driverlib\driverlib.h" if exist "sdk\source\third_party\CMSIS\Core\Include\core_cm0plus.h" (
    echo OK
) else (
    echo ERROR: SDK is incomplete.
    set "FAILED=1"
)

echo [7/7] Offline device pack
if exist "tools\offline_install\TexasInstruments.MSPM0G1X0X_G3X0X_DFP.1.3.1.pack" (
    echo OK
) else (
    echo ERROR
    set "FAILED=1"
)

echo.
findstr /C:"CALIBRATION_CONFIRMED = False" "companion_k230\config.py" >nul
if not errorlevel 1 (
    echo WARNING: K230 Q3 calibration is still locked.
    echo Complete the three-point calibration, then set CALIBRATION_CONFIRMED = True.
)
echo.
if "%FAILED%"=="0" (
    echo PASS: Q1-Q3 delivery package is complete.
) else (
    echo FAIL: One or more required files are missing.
)
echo Read README.md and README_Keil5 first before flashing.
if not defined SELF_CHECK_NO_PAUSE pause
exit /b %FAILED%
