/**
 * @file mi_ls.c
 * @author Paula Cristina Morro Negre
 * @author Maria Teresa Sbert Gomila
 *
 * Sintaxis: ./mi_ls <disco> </ruta>
 *           ./mi_ls -l <disco> </ruta>    (version extendida)
 *
 * Lista el contenido de un directorio (o muestra los datos de un fichero).
 * La ruta acaba en '/' si es directorio, sin '/' si es fichero.
 */

#include "directorios.h"

int main(int argc, char *argv[]) {
    const char *disco;
    const char *ruta;
    char flag = 'n'; // 'n' = simple, 'l' = extendido

    // comprobamos los argumentos
    // ./mi_ls <disco> </ruta>
    if (argc == 3) {
        disco = argv[1];
        ruta  = argv[2];
        flag  = 'n';
    }
    // ./mi_ls -l <disco> </ruta>
    else if (argc == 4 && strcmp(argv[1], "-l") == 0) {
        disco = argv[2];
        ruta  = argv[3];
        flag  = 'l';
    }
    else {
        fprintf(stderr, RED "Sintaxis: ./mi_ls <nombre_dispositivo> </ruta>\n"
                            "          ./mi_ls -l <nombre_dispositivo> </ruta>\n" RESET);
        return FALLO;
    }

    // determinamos el tipo según la sintaxis del camino
    char tipo = (ruta[strlen(ruta) - 1] == '/') ? 'd' : 'f';

    // montamos el dispositivo
    if (bmount(disco) == FALLO) return FALLO;

    // reservamos el buffer de salida
    char buffer[TAMBUFFER];
    memset(buffer, 0, TAMBUFFER);

    // llamamos a mi_dir para obtener el numero de entradas 
    int entradas = mi_dir(ruta, buffer, tipo, flag);

    if (entradas < 0) {
        if (entradas != FALLO) {
            mostrar_error_buscar_entrada(entradas);
        }
        bumount();
        return FALLO;
    }

    // si es directorio mostramos el total de entradas
    if (tipo == 'd') {
        printf("Total: %d\n", entradas);
    }

    // si hay entradas o es un fichero mostramos cabeceras en modo -l
    if (flag == 'l' && (entradas > 0 || tipo == 'f')) {
        printf("Tipo\t%-5s\t%-20s\t%-10s\tNombre\n", "Modo", "mTime", "Tamaño");
        printf("------------------------------------------------------------------------------------\n");
    }

    // imprimimos el contenido del buffer
    if (entradas > 0 || tipo == 'f') {
        printf("%s", buffer);
    }

    if (bumount() == FALLO) return FALLO;
    return EXITO;
}