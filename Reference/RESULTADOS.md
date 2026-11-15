# Verificación — VCA SSL 1.0

Base FET67: commit ecd6fc7f37d69fa31560461fd76dd5188ffe9069. Compilación Windows x64, MSVC 19.51, Visual Studio 2026, JUCE local 9.0.5. VST3 y Standalone generados. No incluye instalador ni firma digital.

Sin parámetro Analog, sin generador de ruido ni saturación. Silencio digital exacto comprobado durante 64 000 muestras con procesamiento activo. Pruebas de pendientes 2/4/10, enlace estéreo, mezcla seca, bypass, power, ataque, todos los ajustes de release, fade, persistencia, mono y sidechain externo; 44.1/48/96 kHz. Todas pasan.

Referencia instalada: Waves SSLComp Stereo V14.12, Analog Off, Threshold 0, Make-Up 0, Attack 10 ms, Release Auto, Ratio 4:1, Mix 100%, Trim 0. Seno de 1 kHz, medición durante el último segundo de cada señal de 3 segundos a 48 kHz. La referencia se ha cargado mediante el host offline del proyecto FET67 anterior. Los archivos sólo contienen mediciones y metadatos públicos.

| Entrada pico dBFS | Waves salida RMS | VCA SSL salida RMS |
| --- | --- | --- |
| -48 | -51.0099 | -51.0099 |
| -36 | -39.0099 | -39.0099 |
| -30 | -33.0099 | -33.0099 |
| -24 | -27.9631 | -27.2081 |
| -18 | -25.7658 | -25.1422 |
| -12 | -23.9902 | -23.7232 |
| -6 | -22.3650 | -22.2868 |
| 0 | -20.8079 | -20.8130 |

La diferencia máxima en estos ocho puntos es 0.755 dB. La rodilla y las envolventes son propias; esto no constituye equivalencia sonora 1:1. Sólo se ha ajustado la calibración a esta curva, no una matriz completa de ratios, umbrales, frecuencias y transitorios. La traza STEP de Waves se conserva como referencia sin afirmar coincidencia. La sección FREQUENCY del host heredado arrastra historia previa del detector y no se utiliza para caracterizar la respuesta en frecuencia.

Preview.png se renderizó desde el editor real y se revisó visualmente. No se ha realizado prueba auditiva ni matriz de compatibilidad de DAWs.
