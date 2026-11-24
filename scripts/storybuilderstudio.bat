@echo off
setlocal enabledelayedexpansion

:: ==========================================
:: CUSTOM CONFIGURATION
:: ==========================================
:: Set CUSTOM_QT_PATH or LOCAL_DLL_PATH in your Windows environment variables,
:: or override them below if your dependencies are in non-standard locations.
if not defined CUSTOM_QT_PATH set "CUSTOM_QT_PATH="
if not defined LOCAL_DLL_PATH set "LOCAL_DLL_PATH="

:: Check if custom Qt path is set and valid
if defined CUSTOM_QT_PATH (
    if exist "!CUSTOM_QT_PATH!" (
        echo [INFO] Using custom Qt path: !CUSTOM_QT_PATH!
        set "QT_BIN=!CUSTOM_QT_PATH!"
        goto LAUNCH
    )
)

echo [INFO] Scanning for local Qt 6.9+ installations...

:: Lists of common compiler profiles to look for
set "PROFILES=llvm-mingw_64 msvc2022_64 msvc2019_64 mingw_64"

:: Scan C:\Qt in descending order for version >= 6.9
if exist "C:\Qt" (
    for /f "delims=" %%V in ('dir /b /ad /o-n C:\Qt 2^>nul') do (
        for /f "tokens=1,2 delims=." %%A in ("%%V") do (
            set "MAJOR=%%A"
            set "MINOR=%%B"

            if "!MAJOR!"=="6" (
                if !MINOR! GEQ 9 (
                    for %%P in (%PROFILES%) do (
                        if exist "C:\Qt\%%V\%%P\bin" (
                            set "QT_BIN=C:\Qt\%%V\%%P\bin"
                            echo [SUCCESS] Found Qt 6.9+ at: !QT_BIN!
                            goto LAUNCH
                        )
                    )
                )
            )
        )
    )
)

:: If not found in C:\Qt, check standard system PATH as a last resort
where windeployqt >nul 2>nul
if %errorlevel% equ 0 (
    echo [INFO] Qt tools detected in system PATH. Attempting launch...
    goto LAUNCH_DIRECT
)

echo [ERROR] Could not automatically locate a Qt 6.9+ installation.
echo [ERROR] Set CUSTOM_QT_PATH in your environment or edit 'storybuilderstudio.bat'.
pause
exit /b 1

:LAUNCH
set "PATH=!QT_BIN!;%PATH%"

:LAUNCH_DIRECT
:: Inject custom external DLL path into environment if specified
if defined LOCAL_DLL_PATH (
    if exist "!LOCAL_DLL_PATH!" (
        echo [INFO] Injecting local DLL path into environment: !LOCAL_DLL_PATH!
        set "PATH=!LOCAL_DLL_PATH!;%PATH%"
    ) else (
        echo [WARN] LOCAL_DLL_PATH does not exist: !LOCAL_DLL_PATH!
    )
)

set "SCRIPT_DIR=%~dp0"
"%SCRIPT_DIR%appStoryBuilderStudio.exe" %*
set "APP_EXIT=%errorlevel%"

if not "%APP_EXIT%"=="0" (
    echo.
    echo [ERROR] Story Builder Studio exited with code %APP_EXIT%.
    pause
    exit /b %APP_EXIT%
)

exit /b 0