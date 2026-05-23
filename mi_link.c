/**
 * @file mi_link.c
 * @author Paula Cristina Morro Negre 
 * @author Maria Teresa Sbert Gomila
 * 
 * Crea un enlace a un fichero
*/

#include "directorios.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_link <disco> </ruta_original> </ruta_enlace>\n" RESET);
        return FALLO;
    }

    char *disco = argv[1];
    char *ruta_original = argv[2];
    char *ruta_enlace = argv[3];

    if (bmount(disco) == FALLO) return FALLO;

    int err = mi_link(ruta_original, ruta_enlace);
    if (err < 0) {
        mostrar_error_buscar_entrada(err);
        bumount();
        return FALLO;
    }

    if (bumount() == FALLO) return FALLO;

    return EXITO;
}