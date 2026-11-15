# Auditoría VCA SSL COMPRESSOR 1.3.1 — papielguapo

Fecha: 15 de noviembre de 2026. Windows x64 Release, MSVC 19.51 / Visual Studio 2026 y JUCE 9.0.5.

## Resultado final

**Pluginval 1.0.4, nivel 10: SUCCESS, código de salida 0.** Validada la copia distribuida en `Binaries/VCA SSL.vst3`, versión reportada 1.3.1, semilla 671301. Sin omitir pruebas de GUI. Tasas 44.1/48/96 kHz y bloques 64/128/256/512/1024. [Log íntegro](Reference/pluginval-level10.txt).

**Batería DSP: APROBADA**, con 8 648 639 comprobaciones internas (incluyen verificaciones por muestra, no son millones de casos independientes). [Log](Reference/Tests.txt). No se detectaron fallos bloqueantes en el alcance ejecutado.

SHA-256 del binario compilado y distribuido: `8E92C7EF3937CC68D1A7A5BC811A49D0435A7E81B4EC9852E270E8029ECA43C6`.

## Configuración auditada

- KI integrado permanentemente, sin interruptor visible, con +25 dB únicamente sobre el componente armónico respecto a 1.2.1.
- Su antiguo interruptor controla SC FILTER: pasa-altos de 80 Hz sólo en el detector. Piloto morado pequeño en ON; apagado por defecto.
- POWER y bypass del host entregan señal seca; FIX iguala RMS de salida con entrada original sin modificar el detector.
- Los parámetros legados KI y SC numérico se conservan para no desplazar identificadores/índices. Sus valores no cambian KI integrado ni los 80 Hz del interruptor actual.

## Mediciones y regresiones

| Comprobación | Resultado |
| --- | --- |
| KI, seno 1 kHz y amplitud 0.2, threshold +15 | H2 −37.0247 dBc; H3 −168.608 dBc |
| Refuerzo H2 frente al caso 1.2.1 (−62.0248 dBc) | +25.0001 dB |
| Ruido de entrada muy bajo, pseudoaleatorio determinista | Ganancia RMS −0.0043171 dB; no aumenta en este ensayo |
| Silencio desde reposo / tras reset | Cero digital exacto |
| Filtro infrasonoro KI de 18 Hz | Atenuación a 5 Hz mayor de 40 dB; DC bloqueado |
| SC a 50 Hz, amplitud 0.5 | GR OFF 13.2608 dB / ON 9.00415 dB |
| SC con detector sin comprimir | RMS y armónicos del audio idénticos OFF/ON |
| FIX a 44.1 kHz | Errores +0.0004520 / −0.0004320 dB |
| FIX a 96 kHz | Errores +0.0004630 / −0.0004371 dB |
| Pendiente para un incremento de entrada de 6.0206 dB | 2:1 → 2.97525; 4:1 → 1.48963; 10:1 → 0.614215 dB |

FIX conserva la reducción de ganancia exactamente igual al comparador con FIX apagado. Tras desactivarlo se recupera la ruta base exacta al concluir la transición. KI permanece activo incluso al cargar estados con el antiguo parámetro KI apagado. Persistencia de SC/FIX y estado antiguo sin SC: aprobados.

Comprobados mono/estéreo, bloques 0/1/7/31/127/511/1024, 8/48/192/384 kHz y cuatro combinaciones SC/FIX; reset, POWER, bypass y sidechain externo heredado. Las 30 combinaciones Attack/Release quedan estables en la prueba de cambios abruptos. Los casos NaN, infinitos y máximos float no contaminan salida ni medidores; se comprueba recuperación posterior.

Pluginval cubre carga fría/caliente, interfaz, editor durante procesamiento, estado/restauración, automatización con y sin editor, parámetros entre hilos, buses y fuzz de parámetros. Las imágenes del editor real se revisaron con SC OFF y ON.

## Revisión del código y límites

Se conserva la protección numérica y la cola declarada de un segundo del filtro KI; latencia cero. En la revisión manual de processBlock no se observaron asignaciones, acceso a archivos, locks ni llamadas a GUI. No se ha instrumentado el hilo de audio con sanitizers o contadores de asignaciones.

KI mantiene la etapa sin sobremuestreo: con +25 dB armónicos pueden aumentar aliasing e intermodulación. El ensayo de ruido prueba silencio y un fondo bajo; no garantiza ausencia de productos no lineales sobre cualquier material. SC sólo filtra el detector, pero el filtro infrasonoro integrado de KI sí actúa sobre el audio por debajo de 18 Hz.

FIX aproxima RMS y requiere estabilización; no es LUFS ni limitador true peak. No se garantiza ausencia de clipping con ajustes extremos. No se realizó sesión auditiva ni matriz completa de DAWs. No se afirma equivalencia sonora 1:1 con Waves. El validador independiente del SDK VST3 fue omitido por pluginval al no tener ruta configurada, y no se presenta como aprobado; auval no corresponde a este VST3 Windows.

Reproducción:

```powershell
ctest --test-dir build -C Release --output-on-failure
pluginval --strictness-level 10 --random-seed 671301 --validate "C:/Program Files/Common Files/VST3/VCA SSL.vst3"
```

## Apertura mínima — 1.3.1

El constructor del editor usa siempre 817 × 126 (50 %), independientemente del tamaño guardado en el estado. El redimensionado durante el uso sigue disponible. Las capturas se generaron sin forzar el tamaño y sus cabeceras PNG confirman 817 × 126. Audio sin cambios respecto a 1.3.0. Se repitieron la batería DSP y pluginval nivel 10 sobre el VST3 distribuido, con resultado aprobado.
