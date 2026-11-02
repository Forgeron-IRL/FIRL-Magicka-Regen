@echo off
setlocal

:: Usage: build.bat [nopause]
:: ── Load the VS 2026 developer environment (local path, adjust if needed) ──
set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat"

if not exist "%VSDEVCMD%" (
    echo [ERROR] VsDevCmd.bat not found : %VSDEVCMD%
    goto :fail
)

call "%VSDEVCMD%" -arch=amd64 -host_arch=amd64 >nul

cd /d "%~dp0"

:: ── CMake configure (only if build/ is not configured yet) ──
:: The first run installs the vcpkg dependencies and compiles CommonLib (slow).
if not exist build\CMakeCache.txt (
    echo [INFO] Configuring CMake...
    cmake -B build -S . -G "Visual Studio 18 2026" -A x64 -T v143 ^
      -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
      -DVCPKG_TARGET_TRIPLET=x64-windows-static-md ^
      -DVCPKG_OVERLAY_TRIPLETS="%~dp0triplets" ^
      -DCMAKE_CXX_FLAGS="/permissive- /Zc:preprocessor /EHsc /MP /W4 /Zc:__cplusplus -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DUNICODE -D_UNICODE"
    if errorlevel 1 (
        echo [ERROR] CMake configure failed.
        goto :fail
    )
)

:: ── Build ──
echo [INFO] Building Release...
cmake --build build --config Release
if errorlevel 1 (
    echo [ERROR] Build failed.
    goto :fail
)

echo.
echo [OK] Build done.
if /i not "%~1"=="nopause" pause
endlocal
exit /b 0

:fail
if /i not "%~1"=="nopause" pause
endlocal
exit /b 1
