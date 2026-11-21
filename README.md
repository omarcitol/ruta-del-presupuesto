# Ruta del presupuesto

Videojuego educativo de plataformas creado con C++ y raylib. El jugador
practica decisiones de ahorro y presupuesto mientras recorre tres niveles.

## Entrega

- **Jugar en el navegador:** <https://omarcitol.github.io/ruta-del-presupuesto/>
- **Descargar para Windows:** [ruta-del-presupuesto.exe](https://github.com/omarcitol/ruta-del-presupuesto/releases/download/v1.0.1/ruta-del-presupuesto.exe)
- **Valorar el juego:** [formulario anónimo de 1 a 5](https://docs.google.com/forms/d/e/1FAIpQLSet3XlT92RFc84tNnBzcfdrSUlO59xzuR6hC3hPIj2D5Brbag/viewform)

## Cómo se juega

La partida tiene tres niveles y un único modo, **Aventura**: 180 segundos y
tres vidas. En cada nivel aparece una pregunta; las tres preguntas se eligen
al azar de un banco de 15 situaciones (cinco por nivel). Cada una ofrece tres
opciones en orden aleatorio y las señales se separan aproximadamente diez
segundos de carrera. Algunas decisiones esconden cargos o estafas, y ciertas
trampas pueden costar dinero, tiempo y una vida.

Recoge monedas, escala, salta los huecos y esquiva o derrota a los enemigos.
Las semillas de ahorro permiten disparar con `X`; golpea los bloques marcados
desde abajo para obtener semillas. Al llegar a la bóveda final se completa la
partida y se solicita una valoración. Si se pierden todas las vidas o se acaba
el tiempo, también se puede valorar al terminar.

El saldo y el ahorro son ficticios. El récord y las valoraciones se guardan
localmente; la versión web también envía las calificaciones anónimas al
formulario enlazado arriba.

## Controles

- Mover: `A`/`D` o flechas izquierda/derecha.
- Saltar: `Espacio`, `W` o flecha arriba.
- Escalar: mantener `W`/`S` o flechas arriba/abajo en una escalera.
- Elegir una decisión: hacer clic en una opción o pulsar `1`, `2` o `3`.
- Disparar una semilla: `X`, después de recoger una.
- Atravesar una plataforma hacia abajo: `Abajo` mientras se está sobre ella.
- Pausar o continuar: `P`.
- Iniciar Aventura: hacer clic en `JUGAR`, pulsar `Espacio` o `Enter`.
- Silenciar o activar el sonido: `M`.
- Cerrar el juego: `Esc`.

## Ejecutar en Windows

Compila el juego con MinGW-w64, GNU Make (o `mingw32-make`) y raylib 5.5.
Define `RAYLIB_DIR` con la ruta a la carpeta de raylib y ejecuta:

```bat
set RAYLIB_DIR=C:\ruta\a\raylib-5.5
build.bat
```

El ejecutable se genera en `build\ruta-del-presupuesto.exe`. El directorio
`build/` está excluido de Git porque contiene archivos compilados y datos
locales; no hace falta subirlo al repositorio.

## Versión web

La demo publicada está en <https://omarcitol.github.io/ruta-del-presupuesto/>.
GitHub Actions compila el juego con raylib 5.5 y Emscripten 3.1.64 y lo
despliega en GitHub Pages al actualizar la rama `main`.

Para compilar localmente, configura `RAYLIB_DIR` y ejecuta `build-web.bat`.
Sirve el contenido de `build\web` por HTTP; no abras el archivo HTML
directamente desde el disco.
