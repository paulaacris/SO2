/**
 * @file permitir.c
 * @author Paula Cristina Morro Negre 
 * @author Maria Teresa Sbert Gomila
*/

#include "ficheros.h"

int main(int argc, char *argv[]) {
    // Validamos el número de argumentos introducidos
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: permitir <nombre_dispositivo> <ninodo> <permisos>\n" RESET);
        return FALLO;
    }

    // Guardamos el primer argumento (nombre del dispositivo)
    char *nombre_dispositivo = argv[1];
    // Guardamos el segundo argumento (ninodo)
    int ninodo = atoi(argv[2]);
    // Guardamos el tercer argumento (permisos)
    unsigned char permisos = (unsigned char) atoi(argv[3]);

    // Montamos el dispositivo
    if (bmount(nombre_dispositivo) == FALLO) {
        return FALLO;
    }

    // LLamamos a la función mi_chmod_f de ficheros.c para cambiar los permisos del inodo
    if (mi_chmod_f(ninodo, permisos) == FALLO) {
        fprintf(stderr, RED "Error al cambiar los permisos del inodo %d\n" RESET, ninodo);
        bumount();
        return FALLO;
    }

    // Desmontamos el dispositivo
    if (bumount() == FALLO) {
        return FALLO;
    }

    return EXITO;
}