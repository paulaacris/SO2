#include "bloques.h"

// $ ./mi_mkfs <nombre_dispositivo> <nbloques>

/*
 *  lee el bloque físico especificado en nbloque
 *  @param argc numero de parametros
 *  @param argv vector de punteros a los parámetros 
 *  @return numero de bytes que ha podido escribir (BLOCKSIZE o -1 si ha habido error)
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        perror(RED "Error: " RESET "Número de argumentos incorrecto. \nUso: ./mi_mkfs <nombre_dispositivo> <nbloques>\n");
        return -1;
        
    }
 
    char camino = argv[1]; 
    int nbloques = argv[2]; 

    // inicializamos buffer
    unsigned char buffer[BLOCKSIZE];
    // lo llenamos todo de 0
    memset(buffer, 0, BLOCKSIZE);


    // 1. montar el dispositivo virtual 
    if (bmount(camino) == -1 ) {
        perror(RED"Error al abrir/crear el fichero"RESET);
    }

    // 2. inicializamos el fichero a 0s con bwrite
    for (int i = 0; i < nbloques; i++) {
        if (bwrite(i, buffer) == -1 ) {
        perror(RED"Error al escribir en el fichero"RESET);
        }
    }

    // 3. desmontamos el dispositivo virtual
    if (bumount() == -1) {
        perror(RED"Error al cerrar el fichero"RESET);
    }
}