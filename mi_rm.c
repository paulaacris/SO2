/**
 * @file mi_rm.c
 * @author Paula Cristina Morro Negre 
 * @author Maria Teresa Sbert Gomila
 * 
 * Borra un fichero
*/

#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: ./mi_rm <disco> <ruta_fichero>\n");
        return -1;
    }

    const char *disco = argv[1];
    const char *ruta = argv[2];

    // comprobar que la ruta sea la de un fichero
    if (ruta[strlen(ruta) - 1] == '/') {
        fprintf(stderr, "Error: La ruta especificada es un directorio. Utiliza mi_rmdir para directorios.\n");
        return FALLO;
    }

    if (bmount(disco) == FALLO) return FALLO;

    int err = mi_unlink(ruta);
    if (err < 0) {
        mostrar_error_buscar_entrada(err);
        bumount();
        return FALLO;
    }

    if (bumount() == FALLO) return FALLO;
    
    return EXITO;
}