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

/*
 *  Escribe el valor de bit (0 - ocupado, 1 - libre) en el bit de MB que representa el nbloque  
 *  @param nbloque bloque fisico que queremos indicar si esta libreo o ocupado
 *  @return 
 */
int escribir_bit(unsigned int nbloque, unsigned int bit) {
    unsigned char bufferMB[BLOCKSIZE];
    // leer el SB
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror(RED"Error al leer el SuperBloque en escribir_bit\n"); printf(RESET);
        return FALLO;
    }

    int posbyteMB = nbloque / 8; // byte relativo al MB que contiene el bit que representa nbloque
    int posbit = nbloque % 8; // posicion del bit que representa nbloque relativo al byte
    int nbloqueMB = posbyteMB / BLOCKSIZE; // bloque relativo al MB donde esta el posbyte 
    int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB; // posicion absoluta donde se encentra el nbloqueMB

    int posbyte = posbyteMB % BLOCKSIZE; // posicion del posbyteMB relativa a un bloque

    // leemos nbloqueabs
    if (bread(nbloqueabs, bufferMB) == FALLO) {
        perror(RED"Error al leer el bloque en escribir_bit\n"); printf(RESET);
        return FALLO;
    }

    // modificamos el bit
    unsigned char mascara = 128; // 10000000
    mascara >>= posbit; // desplazamiento a la derecha

    if (bit == 1) {
        // para poner a 1 el bit
        bufferMB[posbyte] |= mascara; // operador OR para bits
    }

    if (bit == 0) {
        // para poner a 1 el bit
        bufferMB[posbyte] &= ~mascara; // operador AND y NOT para bits
    }

    // escribimos el buffer el el disco en la posicion nbloqueabs
    if (bwrite(nbloqueabs, bufferMB) == FALLO) {
        perror(RED"Error al escribir el bloque en escribir_bit\n"); printf(RESET);
        return FALLO;
    } 
    return EXITO;
}

/*
 *  Lee un bit del MB y devuelve el valor del bit leído 
 *  @param nbloque 
 *  @return valor del bit (0 o 1)
 */
char leer_bit(unsigned int nbloque) {
    struct superbloque SB;
    unsigned char bufferMB[BLOCKSIZE];

    // leer el SB
    if (bread(posSB, &SB) == FALLO) {
        perror(RED"Error al leer el SuperBloque en leer_bit\n"); printf(RESET);
        return FALLO;
    }

    // posicionamiento igual que en escribir_bit
    int posbyteMB = nbloque/8;
    int posbit = nbloque % 8;
    int nbloqueMB = posbyteMB / BLOCKSIZE;
    int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;
    int posbyte = posbyteMB % BLOCKSIZE;

    // leemos nbloqueabs
    if (bread(nbloqueabs, bufferMB) == FALLO) {
        perror(RED"Error al leer el bloque en escribir_bit\n"); printf(RESET);
        return FALLO;
    }

    unsigned char mascara = 128; // 10000000
    mascara >>= posbit; // desplazamos el 1 de la mascara a la derecha las veces que indique el posbit
    mascara &= bufferMB[posbyte]; // and de la mascara con el byte, el resultado se guarda en la mascara 

    if (mascara == 0) { 
        // si la mascara queda a 0 es que el bit era 0
        return 0;
    } else {
        // si es cualquier otra cosa que no sea 0 es que es 1
        return 1;
    }
}

/*
 *  Encuentra el primer bloque libre (consultando MB) y lo ocupa (poniendo el bit a 1)
 *  @return posición del primer bloque libre
 */
int reservar_bloque() {
    struct superbloque SB;
    unsigned char bufferMB[BLOCKSIZE];
    unsigned char bufferAux[BLOCKSIZE];

    // leer el SB
    if (bread(posSB, &SB) == FALLO) {
        perror(RED"Error al leer el SuperBloque en leer_bit\n"); printf(RESET);
        return FALLO;
    }

    // 1. comprobamos que hay bloques libres 
    if (SB.cantBloquesLibres == 0) {
        // si no quedan bloques libres
        return FALLO;
    }

    // 2. encontrar el primer bloque logico de datos libre 
    // iteramos sobre los bloques de MB buscando el primero que no este todo a 1 (255)

    // llenamos el bufferAux todo de 1
    memset(bufferAux, 255, BLOCKSIZE);

    int nbloqueMB;
    for (nbloqueMB = 0; nbloqueMB <= SB.posUltimoBloqueMB - SB.posPrimerBloqueMB; nbloqueMB++) {
        // leemos el siguiente bloque del MB
        if (bread(SB.posPrimerBloqueMB + nbloqueMB, bufferMB) == FALLO) {
            perror(RED"Error al leer el bloque en reservar_bloque\n"); printf(RESET);
            return FALLO;
        }

        // comparamos el bloque leido con el auxiliar
        if (memcmp(bufferAux, bufferMB, BLOCKSIZE) != 0) { // si no son iguales (0 -> iguales)
            // ya hemos encontrado el primer bloque con algun 0, dejamos de buscar
            break; 
        } 
    }
    // al salir del bucle tenemos en bufferMB el primer bloque libre

    // 3. encontramos el primer byte del bloque anterior que no este todo a 1 (!= 255)
    // iteramos el bloque byte a byte hasta encontrar uno no todo a 1s

    int posbyte;
    for (posbyte = 0; posbyte < BLOCKSIZE; posbyte++) { // en un bloque hay BLOCKSIZE bytes
        if (bufferMB[posbyte] < 255) { // si es menos a 255 tiene algun bit a 0 
            // hemso encontrado el byte libre
            break;
        } 
    }

    // 4. encontramos el primer bit a 0 dentro de ese byte 
    unsigned char mascara = 128; // 10000000
    int posbit = 0;

    while (bufferMB[posbyte] & mascara) {
        bufferMB[posbyte] <<= 1; // desplazamos el bloque un bit a la izquierda
        posbit ++;
    }

    // 4. a que bloque fisico corresponde ese bit
    int nbloque = (nbloqueMB * BLOCKSIZE + posbyte) * 8 + posbit;

    // 5. escribimos el bit a 1
    if (escribir_bit(nbloque, 1) == FALLO) {
        perror(RED"Error al escribir bit en reservar_bloque\n"); printf(RESET);
        return FALLO;
    }

    // actualizmos el SB
    SB.cantBloquesLibres --;
    if (bwrite(posSB, &SB) == FALLO) { // escribimos SB en posSB
        perror(RED"Error al escribir el SuperBloque en reservar_bloque\n"); printf(RESET);
        return FALLO;
    } 

    // limpiamos el bloque de datos
    unsigned char buffer[BLOCKSIZE];
    memset(buffer, 0, BLOCKSIZE); // lo llenamos todo de 0
    if (bwrite(nbloque, buffer) == FALLO) {
        perror(RED"Error al escribir en el fichero"); printf(RESET);
        return FALLO;
    }
 
    return nbloque;
}

int liberar_bloque(unsigned int nbloque) { 

}

int escribir_inodo(unsigned int ninodo, inodo_t *inodo) {

}

int leer_inodo(unsigned int ninodo, inodo_t *inodo) {

}

int reservar_inodo(unsigned char tipo, unsigned char permisos) {

}