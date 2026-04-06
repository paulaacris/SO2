*******************************************************************************
TRABAJO DE SISTEMAS OPERATIVOS - SISTEMA DE FICHEROS
*******************************************************************************

MIEMBROS DEL GRUPO:
- Paula Cristina Morro Negre
- Maria Teresa Sbert Gomila

DESCRIPCIÓN DEL PROYECTO:
Implementación de un sistema de ficheros basado en una estructura de inodos, 
desarrollado por capas (niveles). El sistema permite la gestión de un disco 
virtual mediante el uso de un mapa de bits para bloques e inodos, y la 
organización de datos en directorios.

ESTRUCTURA DE CAPAS IMPLEMENTADA:
1. Capa de Bloques (bloques.c): Gestión del hardware virtual (bmount, bwrite, bread...).
2. Capa de Ficheros Básica (ficheros_basico.c): Inicialización de estructuras (SB, MB, AI) 
   y gestión de inodos/bloques.
3. Capa de Ficheros (ficheros.c): Lectura/escritura de bytes lógicos y gestión de STAT.
4. Capa de Directorios (directorios.c): Gestión de la jerarquía de rutas, búsqueda 
   de entradas y extracción de caminos.

SINTAXIS ESPECÍFICA DE LOS COMANDOS:
- mi_mkfs: ./mi_mkfs <nombre_dispositivo> <nbloques>
- escribir: ./escribir <nombre_dispositivo> <"texto"> <diferentes_inodos>
- (Próximos niveles: mi_mkdir, mi_ls, mi_chmod, etc.)

RESTRICCIONES DEL PROGRAMA:
- Tamaño del nombre de fichero/directorio limitado a 60 caracteres (TAMNOMBRE).
- El sistema de ficheros debe ser montado antes de cualquier operación.
- El nivel de profundidad de directorios está limitado por el diseño de la recursividad 
  en buscar_entrada().
- BLOCKSIZE configurado a 1024 bytes.

MEJORAS REALIZADAS Y OBSERVACIONES:
- Gestión de Errores: Se ha implementado un sistema robusto de mensajes de error 
  en 'directorios.c' (mostrar_error_buscar_entrada) para identificar fallos 
  específicos como "Camino incorrecto", "Permiso denegado" o "Archivo ya existente".
- Optimización de Lectura/Escritura: Uso eficiente de offsets para evitar 
  escrituras innecesarias en bloques no modificados.
- Modularidad: Separación estricta de funciones de bajo nivel (bloques) y alto 
  nivel (directorios) para facilitar el mantenimiento.
- Depuración (Debug): Inclusión de mensajes informativos (vía #if DEBUG) para 
  trazar la resolución de caminos y la asignación de inodos en tiempo real.

*******************************************************************************
