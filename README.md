# VCA SSL COMPRESSOR — papielguapo

**Creador: [papielguapo](https://github.com/papielguapo)**

![VCA SSL COMPRESSOR — frontal actual](Preview.png)

[Descargar para Windows x64: VST3, Standalone y código](https://github.com/papielguapo/VCASSL/releases/latest)

Compresor VCA de bus basado en el frontal vectorial de [FET67](https://github.com/papielguapo/FET67). Panel de aluminio, mandos violetas, piloto, firma y lienzo 1634 × 252, con escala del 50 al 250 %. **Siempre se abre al mínimo: 817 × 126 (50 %), también al reabrir el editor o cargar sesiones antiguas.** Se puede ampliar durante el uso.

## Controles actuales — 1.3.1

- Threshold −15…+15 dB; Make-Up −5…+15 dB.
- Attack 0.1/0.3/1/3/10/30 ms; Release 0.1/0.3/0.6/1.2 s/Auto; Ratio 2/4/10.
- POWER enciende/apaga el procesamiento; apagado entrega señal seca. El bypass del DAW también sigue disponible.
- SC FILTER, a la izquierda de METER, activa un pasa-altos de 80 Hz **sólo en el detector**. Reduce la influencia del bombo/subgrave en la compresión sin ecualizar el audio. ON/OFF suavizado; piloto morado pequeño encendido en ON. Apagado por defecto.
- METER alterna GR y OUT. OUT conserva la referencia RMS original: 0 = −18 dBFS RMS. No es un medidor true peak.
- FIX iguala el RMS final de salida al RMS original de entrada para comparar al mismo volumen. No modifica el detector ni la señal de entrada. Apagado por defecto.

Doble clic restaura valores y el popup indica el ajuste. Sin botón Analog ni ruido generado.

## KI integrado y siempre activo

KI ya no tiene interruptor visible. Se integra siempre en la ruta procesada, con **+25 dB en el componente armónico** respecto a 1.2.1: `x + 17.7827941 * k2*x*x/(1 + 0.25*x*x)`, con `k2 = (0.008 + 0.012*(1-g)) * gate`. No se amplifica la señal limpia 25 dB ni se añade un generador de ruido. La intensidad cae a niveles de entrada muy bajos.

La función armónica y el filtro infrasonoro Butterworth de cuarto orden a 18 Hz proceden de PWM TONE BOY, adaptados al VCA. Se aplican después de la reducción de ganancia y antes de Make-Up y FIX. El filtro de 18 Hz sí forma parte de la ruta de audio integrada; SC FILTER a 80 Hz afecta exclusivamente al detector. No se copia la cadena PWM/DAC/EQ del otro plugin ni su sobremuestreo 4×. El color más intenso también puede aumentar la distorsión, intermodulación y aliasing. POWER OFF y bypass del host dejan la señal original sin KI.

## FIX

RMS sin ponderación, integración de 400 ms, adaptación de 750 ms y transición ON/OFF de 50 ms. Referencia: canal de mayor RMS, con corrección común L/R. Límite ±24 dB; la corrección se congela cuando entrada o salida quedan por debajo de −70 dBFS RMS. Necesita unos segundos de estabilización. Iguala RMS, no LUFS, y no garantiza picos por debajo de 0 dBFS. Compensa también Make-Up y Trim. No añade ruido ni limitador.

## DSP y compatibilidad

Detector feed-forward enlazado en estéreo, rodillas suaves y release Auto propio. Referencia de rangos: [manual de Waves](https://assets.wavescdn.com/pdf/plugins/ssl-g-master-buss-compressor.pdf). Modelo independiente sin afiliación Waves/SSL ni equivalencia sonora 1:1. Las mediciones SSLComp guardadas son referencias históricas de versiones anteriores; no certifican la respuesta actual con KI integrado.

Latencia cero; se declara un segundo de cola para el filtro KI. Protección de entradas NaN/Inf y amplitudes absurdas mayores de ±1 000 000 (+120 dBFS), sin limitación de picos musicales normal. Parámetros antiguos de Mix/Trim/Fade/sidechain externo se conservan para sesiones y automatización existentes, neutros en estados nuevos. El parámetro legado KI se conserva pero se ignora: KI está siempre activo. El antiguo valor numérico de SC Filter se conserva por compatibilidad pero el interruptor actual utiliza 80 Hz fijos.

## Instalación y compilación

Copiar la carpeta completa `Binaries/VCA SSL.vst3` a `C:/Program Files/Common Files/VST3` (puede requerir administrador), y volver a escanear el DAW. `Binaries/VCA SSL.exe` es Standalone. Fabricante Papi, plugin VcSL, bundle com.papielguapo.vcassl. No sustituye FET67.

C++17, JUCE 9.0.5 y Visual Studio con C++/Windows SDK. CMake usa C:/JUCE o descarga JUCE. Respetar la licencia JUCE correspondiente a la distribución.

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DVCASSL_COPY_PLUGIN_AFTER_BUILD=OFF
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

[Informe de auditoría](Auditoria.md) · [Log pluginval nivel 10](Reference/pluginval-level10.txt) · [Pruebas DSP](Reference/Tests.txt).

![SC FILTER ON — piloto morado pequeño](Preview-SC-ON.png)
