/**
 * @file mi_escribir.c
 * @author Paula Cristina Morro Negre
 * @author Maria Teresa Sbert Gomila
 *
 * Escribe texto en un fichero desde el offset indicado
*/

#include "directorios.h"

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, RED "Sintaxis: ./mi_escribir <nombre_dispositivo> </ruta_fichero> <texto> <offset>\n" RESET);
        return FALLO;
    }

    const char *disco = argv[1];
    const char *ruta = argv[2];
    const char *texto = argv[3];
    int offset = atoi(argv[4]);

    if (offset < 0) {
        fprintf(stderr, RED "Error: El offset debe ser un valor no negativo.\n" RESET);
        return FALLO;
    }

    int nbytes = strlen(texto);
    fprintf(stderr, "longitud texto: %d\n", nbytes);

    if (bmount(disco) == FALLO) return FALLO;
    

    // llamamos a mi_write para escribir el contenido en el fichero
    int error = mi_write(ruta, texto, offset, nbytes);
    if (error < 0) {
        if (error == FALLO) {
            fprintf(stderr, RED "No hay permisos de escritura.\n"RESET); 
        } else {
            mostrar_error_buscar_entrada(error);
        } 
        bumount();
        return FALLO;
    }

    fprintf(stderr, "Bytes escritos: %d\n", error);

    // desmontamos el dispositivo
    if (bumount() == FALLO) return FALLO;
    
    return EXITO;
}