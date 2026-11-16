# Ruta del presupuesto

A 2D financial-literacy platform game made with C++ and raylib. The original
prototype uses geometric pixel-inspired art and does not rely on assets from
commercial game franchises.

## Gameplay

Explore three distinct stages: the Barrio del ahorro, Avenida de las cuotas,
and Mercado de las metas. Each stage has its own palette, platform routes,
enemies, and financial lesson. Jump across gaps, climb ladders, collect coins,
and make six decisions about everyday finances. Risky decisions attract debt
enemies. Avoid them, jump on them, or collect a savings seed and press `X` to
fire.

Choose Tranquilo, Aventura, or Reto at the title screen with the left/right
arrows or `1`-`3`. The modes change the starting lives, time limit, and enemy
speed; later stages also introduce faster enemies. Press `P` to pause.

The score rewards careful choices, coins, enemy encounters, and remaining time.
Collected coins add to the available balance; setting money aside changes the
savings total, so the same money is not counted twice. Spending beyond the
available balance is shown as debt. After winning or losing, players can rate
the experience from 1 to 5 or press `Enter` to skip. The best score and
anonymous ratings are stored locally on the device; they are not uploaded or
shared online. All money in the game is fictional.

## Controls

- Move: `A`/`D` or the left/right arrows
- Jump: `Space`, `W`, or the up arrow
- Climb: hold `W`/`S` or the up/down arrows while on a ladder
- Choose a financial decision: `1` or `2`
- Shoot a savings seed: `X` (after collecting one)
- Pause/resume: `P`
- Continue: `Enter`
- Select title-screen difficulty: left/right arrows or `1`-`3`
- Toggle sound: `M`
- Close the game: `Esc`

## Run on Windows

Build the executable from source using the instructions below, then run
`build\ruta-del-presupuesto.exe`. The game draws its visuals and synthesizes
its sound effects at runtime, so it does not need external art or audio files.

## Build on Windows

Requirements: MinGW-w64 with `g++` and GNU Make (or `mingw32-make`), plus the
raylib 5.5 source tree. Set `RAYLIB_DIR` to the root of that tree, then run:

```bat
set RAYLIB_DIR=C:\path\to\raylib-5.5
build.bat
```

## Play in the browser

Play at <https://omarcitol.github.io/ruta-del-presupuesto/>. Press `Space` on the
title screen to start. The public build has been verified to load and start a
game in the browser; a full playthrough has not yet been manually tested.

GitHub Actions builds the WebAssembly version with raylib 5.5 and Emscripten
3.1.64, then deploys it to GitHub Pages after pushes to `main`. For a local
build, `build-web.bat` is available; serve the resulting `build\web` directory
over HTTP rather than opening its HTML file directly.
