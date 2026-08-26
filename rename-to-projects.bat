@echo off
chcp 65001 >nul
setlocal

set "VZORG=d:\VZorg"
set "OLD=%VZORG%\проекты"
set "NEW=%VZORG%\projects"

if exist "%NEW%" (
    echo Папка уже есть: %NEW%
    goto :check
)

if not exist "%OLD%" (
    echo Не найдена: %OLD%
    echo Если уже переименовали — откройте %NEW%\67_helper
    exit /b 1
)

echo Закройте Cursor и все окна в папке проекты, затем Enter...
pause

ren "%OLD%" projects
if errorlevel 1 (
    echo Не удалось переименовать. Закройте программы, использующие папку.
    exit /b 1
)

:check
if exist "%NEW%\67_helper" (
    echo.
    echo [OK] Проект теперь здесь:
    echo   %NEW%\67_helper
    echo.
    echo В Cursor: File - Open Folder - %NEW%\67_helper
    echo Сборка:
    echo   cd /d "%NEW%\67_helper"
    echo   build-mingw.bat
) else (
    echo Папка projects есть, но 67_helper не найден внутри.
)

endlocal
