/**
 * @file mi_rm_r.c
 * @author Paula Cristina Morro Negre 
 * @author Maria Teresa Sbert Gomila
 * 
 * Borrado recursivo
*/

#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: mi_rm_r <disco> <ruta_directorio>\n" RESET);
        return FALLO;
    }

    const char *disco = argv[1];
    const char *ruta = argv[2];

    // comprobar que la ruta sea la de un fichero
    if (ruta[strlen(ruta) - 1] != '/') {
        fprintf(stderr, "Error: La ruta especificada es un fichero. Utiliza mi_rm para ficheros.\n");
        return FALLO;
    }

    if (bmount(disco) == FALLO) return FALLO;

    int err = mi_rm_r(ruta);
    if (err < 0) {
        mostrar_error_buscar_entrada(err);
        bumount();
        return FALLO;
    }

    if (bumount() == FALLO) return FALLO;
    
    return EXITO;
}