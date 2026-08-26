@echo off
setlocal EnableExtensions
chcp 65001 >nul 2>&1

cd /d "%~dp0"

where gcc >nul 2>&1
if errorlevel 1 (
    echo [Ошибка] gcc не найден. Добавьте в PATH: C:\msys64\ucrt64\bin
    exit /b 1
)

where cmake >nul 2>&1
if errorlevel 1 (
    echo [Ошибка] cmake не найден.
    exit /b 1
)

if exist build-mingw\CMakeCache.txt (
    echo Сохраняем настройки и рекорды...
    if exist build-mingw\mini_games_records.ini (
        copy /y build-mingw\mini_games_records.ini "%TEMP%\six_seven_records.bak" >nul
    )
    if exist build-mingw\six_seven.ini (
        copy /y build-mingw\six_seven.ini "%TEMP%\six_seven_settings.bak" >nul
    )
    if exist build-mingw\information (
        copy /y build-mingw\information "%TEMP%\six_seven_information.bak" >nul
    )
    echo Удаляем старую сборку...
    rmdir /s /q build-mingw
)

echo Сборка: %CD%
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -B build-mingw
if errorlevel 1 goto :fail

cmake --build build-mingw -j
if errorlevel 1 goto :fail

if exist "%TEMP%\six_seven_records.bak" (
    copy /y "%TEMP%\six_seven_records.bak" build-mingw\mini_games_records.ini >nul
)
if exist "%TEMP%\six_seven_settings.bak" (
    copy /y "%TEMP%\six_seven_settings.bak" build-mingw\six_seven.ini >nul
)
if exist "%TEMP%\six_seven_information.bak" (
    copy /y "%TEMP%\six_seven_information.bak" build-mingw\information >nul
)

if exist "build-mingw\Six_Seven.exe" (
    echo.
    echo [OK] build-mingw\Six_Seven.exe
    echo Запуск: build-mingw\Six_Seven.exe
) else (
    echo [Ошибка] exe не найден.
    exit /b 1
)
exit /b 0

:fail
echo Сборка не удалась.
exit /b 1
