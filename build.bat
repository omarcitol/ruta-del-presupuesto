@echo off
setlocal

if not defined RAYLIB_DIR (
    echo Set RAYLIB_DIR to the raylib 5.5 source directory first.
    exit /b 1
)

if not exist "%RAYLIB_DIR%\src\raylib.h" (
    echo raylib.h was not found under "%RAYLIB_DIR%\src".
    exit /b 1
)

where g++ >nul 2>nul
if errorlevel 1 (
    echo g++ was not found. Install MinGW-w64 and add it to PATH.
    exit /b 1
)

set MAKE_CMD=make
where make >nul 2>nul
if errorlevel 1 (
    set MAKE_CMD=mingw32-make
    where mingw32-make >nul 2>nul
    if errorlevel 1 (
        echo make was not found. Install MinGW-w64 and add it to PATH.
        exit /b 1
    )
)

if not exist build mkdir build

%MAKE_CMD% -C "%RAYLIB_DIR%\src" clean
if errorlevel 1 exit /b 1

%MAKE_CMD% -C "%RAYLIB_DIR%\src" PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=STATIC
if errorlevel 1 exit /b 1

g++ -std=c++17 -O2 -Wall -Wextra -I"%RAYLIB_DIR%\src" main.cpp ^
    -L"%RAYLIB_DIR%\src" -lraylib -lopengl32 -lgdi32 -lwinmm ^
    -static -static-libgcc -static-libstdc++ -Wl,--subsystem,windows ^
    -o "build\ruta-del-presupuesto.exe"
if errorlevel 1 exit /b 1

echo Built build\ruta-del-presupuesto.exe
