# Spectrum Planetary (VST3) - Guia rapida

## Carpeta del proyecto
    SpectrumPlanetary/
    ├─ CMakeLists.txt          (receta de compilacion)
    ├─ Source/                 (el "cerebro" en C++)
    │   ├─ PluginProcessor.h / .cpp
    │   └─ PluginEditor.h / .cpp
    ├─ ui/index.html           (la interfaz que viste en la web)
    └─ .github/workflows/build.yml   (solo para compilar en GitHub)

## Opcion A - Sin instalar nada (GitHub)
1. Crea un repositorio en github.com y sube TODO el contenido de esta carpeta (incluida .github).
2. Pestana Actions -> espera el check verde.
3. Entra a esa ejecucion -> Artifacts -> descarga Spectrum-Planetary-VST3.
4. Copia la carpeta "Spectrum Planetary.vst3" a C:\Program Files\Common Files\VST3

## Opcion B - En tu PC con VS Code
Instala: Git, CMake, Visual Studio Build Tools 2022 (carga "Desarrollo para escritorio con C++"),
VS Code + extensiones "CMake Tools" y "C/C++".
En VS Code: File > Open Folder > SpectrumPlanetary, y en la terminal:
    cmake -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release
Resultado: build\SpectrumPlanetary_artefacts\Release\VST3\  (y una version Standalone para probar sin DAW)

## Uso
Doble clic sobre METERS = reiniciar LUFS integrado y maximos.
Clic en LUFS = cambia a True Peak / Peak / RMS.  Clic en PHASE CORRELATION = vectorscopio.
