#include "ficheros_basico.h"

// $ ./mi_mkfs <nombre_dispositivo> <nbloques>

/*
 *  lee el bloque físico especificado en nbloque
 *  @param argc numero de parametros 
 *  @param argv vector de punteros a los parámetros 
 *  @return numero de bytes que ha podido escribir (BLOCKSIZE o -1 si ha habido error)
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        perror(RED"Error de sintaxis. \nUso: ./mi_mkfs <nombre_dispositivo> <nbloques>\n"); printf(RESET);
        return FALLO;
    }
 
    char *camino = argv[1]; // porque argv[1] es una dirección de memoria (puntero)
    int nbloques = atoi(argv[2]); // argv[2] lo lee como texto y lo convertimos en int (atoi)
    int ninodos = nbloques/4; // calculado de forma heuristica

    // inicializamos buffer
    unsigned char buffer[BLOCKSIZE];
    // lo llenamos todo de 0
    memset(buffer, 0, BLOCKSIZE);


    // 1. montar el dispositivo virtual 
    if (bmount(camino) == FALLO) {
        perror(RED"Error al abrir/crear el fichero"); printf(RESET);
        return FALLO; // si no se puede abrir/crear el disco no importa seguir
    }

    // 2. inicializamos el fichero a 0s con bwrite
    for (int i = 0; i < nbloques; i++) {
        if (bwrite(i, buffer) == FALLO) {
        perror(RED"Error al escribir en el fichero"); printf(RESET);
        }
    }

    // llamadas a las funciones
    initSB(nbloques, ninodos);
    initMB();
    initAI();

    // 3. desmontamos el dispositivo virtual
    if (bumount() == FALLO) {
        perror(RED"Error al cerrar el fichero"); printf(RESET);
    }
}