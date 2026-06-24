@echo off
chcp 65001 > nul


for /F "tokens=1,2 delims=#" %%a in ('"prompt #$H#$E# & echo on & for %%b in (1) do rem"') do set "ESC=%%b"
set "BLUE=%ESC%[94m"
set "GREEN=%ESC%[92m"
set "RED=%ESC%[91m"
set "RESET=%ESC%[0m"


echo %BLUE%=== Generating files and Creating translations ===%RESET%
if not exist "build-windows" mkdir "build-windows" > nul
cd "build-windows"
cmake ../.. -G "MinGW Makefiles" > nul
call ./../../translations/compileLanguages.bat > nul


echo %BLUE%=== Project Compilation ===%RESET%

cd /d "%~dp0"
cd "build-windows"
cmake --build . > nul


if not exist "LunusikLauncher.exe" (
    echo %RED%[ERROR] Failed compilation%RESET%
    pause
    exit /b
)


echo %BLUE%=== Creating a folder with all files ===%RESET%

if exist "build" rmdir /S /Q "build" > nul
mkdir "build" > nul

copy /Y "LunusikLauncher.exe" "build/" > nul
cd "build" > nul
windeployqt --no-translations "LunusikLauncher.exe" > nul


echo %GREEN%=== Successful compilation ===%RESET%
pause