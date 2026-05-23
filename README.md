# SISTEMAS OPERATIVOS - SISTEMA DE FICHEROS

Autoras: Paula Cristina Morro Negre, Maria Teresa Sbert Gomila

## DESCRIPCIÓN DEL PROYECTO:
Implementación de un sistema de ficheros basado en una estructura de inodos, desarrollado por capas (niveles). El sistema permite la gestión de un disco virtual mediante el uso de un mapa de bits para bloques e inodos, y la organización de datos en directorios.

## ESTRUCTURA DE CAPAS IMPLEMENTADA:
1. **Carpeta tests:** Carpeta donde se almacenan los scripts de prueba. 
2. **Capa de Bloques (bloques.c):** Gestión del hardware virtual.
3. **Capa de Ficheros Básica (ficheros_basico.c):** Inicialización de estructuras (SB, MB, AI) y gestión de inodos/bloques.
4. **Capa de Ficheros (ficheros.c):** Lectura/escritura de bytes lógicos y gestión de STAT.
5. **Capa de Directorios (directorios.c):** Gestión de la jerarquía de rutas, búsqueda de entradas y extracción de caminos.

## SINTAXIS DE LOS COMANDOS:
- **mi_mkfs:** ``./mi_mkfs <nombre_dispositivo> <nbloques>``
- **escribir:** ``./escribir <nombre_dispositivo> <"texto"> <diferentes_inodos>``
- **leer:** ``./leer <nombre_dispositivo> <ninodo>``
- **truncar:** ``./truncar <nombre_dispositivo> <ninodo> <nbytes>``
- **permitir:** ``./permitir <nombre_dispositivo> <ninodo> <permisos>``
- **truncar:** ``./truncar <nombre_dispositivo> <ninodo> <nbytes>``
- **mi_cat:** ``./mi_cat <nombre_dispositivo> </ruta_fichero>`` Muestra todo el contenido de un fichero 
- **mi_chmod:** ``./mi_chmod <nombre_dispositivo> <permisos> </ruta>`` Cambia los permisos de un fichero o directorio 
- **mi_escribir:** ``./mi_escribir <nombre_dispositivo> </ruta_fichero> <texto> <offset>`` Escribe texto en un fichero desde el offset indicado 
- **mi_link:** ``./mi_link <disco> </ruta_original> </ruta_enlace>`` Crea un enlace a un fichero
- **mi_ls:** ``./mi_ls <nombre_dispositivo> </ruta>`` ``./mi_ls -l <nombre_dispositivo> </ruta>`` Lista el contenido de un directorio o muestra los datos de un fichero 
- **mi_mkdir:** ``./mi_mkdir <disco> <permisos> </ruta>`` Crea un directorio 
- **mi_rm_r:** ``mi_rm_r <disco> <ruta_directorio>`` Borrado recursivo 
- **mi_rm:** ``./mi_rm <disco> <ruta_fichero>`` Borra un fichero 
- **mi_rmdir:** ``./mi_rmdir <disco> <ruta_directorio>`` Borra un directorio vacio 

## RESTRICCIONES DEL PROGRAMA:
- El nombre de fichero/directorio está limitado a 60 caracteres (TAMNOMBRE).
- El sistema tiene un BLOCKSIZE de 1024 bytes.

## MEJORAS REALIZADAS:
#### Optimización de la estructura de los inodos
Se ha implementado el inodo utilizando una 'union' con una estructura anónima y un array de padding fijo (INODOSIZE). Esto garantiza que el tamaño del inodo en disco sea siempre constante e independiente de la arquitectura o del alineamiento interno del compilador, evitando errores de cálculo manual en el relleno de la estructura y facilitando el acceso a sus campos.
#### Optimización en la liberación de los bloques
Se ha implementado la "Versión 2" de la función de liberación con dos mejoras de saltos:
   - Mejora 1 (Salto por eliminación): Se saltan los bloques lógicos restantes que no requieren exploración tras haber eliminado un bloque de punteros completo (detección mediante memcmp() con un bloque de ceros).
   - Mejora 2 (Salto por puntero nulo): Se saltan los bloques lógicos que no es necesario explorar cuando se detecta que un puntero a un bloque de punteros ya está a 0, evitando descender innecesariamente en la jerarquía.
   - Control de Escritura: Se utiliza un array 'bloque_modificado' para ejecutar 
    'bwrite()' solo en aquellos bloques de punteros que realmente han cambiado.
#### Implementación del comando mi_touch
Se ha separado la funcionalidad de creación de archivos y directorios, creando el comando específico mi_touch para la creación exclusiva de ficheros, paralelo a la funcionalidad de mi_mkdir.
#### Mejoras en el listado de directorios (mi_ls)
Se han implementado todas las mejoras puntuables sugeridas para el listado:
   - Listado extendido: Soporte para el parámetro -l que muestra los metadatos extendidos del inodo (tipo, permisos, fecha de modificación mtime y tamaño en bytes) tabulados como en los sistemas Linux reales.
   - Diferenciación visual: Uso de colores para distinguir fácilmente entre ficheros y directorios en el listado generado.
   - Soporte para ficheros individuales: Se permite ejecutar el comando mi_ls indicando la ruta directa de un fichero, mostrando sus datos en una única fila sin incluir la cabecera de "Total de entradas".
#### Implementación de caché de directorios
Se han optimizado las operaciones de lectura (mi_read) y escritura (mi_write) mediante la implementación de una caché de directorios. Esta caché almacena las últimas rutas consultadas y sus correspondientes identificadores (p_inodo), para evitar accesos repetitivos a disco y lecturas redundantes del directorio. Soporta tres algoritmos de reemplazo distintos:
   - Caché de entrada única: Almacena exclusivamente la última ruta y su inodo correspondiente para operaciones inmediatas consecutivas.
   - Estrategia FIFO (First-In, First-Out): Emplea un array de múltiples entradas gestionado de manera circular para reemplazar la ruta que lleva más tiempo almacenada.
   - Estrategia LRU (Least Recently Used): Implementa un algoritmo de reemplazo basado en marcas de tiempo, expulsando de la caché la entrada que lleva más tiempo sin ser consultada o modificada.
#### Separación de comandos de borrado (mi_rmdir)
Se ha creado el comando mi_rmdir, separando la semántica de borrado para que mi_rm actúe sobre ficheros y mi_rmdir actúe estrictamente sobre directorios que se encuentren vacíos.
#### Funcionalidad Extra: Renombrar (mi_rn)
Implementación de mi_rn, el cual permite cambiar el nombre de una entrada (fichero o directorio).  
#### Funcionalidad Extra: Borrado Recursivo (mi_rm_r)
Implementación del programa mi_rm_r, que elimina directorios que no están vacíos. Navega de forma recursiva por toda la estructura del árbol, eliminando correctamente todo el contenido (ficheros y subdirectorios anidados) y liberando todos los inodos y bloques de datos involucrados. 
