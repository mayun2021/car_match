@echo off
setlocal EnableExtensions
for %%I in ("%~dp0..") do set "PROJECT_ROOT=%%~fI"
set "PROJECT=%PROJECT_ROOT%\keil\CarMatch_MSPM0G3507.uvprojx"
set "TARGET=CarMatch_MSPM0G3507"
set "LOG=%PROJECT_ROOT%\keil\build.log"
set "UV4="
if defined KEIL5_ROOT if exist "%KEIL5_ROOT%\UV4\UV4.exe" set "UV4=%KEIL5_ROOT%\UV4\UV4.exe"
if not defined UV4 if exist "D:\keil5\UV4\UV4.exe" set "UV4=D:\keil5\UV4\UV4.exe"
if not defined UV4 if exist "C:\Keil_v5\UV4\UV4.exe" set "UV4=C:\Keil_v5\UV4\UV4.exe"
if not defined UV4 (
    echo ERROR: Keil5 not found.
    exit /b 2
)
start "" /wait "%UV4%" -b "%PROJECT%" -t "%TARGET%" -j0 -o "%LOG%"
set "BUILD_RC=%ERRORLEVEL%"
if exist "%LOG%" type "%LOG%"
exit /b %BUILD_RC%
