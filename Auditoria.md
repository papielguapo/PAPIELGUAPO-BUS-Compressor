# Auditoría actual — BUS Compressor 1.6.1

16/11/2026. Compilación Windows x64 Release. Creador papielguapo.

Instalada en Common Files/VST3. SHA256: 55E0E7EEEF275E0012A5F15EBDF049B563BF9AAC22F9347642D1C8B882E8F21C.

Pruebas DSP PASS, 18.73 s; edición numérica PASS. Pluginval 1.0.4 strictness 10, semilla 671601, SUCCESS sobre el VST3 instalado. Logs actuales en Reference/Tests.txt y Reference/pluginval-level10.txt. Incluyen estados, automatización, buses mono/estéreo, distintas tasas y bloques, entradas hostiles y estabilidad. El validador independiente del SDK VST3 no está configurado.

Auto ya no detecta conexiones por amplitud: un silencio nunca cambia de detector. Utiliza el auxiliar cuando su bus está activo y vuelve al interno cuando el host lo desactiva. Si el DAW mantiene el bus activo tras borrar el envío, Sidechain Source = Internal permite recuperar FIX. External fuerza el detector auxiliar. FIX no actúa con detector externo, aunque la automatización intente activarlo. La interfaz lo apaga y bloquea. Los parámetros obsoletos se conservan no automatizables por compatibilidad.

Los ensayos automatizados no sustituyen la escucha ni una matriz manual de DAWs. FL Studio estaba disponible, pero no se completó una prueba manual del BUS con audio, enrutamiento, guardar/reabrir y automatización. Por tanto esa validación manual sigue pendiente y no se certifica el producto al 100 %. No se ha modificado la sesión de trabajo del 1167; la segunda sesión vacía recibió un EQP-1A durante el intento de carga y no se guardó.

Mediciones de dinámica y armónicos en Reference/Mediciones.json y Validacion-dinamica.json corresponden al DSP de 1.5.0; las modificaciones posteriores afectan edición, nombre y gestión de FIX/sidechain. No equivalencia sonora 1:1 con Waves. Conservar sus límites: diferencias hasta 1.27 dB por frecuencia y transitorios rápidos con discrepancias puntuales de 3.27 dB.
