/**
 * @file mi_chmod.c
 * @author Paula Cristina Morro Negre
 * @author Maria Teresa Sbert Gomila
 *
 * Cambia los permisos de un fichero o directorio.
*/

#include "directorios.h"

int main(int argc, char *argv[]) {
    // comprobamos los argumentos
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_chmod <nombre_dispositivo> <permisos> </ruta>\n" RESET);
        return FALLO;
    }

    const char *disco = argv[1];
    int permisos_int  = atoi(argv[2]);
    const char *ruta  = argv[3];

    // los permisos deben estar entre 0 y 7
    if (permisos_int < 0 || permisos_int > 7) {
        fprintf(stderr, RED "Error: modo inválido: <<%d>>\n" RESET, permisos_int);
        return FALLO;
    }

    // montamos el dispositivo
    if (bmount(disco) == FALLO) return FALLO;

    // llamamos a mi_chmod 
    int error = mi_chmod(ruta, (unsigned char)permisos_int);
    if (error != EXITO) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return EXITO;
    }

    // desmontamos el dispositivo
    if (bmount(disco) == FALLO) return FALLO;
    
    return EXITO;
}