# Auditoría VCA SSL 1.2.1 — papielguapo

Fecha: 15 de noviembre de 2026. Código revisado: procesador, editor, parámetros, persistencia, buses y pruebas. Compilación Windows x64 Release, MSVC 19.51 / Visual Studio 2026, JUCE 9.0.5.

## Resultado

**APROBADO en pluginval 1.0.4, strictness-level 10, código de salida 0 y resultado SUCCESS.** Ejecutado sobre la copia instalada en `C:/Program Files/Common Files/VST3/VCA SSL.vst3`. No se deshabilitaron pruebas de GUI. Semilla 671201 (0xa3de1), tasas por defecto 44.1/48/96 kHz y bloques 64/128/256/512/1024. Log íntegro: [pluginval-level10.txt](Reference/pluginval-level10.txt).

El ejecutable VST3 instalado coincide por SHA-256 con el binario compilado y distribuido:
`68DA9990E90957E31C243C437ACC0A5304B4F60380E5213324BAD5ECE4F8C851`.

## Hallazgos y correcciones

1. **Corregido: entradas extremas podían contaminar KI y los medidores.** La versión 1.2.0 pasó pluginval nivel 10, pero una nueva prueba dirigida con máximos float reprodujo una salida no finita. NaN e infinitos se sustituyen por cero y amplitudes absurdamente altas se acotan a ±1 000 000 (+120 dBFS), también en sidechain y bypass del host. Es una protección numérica fuera del rango normal de audio, no un limitador de picos musicales. La regresión reproduce el fallo anterior y ahora pasa, comprobando además recuperación sin estados inválidos.
2. **Corregido: cola KI declarada como cero.** El filtro infrasonoro puede seguir produciendo una breve cola tras el último sample. Se declara un segundo conservador para que el host no suspenda antes de tiempo la instancia. La latencia sigue siendo cero.
3. **Mejorado: precisión del igualador FIX.** Coeficientes y acumulador de corrección pasan a doble precisión. En las cuatro mediciones de referencia el error absoluto baja de hasta 0.0647 dB a menos de 0.0005 dB, sin cambiar el detector ni la reducción de ganancia.
4. **Interfaz:** añadido COMPRESSOR bajo VCA SSL. Se revisaron las capturas reales con KI OFF y ON; el piloto morado pequeño responde a ON.

## Comprobaciones

- pluginval: carga fría/caliente, editor, editor durante audio, procesamiento, reconfiguración, estado/restauración, automatización, automatización con editor, seguridad de parámetros entre hilos, buses, sidechain y fuzz de parámetros. Resultado SUCCESS.
- Pruebas DSP anteriores aprobadas: ratios, ataque, releases, enlace estéreo, POWER, bypass, mono, sidechain externo, persistencia, silencio, FIX y KI.
- Nuevas regresiones: NaN/Inf/máximos float, recuperación posterior, estado antiguo sin FIX/KI, cero samples, bloques de 1/7/31/127/511/1024, mono/estéreo y las cuatro combinaciones KI/FIX a 8/192/384 kHz. Todas pasan.
- FIX: error RMS +0.0004625/−0.0004377 dB a 44.1 kHz y +0.0004629/−0.0004368 dB a 96 kHz, con seno estable tras asentamiento; detector/GR idénticos con FIX encendido y apagado.
- KI: H2 −62.0248 dBc, H3 −157.202 dBc en el caso de referencia; bloqueo DC, filtro de 18 Hz, silencio desde reposo y coexistencia con FIX aprobados.
- Revisión manual del callback de audio: no se observaron asignaciones de memoria, E/S de archivos, locks ni llamadas a la GUI en processBlock. No se ha realizado instrumentación específica de asignaciones ni análisis de carreras con sanitizers.

Reproducción básica:

```powershell
ctest --test-dir build -C Release --output-on-failure
pluginval --strictness-level 10 --random-seed 671201 --validate "C:/Program Files/Common Files/VST3/VCA SSL.vst3"
```

## Límites del resultado

Pluginval no es una certificación de todos los DAWs ni de equivalencia sonora con Waves. No se ha efectuado una sesión auditiva en el DAW. El validador independiente del SDK VST3 no se ejecutó: pluginval lo indica como omitido por no tener ruta configurada; no se presenta como prueba superada. auval corresponde a Audio Units y no valida este VST3 Windows.

KI sigue sin sobremuestreo, por lo que puede producir aliasing con contenido de alta frecuencia; queda documentado, sin modificar aquí su carácter. FIX iguala RMS suavizado con límite ±24 dB y necesita estabilización: no iguala LUFS ni garantiza picos bajo 0 dBFS. El compresor conserva parámetros heredados ocultos para compatibilidad. No se promete ausencia universal de defectos fuera del alcance probado.
