/**
 * @file mi_mkfs.c
 * @author Paula Cristina Morro Negre 
 * @author Maria Teresa Sbert Gomila
*/

#include "directorios.h"

// $ ./mi_mkfs <nombre_dispositivo> <nbloques>

/*
 *  lee el bloque físico especificado en nbloque
 *  @param argc numero de parametros 
 *  @param argv vector de punteros a los parámetros 
 *  @return numero de bytes que ha podido escribir (BLOCKSIZE o -1 si ha habido error)
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, RED"Error de sintaxis. \nUso: ./mi_mkfs <nombre_dispositivo> <nbloques>\n"RESET); 
        return FALLO;
    }
 
    char *camino = argv[1]; // porque argv[1] es una dirección de memoria (puntero)
    int nbloques = atoi(argv[2]); // argv[2] lo lee como texto y lo convertimos en int (atoi)
    int ninodos = nbloques/4; // calculado de forma heuristica

    // inicializamos buffer
    unsigned char buffer[BLOCKSIZE];
    // lo llenamos todo de 0
    memset(buffer, 0, BLOCKSIZE);


    // montar el dispositivo virtual 
    if (bmount(camino) == FALLO) {
        fprintf(stderr, RED"Error al abrir/crear el fichero"RESET); 
        return FALLO; // si no se puede abrir/crear el disco no importa seguir
    }

    // inicializamos el fichero a 0s con bwrite
    for (int i = 0; i < nbloques; i++) {
        if (bwrite(i, buffer) == FALLO) {
            fprintf(stderr, RED"Error al escribir en el fichero"RESET);
            return FALLO;
        }
    }

    // inicializamos el SB
    if (initSB(nbloques, ninodos) == FALLO){
        fprintf(stderr, RED"Error al inicializar el SB"RESET); 
        return FALLO;
    }

    // inicializamos el MB
    if (initMB() == FALLO){
        fprintf(stderr, RED"Error al inicializar el MB"RESET);
        return FALLO;
    }

    // inicializamos el AI
    if (initAI() == FALLO){
        fprintf(stderr, RED"Error al inicializar el AI"RESET); 
        return FALLO;
    }

    // crear el directorio raiz (tipo directorio con permisos rwx)
    int inodoRaiz = reservar_inodo('d', 7);
    if (inodoRaiz == FALLO) {
        fprintf(stderr, RED"Error al reservar el inodo raiz"RESET); 
        return FALLO;
    }

    // 3. desmontamos el dispositivo virtual
    if (bumount() == FALLO) {
        fprintf(stderr, RED"Error al cerrar el fichero"RESET);
        return FALLO;
    }
}