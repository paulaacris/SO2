#include "ficheros_basico.h"

// Calcula el tamaño en bloques necesario para el mapa de bits
int tambMB (unsigned int nbloques) {
    // Cada bit representa un bloque -> 1bit = 8bytes
    return (nbloques/8 * BLOCKSIZE);
}

/*
 *  calcla el tamaño en bloques del array de inodos 
 *  @param ninodos numero de inodos calculado de manera heurística (nbloques/4)
 *  @return cantidad de bloques que ocupa AI
 */
int tamAI(unsigned int ninodos){
    int tamaño = (ninodos*INODOSIZE)/BLOCKSIZE;

    if ((ninodos*INODOSIZE)/%BLOCKSIZE != 0){
        tamaño ++;
    }
    
    return tamaño;
}

/*
 *  inicializa los datos del superbloque
 *  @param nbloques bunero de bloques 
 *  @param ninodos numero de inodos calculado de manera heurística (nbloques/4)
 */
int initSB(unsigned int nbloques, unsigned int ninodos){
    
}

/*
 *  Inicializa el mapa de bits poniendo a 1 los bits que representan los metadatos (ocupados)  
 *  @param nbloques numero de bloques del dispositivo
 *  @return 0 si se ha inicializado correctamente, -1 en caso de error
 */
int initMB() {
    
    
    
}