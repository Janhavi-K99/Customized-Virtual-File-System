@echo off
setlocal enabledelayedexpansion

REM ─────────────────────────────────────────────────────────────
REM  Windows packaging script for CVFS File System Explorer
REM  Run from project root: scripts\package_windows.bat
REM ─────────────────────────────────────────────────────────────

set SCRIPT_DIR=%~dp0
set PROJECT_DIR=%SCRIPT_DIR%..
set BUILD_DIR=%PROJECT_DIR%\build
set DIST_DIR=%PROJECT_DIR%\dist
set BUILD_TYPE=Release

echo === Building CVFS (%BUILD_TYPE%) ===

REM 1. Configure CMake
cmake -B "%BUILD_DIR%" -S "%PROJECT_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

REM 2. Build
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed.
    exit /b 1
)

REM 3. Create dist folder
if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%"

REM 4. Copy cvfs_gui.exe (the main GUI binary)
copy "%BUILD_DIR%\%BUILD_TYPE%\cvfs_gui.exe" "%DIST_DIR%\" >nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Could not find cvfs_gui.exe. Trying flat build directory...
    copy "%BUILD_DIR%\cvfs_gui.exe" "%DIST_DIR%\" >nul 2>&1
)
if not exist "%DIST_DIR%\cvfs_gui.exe" (
    echo [ERROR] cvfs_gui.exe not found after build. Aborting.
    exit /b 1
)

REM 5. Run windeployqt to gather Qt runtime DLLs
echo === Running windeployqt ===
where windeployqt >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [WARN] windeployqt not found in PATH. Trying Qt install locations...

    REM Common Qt install paths
    set QT_PATHS=C:\Qt\6.6.3\msvc2019_64\bin
    set QT_PATHS=%QT_PATHS%;C:\Qt\6.5.3\msvc2019_64\bin
    set QT_PATHS=%QT_PATHS%;C:\Qt\6.6.3\mingw_64\bin
    set QT_PATHS=%QT_PATHS%;C:\Qt\6.5.3\mingw_64\bin
    set QT_PATHS=%QT_PATHS%;C:\Qt\5.15.2\msvc2019_64\bin
    set QT_PATHS=%QT_PATHS%;C:\Qt\5.15.2\mingw81_64\bin

    for %%p in (%QT_PATHS%) do (
        if exist "%%p\windeployqt.exe" (
            set "PATH=%%p;!PATH!"
            echo Found windeployqt at %%p
            goto :run_windeployqt
        )
    )

    echo [ERROR] windeployqt not found. Install Qt or add windeployqt to PATH.
    exit /b 1
)

:run_windeployqt
windeployqt "%DIST_DIR%\cvfs_gui.exe" --release --no-translations
if %ERRORLEVEL% neq 0 (
    echo [WARN] windeployqt returned errors. Proceeding anyway...
)

REM 6. Copy optional CLI and original executables
if exist "%BUILD_DIR%\%BUILD_TYPE%\cvfs_cli.exe" (
    copy "%BUILD_DIR%\%BUILD_TYPE%\cvfs_cli.exe" "%DIST_DIR%\" >nul
    echo Copied cvfs_cli.exe
) else if exist "%BUILD_DIR%\cvfs_cli.exe" (
    copy "%BUILD_DIR%\cvfs_cli.exe" "%DIST_DIR%\" >nul
)

if exist "%BUILD_DIR%\%BUILD_TYPE%\cvfs_original.exe" (
    copy "%BUILD_DIR%\%BUILD_TYPE%\cvfs_original.exe" "%DIST_DIR%\" >nul
    echo Copied cvfs_original.exe
) else if exist "%BUILD_DIR%\cvfs_original.exe" (
    copy "%BUILD_DIR%\cvfs_original.exe" "%DIST_DIR%\" >nul
)

REM 7. Copy license
copy "%PROJECT_DIR%\LICENSE" "%DIST_DIR%\" >nul 2>&1

echo.
echo === Packaging complete ===
echo Dist folder: %DIST_DIR%
echo Contents:
dir "%DIST_DIR%" /b
echo.
echo You can now run Inno Setup compiler:
echo   ISCC installer\CVFS_Setup.iss
echo.
endlocal
