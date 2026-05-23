/**
 * @file mi_rn.c
 * @author Paula Cristina Morro Negre 
 * @author Maria Teresa Sbert Gomila
 * 
 * Renombrar fichero / directorio
*/

#include "directorios.h"

int main(int argc, char const *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: ./mi_rn <disco> <ruta/antiguo> <nuevo>\n");
        return FALLO;
    }

    const char *disco = argv[1];
    const char *ruta_antiguo = argv[2];
    const char *nuevo_nombre = argv[3];

    if (bmount(disco) == FALLO) return FALLO;

    int err = mi_rn(ruta_antiguo, nuevo_nombre);
    if (err < 0) {
        mostrar_error_buscar_entrada(err);
        bumount();
        return FALLO;
    }

    if (bumount() == FALLO) return FALLO;

    return EXITO;
}
