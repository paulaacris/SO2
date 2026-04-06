# SISTEMAS OPERATIVOS - SISTEMA DE FICHEROS

Autoras: Paula Cristina Morro Negre, Maria Teresa Sbert Gomila

## DESCRIPCIÓN DEL PROYECTO:
Implementación de un sistema de ficheros basado en una estructura de inodos, desarrollado por capas (niveles). El sistema permite la gestión de un disco virtual mediante el uso de un mapa de bits para bloques e inodos, y la organización de datos en directorios.

## ESTRUCTURA DE CAPAS IMPLEMENTADA:
1. **Capa de Bloques (bloques.c):** Gestión del hardware virtual.
2. **Capa de Ficheros Básica (ficheros_basico.c):** Inicialización de estructuras (SB, MB, AI) y gestión de inodos/bloques.
3. **Capa de Ficheros (ficheros.c):** Lectura/escritura de bytes lógicos y gestión de STAT.
4. **Capa de Directorios (directorios.c):** Gestión de la jerarquía de rutas, búsqueda de entradas y extracción de caminos.

## SINTAXIS DE LOS COMANDOS:
- **mi_mkfs:** ``./mi_mkfs <nombre_dispositivo> <nbloques>``
- **escribir:** ``./escribir <nombre_dispositivo> <"texto"> <diferentes_inodos>``
- **leer:** ``./leer <nombre_dispositivo> <ninodo>``
- **truncar:** ``./truncar <nombre_dispositivo> <ninodo> <nbytes>``
- **permitir:** ``./permitir <nombre_dispositivo> <ninodo> <permisos>``
- **truncar:** ``./truncar <nombre_dispositivo> <ninodo> <nbytes>``

## RESTRICCIONES DEL PROGRAMA:
- El nombre de fichero/directorio está limitado a 60 caracteres (TAMNOMBRE).
- El sistema tiene un BLOCKSIZE de 1024 bytes.

## MEJORAS REALIZADAS:
### Optimización de la estructura de los inodos
Se ha implementado el inodo utilizando una 'union' con una estructura anónima y un array de padding fijo (INODOSIZE). Esto garantiza que el tamaño del inodo en disco sea siempre constante e independiente de la arquitectura o del alineamiento interno del compilador, evitando errores de cálculo manual en el relleno de la estructura y facilitando el acceso a sus campos.
2. **Optimización en la liberación de los bloques:** Se ha implementado la "Versión 2" de la función de liberación con dos mejoras de saltos:
   - Mejora 1 (Salto por eliminación): Se saltan los bloques lógicos restantes que no requieren exploración tras haber eliminado un bloque de punteros completo (detección mediante memcmp() con un bloque de ceros).
   - Mejora 2 (Salto por puntero nulo): Se saltan los bloques lógicos que no es necesario explorar cuando se detecta que un puntero a un bloque de punteros ya está a 0, evitando descender innecesariamente en la jerarquía.
   - Control de Escritura: Se utiliza un array 'bloque_modificado' para ejecutar 
     'bwrite()' solo en aquellos bloques de punteros que realmente han cambiado.
*******************************************************************************
