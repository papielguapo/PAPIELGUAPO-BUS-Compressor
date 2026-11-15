# VCA SSL — papielguapo

**Creador: [papielguapo](https://github.com/papielguapo)**

![VCA SSL de papielguapo: frontal de aluminio y controles violetas](Preview.png)

[Descargar VCA SSL para Windows x64 (VST3 + Standalone + código)](https://github.com/papielguapo/VCASSL/releases/latest)

Compresor de bus VCA original basado en el frontal vectorial de [FET67](https://github.com/papielguapo/FET67). Conserva el panel de aluminio, mandos violetas, piloto, firma, tornillos, medidor y lienzo 1634 × 252. Cambian nombre, escalas y funciones; los controles adicionales son necesarios para cubrir la referencia.

## Controles

Threshold −15…+15 dBu; Make-Up −5…+15 dB; Attack 0.1/0.3/1/3/10/30 ms; Release 0.1/0.3/0.6/1.2 s/Auto; Ratio 2/4/10; SC Filter Off o 21…500 Hz (valores 1…20 equivalen a 21 Hz); Mix 0…100%; Trim −18…+18 dB; Rate 1…60 s. Sin modo Analog, saturación ni ruido añadido. Power y Bypass entregan señal seca; Meter alterna GR/OUT. FADE OUT activa fundido a cero; desactivarlo vuelve a unidad desde la posición actual. EXT SC utiliza el bus auxiliar cuando el host lo habilita; en caso contrario utiliza entrada interna. Doble clic restaura el valor inicial y el popup indica el valor exacto.

Referencia de rangos: [manual oficial Waves](https://assets.wavescdn.com/pdf/plugins/ssl-g-master-buss-compressor.pdf). No incluye código, recursos ni presets de Waves. Modelo propio, sin afiliación ni certificación Waves/SSL. No se afirma equivalencia sonora 1:1. Auto, knee y autofade son implementaciones propias; se ha medido SSLComp Stereo V14.12 instalado para aproximar la calibración del detector, con Analog Off. El control Fade es de dos estados, sin el tercer estado de cancelación descrito en su manual.

## DSP

Detector de pico feed-forward, enlace estéreo permanente y misma ganancia en ambos canales. Calibración del detector +24.1 dB respecto a dBFS pico, ajustada con senoidal de 1 kHz frente a SSLComp V14.12, threshold 0, ratio 4:1, ataque 10 ms y release Auto. Rodilla suave 6/3/1 dB para ratio 2/4/10. Attack y Release son constantes de tiempo exponenciales del control de ganancia; Auto depende de la historia del nivel. El filtro pasa-altos sólo afecta al detector. Sin limitador oculto, sin ganancia automática, sin latencia. La etapa de ganancia es limpia y no incluye modelado de distorsión. OUT conserva el medidor RMS original (0 = −18 dBFS RMS); GR conserva la escala LED original hasta 21 dB.

## Windows x64

Copiar la carpeta completa `Binaries/VCA SSL.vst3` al directorio VST3 del DAW. `Binaries/VCA SSL.exe` es Standalone. Identificador propio `VcSL`, bundle `com.papielguapo.vcassl`: no sustituye FET67 ni carga sus estados. No se instala automáticamente.

## Compilar

C++17, JUCE y Visual Studio con C++/Windows SDK. CMake utiliza C:/JUCE o descarga JUCE 9.0.5. Respetar la licencia JUCE correspondiente al uso y distribución.

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DVCASSL_COPY_PLUGIN_AFTER_BUILD=OFF
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

Pruebas offline incluidas en Tests. No se ha validado una matriz de DAWs ni se ha realizado comparación auditiva. La medición de referencia incluida cubre una curva estática y una traza de transitorio, no todos los ajustes. Las constantes nominales no garantizan transitorios idénticos a hardware u otros plugins.
