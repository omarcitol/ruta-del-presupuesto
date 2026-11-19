# Ruta del presupuesto

A 2D financial-literacy platform game made with C++ and raylib. The original
prototype uses geometric pixel-inspired art and does not rely on assets from
commercial game franchises.

## Gameplay

Explore three distinct stages: the Barrio del ahorro, Avenida de las cuotas,
and Mercado de las metas. Each stage has its own palette, platform routes,
enemies, and financial lesson. Jump across gaps, climb ladders, collect coins,
and face nine financial decisions. The three questions in each stage and the
order of their answers are randomized; answer choices do not reveal which is
safer. Feedback explains hidden fees, debt, and trade-offs, while a risky
choice costs money and brings a pursuer. Coins are worth $250, so careful
choices matter more than collecting every coin.

Debt enemies throw bills, later-stage enemies jump and toss arcing projectiles,
and cannons fire money shots. Dodge them by jumping or stomp enemies from
above. Your savings seeds can shoot enemies with `X`; shots and hazards stop at
stage boundaries. Jump into fruit blocks from below to release seeds. Press
`Down` while standing on a platform to drop through it. Each hit costs one
life, $2,000, and 18 seconds. Reach the vault at the end of the final stage to
complete the run and rate the game; running out of lives or money ends the run.

Choose Tranquilo, Aventura, or Reto at the title screen with the left/right
arrows or `1`-`3`. The modes change the starting lives, time limit, and enemy
speed; enemies get faster in each stage. A hit costs 250 points and 18 seconds,
and losing all lives or running out of time ends the run. Press `P` to pause.

The score rewards careful choices, coins, enemy encounters, and remaining time.
Collected coins add to the available balance; setting money aside changes the
savings total, so the same money is not counted twice. Spending beyond the
available balance is shown as debt. After winning or losing, players can rate
the experience from 1 to 5 upon reaching the vault, or after a loss; press `Enter`
to skip. The best score and anonymous ratings are stored locally on the device;
they are not uploaded or shared online. All money in the game is fictional.

## Controls

- Move: `A`/`D` or the left/right arrows
- Jump: `Space`, `W`, or the up arrow
- Climb: hold `W`/`S` or the up/down arrows while on a ladder
- Choose a financial decision: click an option or press `1`/`2` (applies immediately)
- Shoot a savings seed: `X` (after collecting one)
- Hit a marked block from below: jump into it
- Drop through a platform: `Down` while standing on it
- Pause/resume: `P`
- Continue: `Enter`
- Start a difficulty: click its card or press `1`-`3`; `Space` starts the selected one
- Select title-screen difficulty: left/right arrows
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

Play at <https://omarcitol.github.io/ruta-del-presupuesto/>. Click a difficulty
card or press `1`-`3` to start immediately, or press `Space` to start the
selected difficulty. Decision cards also respond to a click, so no extra
confirmation key is needed. The web version uses a readable font and a clean,
responsive full-screen page.

GitHub Actions builds the WebAssembly version with raylib 5.5 and Emscripten
3.1.64, then deploys it to GitHub Pages after pushes to `main`. For a local
build, `build-web.bat` is available; serve the resulting `build\web` directory
over HTTP rather than opening its HTML file directly.
