# Auditoría VCA SSL COMPRESSOR 1.4.0 — papielguapo

Fecha: 15 de noviembre de 2026. Windows x64 Release, MSVC 19.51 / Visual Studio 2026, JUCE 9.0.5.

## Resultado

**Pluginval 1.0.4 nivel 10: SUCCESS, código de salida 0**, ejecutado sobre la versión instalada en `C:/Program Files/Common Files/VST3/VCA SSL.vst3`. Semilla 671400, GUI habilitada, tasas 44.1/48/96 kHz y bloques 64/128/256/512/1024. Timeout de inactividad ampliado a 120 s, sin deshabilitar pruebas. [Log íntegro](Reference/pluginval-level10.txt).

**Pruebas DSP aprobadas:** 8 937 112 comprobaciones internas, incluyendo verificaciones por muestra; no son millones de casos independientes. [Log](Reference/Tests.txt).

SHA-256 del VST3 compilado, instalado y distribuido: `C289766900AB7D684B0F594A81478DA6B14DE6A1AF88D47691376092AA71DF9D`.

## Cambio de audio

Se ha eliminado KI: no quedan su transferencia, su incremento de 25 dB, su dependencia de GR ni su filtro de 18 Hz en la ruta de audio. Se mantienen los identificadores de parámetros antiguos para compatibilidad; el parámetro KI legado se ignora.

La nueva etapa suma únicamente un residual polinómico de grado tres al audio comprimido: `0.01*x*x - 0.02*x*x*x`. El residual se calcula con interpolación/filtrado FIR 4× (257 taps en cada conversión). La señal limpia se retrasa 64 samples para alinearla, sin pasarla por esos filtros. Se elimina DC con un bloqueador de 1 Hz sólo en el residual añadido.

No hay generador de ruido, dither, gate armónico, saturador adicional, EQ ni pasa-altos de 18 Hz sobre la señal limpia. La protección del residual por encima de +6 dBFS evita crecimiento polinómico extremo y no recorta la ruta limpia. El compresor VCA, SC FILTER de 80 Hz y FIX se conservan. El editor sigue abriendo al mínimo, 817 × 126.

## Mediciones

Condición armónica: seno de 1 kHz, pico 0.2, 48 kHz, threshold +15 para que el compresor no actúe; análisis tras asentamiento.

| Comprobación | Resultado |
| --- | --- |
| H2 | −59.9956 dBc |
| H3 | −73.9750 dBc |
| H4–H12 | Todos por debajo de −110 dBc en el ensayo |
| Tono 18 kHz: alias de H2 a 12 kHz | −150.698 dBc |
| Tono 18 kHz: alias de H3 a 6 kHz | −155.478 dBc |
| Cambio RMS de un fondo de ruido de entrada muy bajo | −0.000285 dB, sin aumento en el ensayo |
| Silencio desde reposo / tras reset | Cero digital exacto |
| Respuesta fundamental a 5 Hz frente a 1 kHz | Diferencia inferior a 0.02 dB; sin corte de 18 Hz |
| FIX, 44.1 kHz | +0.000434 / −0.000468 dB |
| FIX, 96 kHz | +0.000463 / −0.000437 dB |

Frente al alias de aproximadamente −37 dBc de KI +25 dB en el ensayo anterior de 18 kHz, la mejora medida es superior a 110 dB. Los valores cercanos a −150 dBc están condicionados por el render float32 y la precisión del ensayo; no se presentan como una especificación física universal.

## Latencia, bypass y estabilidad

La latencia declarada es **64 samples**: 1.33 ms a 48 kHz. Pruebas de impulsos y bloques partidos confirman el retardo exacto de la mezcla seca y del bypass del host. El bypass continúa alimentando los estados internos para evitar una vuelta con estados reiniciados. La latencia la compensa el DAW según su configuración.

Cola declarada conservadora de dos segundos para la eliminación de DC del residual. La sección informativa inicial de pluginval muestra 0 samples antes del procesamiento; el host offline, después de prepareToPlay, consulta y retira correctamente 64 samples. La cola reportada por pluginval es de dos segundos. POWER y bypass del host conservan la señal seca, alineada con ese retardo.

Pasan las pruebas de pendientes 2/4/10, todas las combinaciones Attack/Release, SC sólo en detector, sidechain externo heredado, FIX sin modificar GR, persistencia, estados antiguos, mono/estéreo, bloques de 0/1/7/31/127/511/1024 y tasas 8/48/192/384 kHz. Las entradas NaN/Inf/máximos float no contaminan salida ni medidores. En la revisión del callback no se observan asignaciones de memoria, archivos, locks ni llamadas a GUI: los buffers FIR son arrays de tamaño fijo.

## Alcance

La etapa genera H2 y H3 en un tono simple dentro del rango normal. Las señales complejas producen intermodulación y la compresión puede generar otros productos; no se promete exclusividad matemática de H2/H3 para cualquier señal o nivel. La protección fuera de rango puede cambiar el espectro con entradas superiores a +6 dBFS.

No se afirma ausencia absoluta de aliasing: se ha medido un caso exigente concreto. No se ha realizado escucha humana ni una matriz completa de DAWs. Pluginval no es certificación sonora ni equivalencia con Waves. El validador independiente del SDK VST3 se omitió al no estar configurado; auval no corresponde al formato Windows. Las diferencias de calibración por ratio y de recuperación Auto descritas en la comparación anterior no se han modificado en esta versión.

Reproducción:

```powershell
ctest --test-dir build -C Release --output-on-failure
pluginval --strictness-level 10 --random-seed 671400 --timeout-ms 120000 --validate "C:/Program Files/Common Files/VST3/VCA SSL.vst3"
```
