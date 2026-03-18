@echo off
setlocal

where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [A_State] CMake not found. Please install CMake and add it to PATH.
    pause & exit /b 1
)

set "GENERATOR="

if exist "C:\Program Files\Microsoft Visual Studio\2022" (
    set "GENERATOR=Visual Studio 17 2022"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022" (
    set "GENERATOR=Visual Studio 17 2022"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019" (
    set "GENERATOR=Visual Studio 16 2019"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017" (
    set "GENERATOR=Visual Studio 15 2017"
)

if "%GENERATOR%"=="" (
    echo [A_State] No supported Visual Studio version found (2017/2019/2022).
    echo [A_State] Please install Visual Studio with C++ Desktop workload.
    pause & exit /b 1
)

echo [A_State] Using generator: %GENERATOR%
echo [A_State] Building x86 Release...

if not exist build mkdir build
cd build

cmake .. -G "%GENERATOR%" -A Win32 -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo [A_State] CMake configure failed.
    pause & exit /b 1
)

cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [A_State] Build failed.
    pause & exit /b 1
)

echo.
echo [A_State] Build complete! DLL and injector are in the build\ folder.
pause
