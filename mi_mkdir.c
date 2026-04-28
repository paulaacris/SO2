/**
 * @file mi_mkdir.c
 * @author Paula Cristina Morro Negre
 * @author Maria Teresa Sbert Gomila
 *
 * Sintaxis: ./mi_mkdir <disco> <permisos> </ruta_directorio/>
 * Crea un directorio. La ruta debe acabar en '/' para indicar que es un directorio.
 */

#include "directorios.h"

int main(int argc, char *argv[]) {
    // Comprobar que se han pasado los argumentos correctos
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: %s <disco> <permisos> </ruta>\n" RESET, argv[0]);
        return FALLO;
    }

    char *disco = argv[1];
    unsigned char permisos = atoi(argv[2]);
    char *ruta = argv[3];
    int err;

    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, RED "Error: modo inválido: <<%d>>" RESET, permisos);
        return FALLO;
    }

    // comprobar que se trata de un directorio
    if (ruta[strlen(ruta) - 1] != '/') {
        fprintf(stderr, RED "Error: la ruta no se corresponde a un directorio: %s\n" RESET, ruta);
        return FALLO;
    }

    // montamos el disco
    if (bmount(disco) == FALLO) return FALLO;

    if ((err = mi_creat(ruta, permisos)) < 0) {
        mostrar_error_buscar_entrada(err);
        if (bmount(disco) == FALLO) return FALLO;
        return FALLO;
    }

    // desmontamos el disco
    if (bumount(disco) == FALLO) return FALLO;
    return EXITO;
}