#include "ficheros_basico.h"

/*
 *  calcla el tamaño en bloques del mapa de bits (cada bit representa un bloque) 
 *  @param nbloques numero de bloques del disco
 *  @return cantidad de bloques que ocupa MB
 */
int tamMB (unsigned int nbloques) {
    // bytes que necesitamos para todos los bits 
    unsigned int nbytes = nbloques / 8;
    if ((nbloques % 8) != 0) {
        nbytes ++;
    }

    // bloques que necesitamso para los bytes
    unsigned int nbloquesMB = nbytes / BLOCKSIZE;
    if ((nbytes % BLOCKSIZE) != 0){
        nbloquesMB ++;
    }
    
    return nbloquesMB;
}

/*
 *  calcla el tamaño en bloques del array de inodos 
 *  @param ninodos numero de inodos calculado de manera heurística (nbloques/4)
 *  @return cantidad de bloques que ocupa AI
 */
int tamAI(unsigned int ninodos){
    
    unsigned int nbloquesAI = (ninodos * INODOSIZE) / BLOCKSIZE;

    if ((ninodos * INODOSIZE) % BLOCKSIZE != 0){
        nbloquesAI ++;
    }
    
    return nbloquesAI;
}

/*
 *  inicializa los datos del superbloque
 *  @param nbloques bunero de bloques 
 *  @param ninodos numero de inodos calculado de manera heurística (nbloques/4)
 *  @return 0 si se ha inicializado correctamente, -1 en caso de error
 */
int initSB(unsigned int nbloques, unsigned int ninodos){  
    struct superbloque SB;
    SB.posPrimerBloqueMB = posSB + tamSB;
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques-1;
    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0; // inicialmente
    SB.cantBloquesLibres = nbloques; // inicialmente
    SB.cantInodosLibres = ninodos; // inicialmente
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    // no restamos a cantBloquesLibres el bloque de SB porqu elo hace luego initMB

    if (bwrite(posSB, &SB) == FALLO) { // escribimos SB en posSB
        perror(RED"Error al escribir el SuperBloque en initSB\n"); printf(RESET);
        return FALLO;
    } 

    return EXITO;
}

/*
 *  Inicializa el mapa de bits poniendo a 1 los bits que representan los metadatos (ocupados)  
 *  @return 0 si se ha inicializado correctamente, -1 en caso de error
 */
int initMB() {
    struct superbloque SB;
    unsigned char bufferMB[BLOCKSIZE]; // cada posicion del buffer es un byte

    // leemos el SB
    if (bread(posSB, &SB) == FALLO) {
        perror(RED"Error al leer el SuperBloque en initMB\n"); printf(RESET);
        return FALLO;
    }

    // bloques que ocupan los metadatos
    int tamMetadatos = tamSB + tamMB(SB.totBloques) + tamAI(SB.totInodos);

    // bytes y bits equivalen esos bloques
    int bytesCompletos = tamMetadatos / 8;
    int bitsRestantes = tamMetadatos % 8;

    // escribir los bloques completos 
    int bloquesCompletosMB = bytesCompletos / BLOCKSIZE;
    memset(bufferMB, 255, BLOCKSIZE); // Llenamos el buffer de 1

    int i; // valor persistente para saber por que bloque vamso 
    for (i = 0; i < bloquesCompletosMB; i++) {
        if (bwrite(SB.posPrimerBloqueMB + i, bufferMB) == FALLO) {
            perror(RED"Error al escribir bloque completo en initMB\n"); printf(RESET);
            return FALLO;
        }
    }

    // ahora llenamos los bloques no completos 
    memset(bufferMB, 0, BLOCKSIZE); // limpiamos el buffer

    // bytes restantes que no han completado un bloque entero 
    int bytesRestantes = bytesCompletos % BLOCKSIZE;
    for (int j = 0; j < bytesRestantes; j++) {
        bufferMB[j] = 255; // ponemos a 1 los primeros bytes
    }

    // si sobran bits los ponemos en el byte siguiente
    if (bitsRestantes > 0) {
        // los ponemos al principio del byte 
        bufferMB[bytesRestantes] = 255 << (8 - bitsRestantes);
    }

    // escribimos el bloque restante
    if (bwrite(SB.posPrimerBloqueMB + i, bufferMB) == FALLO) {
        perror(RED"Error al escribir el bloque residual en initMB\n"); printf(RESET);
        return FALLO;
    }

    // actualizar el SB restando los bloques ocupados
    SB.cantBloquesLibres -= tamMetadatos;
    
    // guardamos la nueva informacion del SB
    if (bwrite(posSB, &SB) == FALLO) {
        perror(RED"Error al actualizar el SuperBloque en initMB\n"); printf(RESET);
        return FALLO;
    }

    return EXITO; 
}

/*
 *  Inicializa los inodos libres. Enlaza todos los inodos entre si 
 *  @param nbloques numero de bloques del dispositivo
 *  @return 0 si se ha inicializado correctamente, -1 en caso de error
 */
int initAI() {
    struct superbloque SB;
    // buffer de inodos 
    // porque el disco lee por bloques (BLOCKSIZE/INODOSIZE inodos a la vez)
    inodo_t inodos[BLOCKSIZE / INODOSIZE]; 

    // leemos el SB
    if (bread(posSB, &SB) == FALLO) {
        perror(RED"Error al leer el SuperBloque en initAI\n"); printf(RESET);
        return FALLO;
    }

    // siguiente inodo libre 
    int sigInodoLibre = SB.posPrimerInodoLibre + 1; // si SB.posPrimerInodoLibre es 0, el siguiente será el 1

    // recorremos cada bloque del array de inodos 
    for (int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++) {
        // leemos el bloque 
        if (bread(i, inodos) == FALLO) {
            perror(RED"Error al leer bloque de inodos en initAI\n"); printf(RESET);
            return FALLO;
        }

        // recorremos cada inodo del bloque
        for (int j = 0; j < (BLOCKSIZE / INODOSIZE); j++) {
            inodos[j].tipo = 'l'; // marcamos el inodo como libre

            // si quedan mas inodos 
            if (sigInodoLibre < SB.totInodos) {
                // punterosDirectos[0] para enlazar la lista de inodos libres
                inodos[j].punterosDirectos[0] = sigInodoLibre;
                sigInodoLibre ++;
            } else {
                // si es el ultimo inodo apunta a null
                inodos[j].punterosDirectos[0] = UINT_MAX;
            } 
        }

        // escribimos el bloque de inodos ya enlazados 
        if (bwrite(i, inodos) == FALLO) {
            perror(RED"Error al escribir el bloque de inodos en initAI\n"); printf(RESET);
            return FALLO;
        } 
    }
    return EXITO;
}