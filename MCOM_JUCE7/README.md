# MCOM — JUCE 7 / VST3

Plugin VST3 de compresión óptica experimental para Windows x64.

## Estructura
- `CMakeLists.txt`: configuración JUCE/CMake.
- `Source/PluginProcessor.h/.cpp`: DSP y parámetros.
- `Source/PluginEditor.h/.cpp`: interfaz.
- `.github/workflows/build.yml`: compilación automática en GitHub Actions.

## Compilar automáticamente en GitHub
1. Crea un repositorio llamado `MCOM` en GitHub.
2. Sube todos los archivos y carpetas de este proyecto, incluida la carpeta `.github`.
3. Abre la pestaña **Actions** y ejecuta el workflow **Build MCOM VST3** (o usa **Run workflow**).
4. Al terminar en verde, abre la ejecución y descarga el artefacto `MCOM-Windows-VST3`.
5. Extrae el ZIP y copia `MCOM.vst3` a `C:\Program Files\Common Files\VST3`.
6. En FL Studio: Options → Manage plugins → Find installed plugins.

## Compilación local (opcional)
Con JUCE 7 en `C:\JUCE` y CMake/Visual Studio 2022: `cmake -S . -B build -DJUCE_DIR=C:/JUCE -G "Visual Studio 17 2022" -A x64` y luego `cmake --build build --config Release --target MCOM_VST3`.

## Controles y DSP
Peak Reduction, Gain, Limit, Mix, Input, Output y Valve Color. Detector estéreo enlazado con HPF de 100 Hz, ataque nominal de 10 ms, recuperación rápida de 60 ms más cola program-dependent aproximada de 0.5–5 s, histéresis aproximada y saturación tanh sobremuestreada 4x. No es una emulación física exacta de una celda T4.

**Nota:** el proyecto es código fuente; GitHub Actions debe compilarlo antes de instalar el VST3. No se ha verificado todavía una compilación real en GitHub.
