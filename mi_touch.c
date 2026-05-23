/**
 * @file mi_touch.c
 * @author Paula Cristina Morro Negre
 * @author Maria Teresa Sbert Gomila
 *
 * Crea un fichero
*/

#include "directorios.h"

int main(int argc, char *argv[]) {
    // comprobamos los argumentos
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_touch <nombre_dispositivo> <permisos> </ruta_fichero>\n" RESET);
        return FALLO;
    }

    const char *disco = argv[1];
    int permisos_int = atoi(argv[2]);
    const char *ruta = argv[3];

    // los permisos deben estar entre 0 y 7
    if (permisos_int < 0 || permisos_int > 7) {
        fprintf(stderr, RED "Error: modo inválido: <<%d>>\n" RESET, permisos_int);
        return FALLO;
    }

    // la ruta NO debe acabar en '/' (es un fichero)
    if (ruta[strlen(ruta) - 1] == '/') {
        fprintf(stderr, RED "Error: Camino incorrecto (un fichero no puede acabar en '/').\n" RESET);
        return FALLO;
    }

    // montamos el dispositivo
    if (bmount(disco) == FALLO) return FALLO;
    

    // llamamos a mi_creat para crear el fichero
    int error = mi_creat(ruta, (unsigned char)permisos_int);
    if (error != EXITO) {
        mostrar_error_buscar_entrada(error);
        bumount(disco);
        return FALLO;
    }

    // desmontamos el dispositivo
    if (bumount(disco) == FALLO) return FALLO;
    
    return EXITO;
}