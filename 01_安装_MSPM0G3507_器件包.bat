@echo off
setlocal
set "PACK=%~dp0tools\offline_install\TexasInstruments.MSPM0G1X0X_G3X0X_DFP.1.3.1.pack"
if not exist "%PACK%" (
    echo ERROR: Device pack not found:
    echo %PACK%
    pause
    exit /b 2
)
start "" "%PACK%"
