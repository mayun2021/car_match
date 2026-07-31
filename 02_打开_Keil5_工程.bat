@echo off
setlocal EnableExtensions
set "PROJECT=%~dp0keil\CarMatch_MSPM0G3507.uvprojx"
set "UV4="
if defined KEIL5_ROOT if exist "%KEIL5_ROOT%\UV4\UV4.exe" set "UV4=%KEIL5_ROOT%\UV4\UV4.exe"
if not defined UV4 if exist "D:\keil5\UV4\UV4.exe" set "UV4=D:\keil5\UV4\UV4.exe"
if not defined UV4 if exist "C:\Keil_v5\UV4\UV4.exe" set "UV4=C:\Keil_v5\UV4\UV4.exe"
if not defined UV4 (
    echo ERROR: Keil5 not found. Set KEIL5_ROOT and retry.
    pause
    exit /b 2
)
start "" "%UV4%" "%PROJECT%"
