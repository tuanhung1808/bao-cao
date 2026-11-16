@echo off
setlocal
cd /d "%~dp0"
if not exist Objects\greenhouse_app.axf (
  echo ERROR: Objects\greenhouse_app.axf not found. Build Greenhouse_APP first.
  exit /b 1
)
where fromelf >nul 2>nul
if errorlevel 1 (
  echo ERROR: fromelf.exe is not in PATH.
  echo Run this script from the Keil MDK command prompt or add Keil\ARM\ARMCC\bin to PATH.
  exit /b 2
)
fromelf --bin --output=Objects\greenhouse_app.bin Objects\greenhouse_app.axf
if errorlevel 1 exit /b 3
echo Created Objects\greenhouse_app.bin
endlocal
