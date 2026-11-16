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

if exist "C:\msys64\ucrt64\lib\emscripten\emcc.exe" (
    set "PATH=C:\msys64\ucrt64\lib\emscripten;C:\msys64\ucrt64\bin;%PATH%"
)

where emcc >nul 2>nul
if errorlevel 1 (
    echo emcc was not found. Install Emscripten and add it to PATH.
    exit /b 1
)

where em++ >nul 2>nul
if errorlevel 1 (
    echo em++ was not found. Install Emscripten and add it to PATH.
    exit /b 1
)

set MAKE_CMD=make
where make >nul 2>nul
if errorlevel 1 (
    set MAKE_CMD=mingw32-make
    where mingw32-make >nul 2>nul
    if errorlevel 1 (
        echo GNU Make was not found. Install it and add it to PATH.
        exit /b 1
    )
)

if not exist build\web mkdir build\web

%MAKE_CMD% -C "%RAYLIB_DIR%\src" clean
if errorlevel 1 exit /b 1

%MAKE_CMD% -C "%RAYLIB_DIR%\src" PLATFORM=PLATFORM_WEB RAYLIB_LIBTYPE=STATIC
if errorlevel 1 exit /b 1

em++ -std=c++17 -O2 -Wall -Wextra -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 ^
    -I"%RAYLIB_DIR%\src" main.cpp -L"%RAYLIB_DIR%\src" -lraylib ^
    -sUSE_GLFW=3 -sASYNCIFY -sALLOW_MEMORY_GROWTH=1 ^
    -sMIN_WEBGL_VERSION=1 -sMAX_WEBGL_VERSION=2 ^
    -o "build\web\index.html"
if errorlevel 1 exit /b 1

echo Built build\web\index.html
