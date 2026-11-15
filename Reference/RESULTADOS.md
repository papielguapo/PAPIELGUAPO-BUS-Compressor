# Verificación — VCA SSL 1.2.1

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

Versión 1.0.1: eliminado únicamente el botón BYPASS del frontal y sus etiquetas; se mantiene el parámetro de bypass del host para compatibilidad. DSP sin cambios.

Frontal final 1.0.1: retirados también FADE, EXT SC y SC Hz/MIX/TRIM/RATE; permanecen cinco mandos, POWER y METER. Parámetros conservados para sesiones existentes.

## Versión 1.1.0: FIX

Igualador RMS de salida respecto a la entrada original, situado donde estaba BYPASS. No modifica la señal que entra al compresor. Apagado por defecto; parámetro añadido al final para conservar los índices anteriores.

Pruebas nuevas con seno de 997 Hz a amplitudes 0.05 y 0.5, 44.1 y 96 kHz; incluyen compensación de Make-Up y Trim, conservación de balance L/R, reducción de ganancia idéntica con FIX activado/desactivado, guardado de FIX, silencio exacto y POWER apagado. Al desactivar FIX se recupera exactamente la salida DSP de referencia tras la transición. Error RMS medido entre los segundos 7 y 9: +0.0256 / −0.0208 dB a 44.1 kHz y +0.0644 / −0.0646 dB a 96 kHz. Todos los errores están por debajo de 0.1 dB. No se afirma igualación perceptual LUFS ni ausencia de variaciones con música; la compensación RMS necesita estabilización.

## Versión 1.2.0: KI

Transferencia armónica tomada de la implementación local de PWM TONE BOY (`processCraneTriodeEven`) y filtro Butterworth de cuarto orden a 18 Hz con los mismos Q. Envolvente de nivel adaptada al VCA, sin añadir la cadena PWM/DAC/EQ ni el sobremuestreo del otro plugin. Parámetro KI añadido al final y apagado por defecto. Transición de 20 ms.

Seno de 1 kHz y amplitud 0.2, threshold +15: H2 = −62.0248 dBc y H3 = −157.202 dBc. Pruebas aprobadas de predominio H2, diferencia con ruta limpia, bloqueo DC, atenuación infrasonora mayor de 40 dB a 5 Hz, silencio exacto desde reposo, persistencia y FIX RMS junto con KI (error menor de 0.1 dB). Las pruebas previas de compresión y FIX siguen pasando. Capturas reales de KI OFF y ON revisadas; el piloto morado pequeño se ilumina sólo en ON.

## Auditoría 1.2.1

Pluginval nivel 10 SUCCESS y batería DSP ampliada aprobada. FIX mejorado a menos de 0.0005 dB de error en los cuatro casos medidos. Ver [informe completo](../Auditoria.md) y [log final](pluginval-level10.txt).
