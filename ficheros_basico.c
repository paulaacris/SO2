#include "ficheros_basico.h"

/*
 *  calcla el tamaño en bloques del mapa de bits (cada bit representa un bloque)
 *  @param nbloques numero de bloques del disco
 *  @return cantidad de bloques que ocupa MB
 */
int tamMB(unsigned int nbloques) {
    // bytes que necesitamos para todos los bits
    unsigned int nbytes = nbloques / 8;
    if ((nbloques % 8) != 0) {
        nbytes++;
    }

    // bloques que necesitamso para los bytes
    unsigned int nbloquesMB = nbytes / BLOCKSIZE;
    if ((nbytes % BLOCKSIZE) != 0) {
        nbloquesMB++;
    }

    return nbloquesMB;
}

/*
 *  calcla el tamaño en bloques del array de inodos
 *  @param ninodos numero de inodos calculado de manera heurística (nbloques/4)
 *  @return cantidad de bloques que ocupa AI
 */
int tamAI(unsigned int ninodos) {

    unsigned int nbloquesAI = (ninodos * INODOSIZE) / BLOCKSIZE;

    if ((ninodos * INODOSIZE) % BLOCKSIZE != 0) {
        nbloquesAI++;
    }

    return nbloquesAI;
}

/*
 *  inicializa los datos del superbloque
 *  @param nbloques bunero de bloques
 *  @param ninodos numero de inodos calculado de manera heurística (nbloques/4)
 *  @return 0 si se ha inicializado correctamente, -1 en caso de error
 */
int initSB(unsigned int nbloques, unsigned int ninodos) {
    struct superbloque SB;

    SB.posPrimerBloqueMB = posSB + tamSB;
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques - 1;
    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0;      // inicialmente
    SB.cantBloquesLibres = nbloques; // inicialmente
    SB.cantInodosLibres = ninodos;   // inicialmente
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    // no restamos a cantBloquesLibres el bloque de SB porqu elo hace luego initMB

    // escribimos SB en posSB
    if (bwrite(posSB, &SB) == FALLO) return FALLO;

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
        fprintf(stderr, RED "Error al leer el SuperBloque en initMB\n"RESET);
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
        if (bwrite(SB.posPrimerBloqueMB + i, bufferMB) == FALLO) return FALLO;
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
    if (bwrite(SB.posPrimerBloqueMB + i, bufferMB) == FALLO) return FALLO;

    // actualizar el SB restando los bloques ocupados
    SB.cantBloquesLibres -= tamMetadatos;

    // guardamos la nueva informacion del SB
    if (bwrite(posSB, &SB) == FALLO) return FALLO;

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
    if (bread(posSB, &SB) == FALLO) return FALLO;

    // siguiente inodo libre
    int sigInodoLibre = SB.posPrimerInodoLibre + 1; // si SB.posPrimerInodoLibre es 0, el siguiente será el 1

    // recorremos cada bloque del array de inodos
    for (int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++) {
        // leemos el bloque
        if (bread(i, inodos) == FALLO) return FALLO;

        // recorremos cada inodo del bloque
        for (int j = 0; j < (BLOCKSIZE / INODOSIZE); j++) {
            inodos[j].tipo = 'l'; // marcamos el inodo como libre

            // si quedan mas inodos
            if (sigInodoLibre < SB.totInodos) {
                // punterosDirectos[0] para enlazar la lista de inodos libres
                inodos[j].punterosDirectos[0] = sigInodoLibre;
                sigInodoLibre++;
            }
            else {
                // si es el ultimo inodo apunta a null
                inodos[j].punterosDirectos[0] = UINT_MAX;
            }
        }

        // escribimos el bloque de inodos ya enlazados
        if (bwrite(i, inodos) == FALLO) return FALLO;
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
    if (bread(posSB, &SB) == FALLO) return FALLO;

    int posbyteMB = nbloque / 8;                       // byte relativo al MB que contiene el bit que representa nbloque
    int posbit = nbloque % 8;                          // posicion del bit que representa nbloque relativo al byte
    int nbloqueMB = posbyteMB / BLOCKSIZE;             // bloque relativo al MB donde esta el posbyte
    int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB; // posicion absoluta donde se encentra el nbloqueMB

    int posbyte = posbyteMB % BLOCKSIZE; // posicion del posbyteMB relativa a un bloque

    // leemos nbloqueabs
    if (bread(nbloqueabs, bufferMB) == FALLO) return FALLO;

    // modificamos el bit
    unsigned char mascara = 128; // 10000000
    mascara >>= posbit;          // desplazamiento a la derecha

    if (bit == 1) {
        // para poner a 1 el bit
        bufferMB[posbyte] |= mascara; // operador OR para bits
    }

    if (bit == 0) {
        // para poner a 1 el bit
        bufferMB[posbyte] &= ~mascara; // operador AND y NOT para bits
    }

    // escribimos el buffer el el disco en la posicion nbloqueabs
    if (bwrite(nbloqueabs, bufferMB) == FALLO) return FALLO;

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
    if (bread(posSB, &SB) == FALLO) return FALLO;

    // posicionamiento igual que en escribir_bit
    int posbyteMB = nbloque / 8;
    int posbit = nbloque % 8;
    int nbloqueMB = posbyteMB / BLOCKSIZE;
    int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;
    int posbyte = posbyteMB % BLOCKSIZE;

    #if DEBUG_leer_bit
        printf(GRAY "[leer_bit(%d) -> posbyteMB:%d, posbyte:%d, posbit:%d, nbloqueMB:%d, nbloqueabs:%d]\n" RESET, nbloque, posbyteMB, posbyte, posbit, nbloqueMB, nbloqueabs);
    #endif

    // leemos nbloqueabs
    if (bread(nbloqueabs, bufferMB) == FALLO) return FALLO;

    unsigned char mascara = 128;  // 10000000
    mascara >>= posbit;           // desplazamos el 1 de la mascara a la derecha las veces que indique el posbit
    mascara &= bufferMB[posbyte]; // and de la mascara con el byte, el resultado se guarda en la mascara

    if (mascara == 0) {
        // si la mascara queda a 0 es que el bit era 0
        return 0;
    }
    else {
        // si es cualquier otra cosa que no sea 0 es que es 1
        return 1;
    }
}

/*
 *  Encuentra el primer bloque libre (consultando MB) y lo ocupa (poniendo el bit a 1)
 *  @return posición del bloque reservado
 */
int reservar_bloque() {
    struct superbloque SB;
    unsigned char bufferMB[BLOCKSIZE];
    unsigned char bufferAux[BLOCKSIZE];

    // leer el SB
    if (bread(posSB, &SB) == FALLO) return FALLO;

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
        if (bread(SB.posPrimerBloqueMB + nbloqueMB, bufferMB) == FALLO) return FALLO;

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
        posbit++;
    }

    // 4. a que bloque fisico corresponde ese bit
    int nbloque = (nbloqueMB * BLOCKSIZE + posbyte) * 8 + posbit;

    // 5. escribimos el bit a 1
    if (escribir_bit(nbloque, 1) == FALLO) return FALLO;

    // actualizmos el SB
    SB.cantBloquesLibres--;
    if (bwrite(posSB, &SB) == FALLO) return FALLO;

    // limpiamos el bloque de datos
    unsigned char buffer[BLOCKSIZE];
    memset(buffer, 0, BLOCKSIZE); // lo llenamos todo de 0
    if (bwrite(nbloque, buffer) == FALLO) return FALLO;

    return nbloque;
}

/*
 *  Ponemos a 0 el bit de MB que representa nbloque mediante escribir_bit
 *  @param nbloque numero de bloque a liberar
 *  @return numero de bloque liberado (nbloque)
 */
int liberar_bloque(unsigned int nbloque) {
    struct superbloque SB;

    // leer el SB
    if (bread(posSB, &SB) == FALLO)  return FALLO;

    // 1. mediante escribir_bit ponemos a 0 el bit de MB que representa nbloque
    if (escribir_bit(nbloque, 0) == FALLO) return FALLO;

    // 2. incrementamos cantidad bloques libres de SB
    SB.cantBloquesLibres++;
    if (bwrite(posSB, &SB) == FALLO) return FALLO;

    return nbloque;
}

/*
 *  Escribe el contenido de inodo en el inodo ninodo
 *  @param ninodo numero de inodo que queremos escribir
 *  @param inodo lo que queremos escribir
 *  @return 0 si todo ha ido bien, -1 en caso contrario
 */
int escribir_inodo(unsigned int ninodo, inodo_t *inodo) {
    struct superbloque SB;
    inodo_t inodos[BLOCKSIZE / INODOSIZE];

    // leer el SB
    if (bread(posSB, &SB) == FALLO) return FALLO;

    // numeor de bloque relativo al  AI que contiene el ninodo
    int nbloqueAI = ninodo / (BLOCKSIZE / INODOSIZE); // dividir por el numero de inodos que caben en un bloque
    // posicion abroluta del bloque
    int nbloqueabs = SB.posPrimerBloqueAI + nbloqueAI;

    // leemos la posicion nbloqueabs del disco y lo guardamos en inodos
    // lo leemos porque para escribir inodos se escriben BLOCKSIZE/INODOSIZE a la vez (los que camben en un bloque)
    // y el contenido del resto hay que preservarlo
    if (bread(nbloqueabs, inodos) == FALLO) return FALLO;

    // numero de inodo [0, 7] que se escribirá
    int posinodo = ninodo % (BLOCKSIZE / INODOSIZE);

    // escribimos el inodo
    inodos[posinodo] = *inodo;

    // escribimos el bloque de inodos en el disco
    if (bwrite(nbloqueabs, inodos) == FALLO) return FALLO;

    return EXITO;
}

/*
 *  Lee un determinado inodo del AI
 *  @param ninodo numero de inodo que queremos leer
 *  @param inodo donde vamos a volcar el contenido
 *  @return 0 si todo ha ido bien, -1 en caso contrario
 */
int leer_inodo(unsigned int ninodo, inodo_t *inodo) {
    struct superbloque SB;
    inodo_t inodos[BLOCKSIZE / INODOSIZE];

    // leer el SB
    if (bread(posSB, &SB) == FALLO) return FALLO;

    // numeor de bloque relativo al  AI que contiene el ninodo
    int nbloqueAI = ninodo / (BLOCKSIZE / INODOSIZE);
    // posicion abroluta del bloque
    int nbloqueabs = SB.posPrimerBloqueAI + nbloqueAI;

    // leemos la posicion nbloqueabs del disco y lo guardamos en inodos
    if (bread(nbloqueabs, inodos) == FALLO) return FALLO;

    // numero de inodo [0, 7] que se leerá
    int posinodo = ninodo % (BLOCKSIZE / INODOSIZE);

    // volcamso el inodo
    *inodo = inodos[posinodo];

    return EXITO;
}

/*
 *  Encuentra el primer inodo libre (dato del SB), lo reserva (con escribir_inodo) y actualiza la lista enlazada
 *  @param tipo
 *  @param permisos
 *  @return numeor de inodo reservado
 */
int reservar_inodo(unsigned char tipo, unsigned char permisos) {
    struct superbloque SB;
    inodo_t inodo; // donde guardaremos el contenido del primer inodo libre

    // leer el SB
    if (bread(posSB, &SB) == FALLO) return FALLO;

    if (SB.cantInodosLibres == 0) return FALLO;

    // actualizamos lista enlazada
    // guardamso el primer inodo libre en una variable auxiliar
    int posInodoReservado = SB.posPrimerInodoLibre;

    // leemos el inodo que vamos a reservar
    if (leer_inodo(posInodoReservado, &inodo) == FALLO) return FALLO;

    // el nuevo primer inodo libre es al que apuntaba antes posPrimerInodoLibre
    SB.posPrimerInodoLibre = inodo.punterosDirectos[0];

    // inicializamos el inodo reservado
    inodo.tipo = tipo;
    inodo.permisos = permisos;
    inodo.nlinks = 1;
    inodo.tamEnBytesLog = 0;
    inodo.atime = time(NULL);
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);
    inodo.btime = time(NULL);
    inodo.numBloquesOcupados = 0;
    // tamaños en bytes de punterosDirectos / tamaño en bytes de un elemento = numero de elementos
    for (int i = 0; i < (sizeof(inodo.punterosDirectos) / sizeof(inodo.punterosDirectos[0])); i++) {
        inodo.punterosDirectos[i] = 0; // 0 porque no apuntan a nada
    }
    for (int i = 0; i < (sizeof(inodo.punterosIndirectos) / sizeof(inodo.punterosIndirectos[0])); i++){
        inodo.punterosIndirectos[i] = 0; // 0 porque no apuntan a nada
    }

    // escribimos el inodo inicializado en la posicion dle que era el primer inodo libre (posInodoReservado)
    if (escribir_inodo(posInodoReservado, &inodo) == FALLO) return FALLO;

    // actualizamos el SB
    SB.cantInodosLibres--;
    if (bwrite(posSB, &SB) == FALLO) return FALLO;

    return posInodoReservado;
}

/*
 *  Generaliza la obtención de los índices de los bloques de punteros
 *  @param nblogico bloque lógico del que queremos obtener el índice
 *  @param nivel_punteros nivel de punteros (0 para directos, 1 para indirectos de primer nivel, 2 para indirectos de segundo nivel)
 *  @return índice dentro del bloque de punteros correspondiente al bloque lógico, o FALLO si el nivel de punteros no es válido
 */
int obtener_indice(unsigned int nblogico, int nivel_punteros) {
    // BLOQUES DIRECTOS (0 a 11)
    if (nblogico < DIRECTOS) {
        return nblogico; // el índice es el bloque lógico
    }

    // INDIRECTOS DE NIVEL 0 (un nivel de bloques de punteros)
    else if (nblogico < INDIRECTOS0) {
        return nblogico - DIRECTOS; // restamos los bloques directos para obtener el índice dentro del bloque de punteros indirectos
    }

    // INDIRECTOS DE NIVEL 1 (dos niveles de bloques de punteros)
    else if (nblogico < INDIRECTOS1) {
        if (nivel_punteros == 2) {
            return (nblogico - INDIRECTOS0) / NPUNTEROS; // índice dentro del bloque de punteros indirectos de nivel 2
        }
        else if (nivel_punteros == 1) {
            return (nblogico - INDIRECTOS0) % NPUNTEROS; // índice dentro del bloque de punteros indirectos de nivel 1
        } 
    }

    // INDIRECTOS DE NIVEL 2 (tres niveles de blqoues de punteros)
    else if (nblogico < INDIRECTOS2){
        if (nivel_punteros == 3){
            return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
        } else if (nivel_punteros == 2) {
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;
        } else if (nivel_punteros == 1) {
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS;
        }
    }

    return FALLO;
}

/*
 *  Obtiene el rango de punteros en el que se sitúa el bloque lógico que buscamos
 *  @param inodo el inodo del que queremos obtener el puntero al bloque lógico
 *  @param nblogico bloque lógico del que queremos obtener el puntero
 *  @param ptr puntero donde se guardará el resultado (puntero directo o indirecto que apunta al bloque lógico)
 *  @return 0 si el bloque lógico es directo, 1 si es indirecto de primer nivel, 2 si es indirecto de segundo nivel, 3 si es indirecto de tercer nivel, FALLO si el bloque lógico es demasiado grande para el sistema de archivos
 */
int obtener_nRangoBL(inodo_t *inodo, unsigned int nblogico, unsigned int *ptr) {
    if (nblogico < DIRECTOS) {
        *ptr = inodo->punterosDirectos[nblogico]; // puntero al bloque logico donde estan los datos 
        return 0;
    }
    // puntero al bloque de punteros donde empezar  abuscar
    else if (nblogico < INDIRECTOS0) {
        *ptr = inodo->punterosIndirectos[0]; 
        return 1;
    }
    else if (nblogico < INDIRECTOS1) {
        *ptr = inodo->punterosIndirectos[1];
        return 2;
    }
    else if (nblogico < INDIRECTOS2) {
        *ptr = inodo->punterosIndirectos[2];
        return 3;
    }
    
    // el bloque lógico es demasiado grande para el sistema de archivos
    return FALLO;
}

/*
 *  Obtiene el numero de bloque fisico correspondiente a un bloque logico determinado del inodo indicado
 *  @param inodo número del inodo a consultar
 *  @param nblogico bloque lógico a traducir
 *  @param reservar si es 1 reserva bloques si no existen, si es 0 solo consulta
 *  @return número de bloque físico (ptr), o FALLO si hay error o si no existe y reservar=0
 */
int traducir_bloque_inodo(unsigned int inodo, unsigned int nblogico, unsigned char reservar) {
    inodo_t inodoAux;
    unsigned int ptr = 0; // puntero donde se guardará el resultado de obtener_nRangoBL
    unsigned int ptr_ant = 0; 
    unsigned int salvar_inodo = 0; // si hay que actualizar o no el inodo en el disco
    int indice = 0; 
    unsigned int buffer[NPUNTEROS];

    // leer el inodo
    if (leer_inodo(inodo, &inodoAux) == FALLO) return FALLO;

    // obtenemos el rango y el puntero inicial
    int nRangoBL = obtener_nRangoBL(&inodoAux, nblogico, &ptr); // 0:D, 1:I0, 2:I1, 3:I2
    if (nRangoBL == FALLO) return FALLO; 

    int nivel_punteros = nRangoBL; // el nivel mas alto es el que cuelga directamente del inodo

    // CASO PUNTEROS DIRECTOS (nRangoBL = 0)
    if (nRangoBL == 0) {
        if (ptr == 0) { // no existe bloque de datos
            if (reservar == 0) return FALLO; // no existe el bloque y no reservamos
            
            ptr = reservar_bloque(); // se reserva el bloque de datos

            if (ptr == FALLO) return FALLO;
        
            # if DEBUG_traducir_bloque_inodo
                printf(GRAY"[traducir_bloque_inodo() -> inodo.punterosDirectos[%d] = %d (reservado BF %d para BL %d)]\n"RESET, nblogico, ptr, ptr, nblogico);
            # endif 

            inodoAux.numBloquesOcupados ++; 
            inodoAux.ctime = time(NULL);
            inodoAux.punterosDirectos[nblogico] = ptr; // guardamos el nuevo bloque en el inodo
            
            salvar_inodo = 1;
        }
        // si el bloque ya existia solo devolvemos el puntero que lo apunta
    } 

    // CASO DE PUNTEROS INDIRECTOS (nRangoBL > 0)
    else { 
        // iterar para cada nivel de punteros indirectos 
        while (nivel_punteros > 0) {
            // si no cuelgan bloques de punteros intermedios hay que crearlos
            if (ptr == 0) { 
                if (reservar == 0) return FALLO; // no existe el bloque y no reservamos
               
                ptr = reservar_bloque(); // se reserva el bloque de datos

                if (ptr == FALLO) return FALLO;

                inodoAux.numBloquesOcupados ++; 
                inodoAux.ctime = time(NULL);

                salvar_inodo = 1;

                // si el bloque cuelga directamente del inodo
                if (nivel_punteros == nRangoBL) {
                    # if DEBUG_traducir_bloque_inodo 
                        printf(GRAY "[traducir_bloque_inodo() -> inodo.punterosIndirectos[%d] = %d (reservado BF %d para punteros_nivel%d)]\n"RESET, nRangoBL-1, ptr, ptr, nivel_punteros);
                    # endif
                    inodoAux.punterosIndirectos[nRangoBL - 1] = ptr; // guardamos el nuevo bloque en el inodo
                }
                // el bloque cuelga de otro bloque de punteros 
                else { 
                    # if DEBUG_traducir_bloque_inodo
                        printf(GRAY"[traducir_bloque_inodo() -> punteros_nivel%d[%d] = %d (reservado BF %d para punteros_nivel%d)]\n"RESET, nivel_punteros+1, indice, ptr, ptr, nivel_punteros);
                    # endif
                    buffer[indice] = ptr;
                    // salvamos el buffer modificado
                    // escribimos la direccion del bloque (ptr) en el padre
                    if (bwrite(ptr_ant, buffer) == FALLO) return FALLO;
                }
                memset(buffer, 0, BLOCKSIZE); // limpiamos el buffer para el nuevo bloque de punteros
            }  
            else { // ptr != 0
                // leemso el bloque de punteros ya existente 
                if (bread(ptr, buffer) == FALLO) return FALLO;
            }

            indice = obtener_indice(nblogico, nivel_punteros); // actualizamos el indice
            ptr_ant = ptr; // guardamso el puntero actual
            ptr = buffer[indice]; // y lo desplazamos al siguiente nivel
            nivel_punteros --; // reducimos le nivel 
        } // al salir del bucle ya estamos al nivel de datos 

        // si al salir del bucle el puntero aun está a 0 
        if (ptr == 0) {
            if (reservar == 0) {
                return FALLO;
            }

            ptr = reservar_bloque();
            inodoAux.numBloquesOcupados ++;
            inodoAux.ctime = time(NULL);
            salvar_inodo = 1;  

            # if DEBUG_traducir_bloque_inodo
                printf(GRAY"[traducir_bloque_inodo() -> punteros_nivel%d[%d] = %d (reservado BF %d para BL %d)]\n"RESET, nivel_punteros+1, indice, ptr, ptr, nblogico);
            # endif
            
            buffer[indice] = ptr;

            if (bwrite(ptr_ant, buffer) == FALLO) return FALLO;
        } 
    }

    if (salvar_inodo) {
        if (escribir_inodo(inodo, &inodoAux) == FALLO) return FALLO;
    }

    return ptr;
}

/*
 * Libera (borra) un inodo (fichero/directorio) completo
 * @param inodo inodo que vamos a liberar 
 * @return numero de inodo liberado
 */
int liberar_inodo(unsigned int ninodo) {
    inodo_t inodo;

    // leer el inodo
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;

    // liberar todos los bloques de datos e índices (desde el bloque lógico 0)
    int liberados = liberar_bloques_inodo(0, &inodo);

    // actualizar los metadatos del inodo
    inodo.numBloquesOcupados -= liberados;
    inodo.tipo = 'l'; // marcar el inodo como libre 
    inodo.tamEnBytesLog = 0;
    
    // actualizar la lista enlazada de inodos libres
    struct superbloque SB; 
    // leemos el SB
    if (bread(posSB, &SB) == FALLO) return FALLO;
    
    inodo.punterosDirectos[0] = SB.posPrimerInodoLibre;
    SB.posPrimerInodoLibre = ninodo;

    SB.cantInodosLibres ++; // bloques libres ya se incrementa en liberar_bloque
    
    // escribir el SB actualizado 
    if (bwrite(posSB, &SB) == FALLO) return FALLO;

    #if DEBUG_liberar_inodo
        fprintf(stderr, MAGENTA "[liberar_inodo()→ Tras liberar inodo: primerInodoLibre=%d]\n" RESET, SB.posPrimerInodoLibre);
    #endif
    
    inodo.ctime = time(NULL);

    // escribir el inodo actualizado 
    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;

    return liberados;
}

/*
 * Libera todos los bloques ocupados a partir del BL indicado (inclusive) - VERSION 2
 * @param primerBL BL a partir del que vamos a liberar
 * @param inodo inodo que vamos a liberar 
 * @return total de bloques liberados
 */
int liberar_bloques_inodo(unsigned int primerBL, inodo_t *inodo) {
    unsigned int nivel_punteros = 0, nBL = primerBL, ultimoBL, ptr = 0;
    int liberados = 0, eof = 0;

    // si el fichero esta vacio no hay nada que liberar
    if (inodo->tamEnBytesLog == 0) {
        return 0;
    }

    // calculamos cual será el ultimo BL a liberar
    if ((inodo->tamEnBytesLog % BLOCKSIZE) == 0) {
        ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE -1;
    } else {
        ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE;
    }

    #if DEBUG_liberar_inodo
        fprintf(stderr, GRAY "[liberar_bloques_inodo()→ primer BL: %d, último BL: %d]\n" RESET, primerBL, ultimoBL);
    #endif

    // obtenemos el rango del BL
    int nRangoBL = obtener_nRangoBL(inodo, nBL, &ptr);
    // si el rango es 0 es que el primer bloque a liberar es 0
    if (nRangoBL == 0) {
        liberados += liberar_directos(&nBL, ultimoBL, inodo, &eof); 
    }
    
    // liberamos los bloques indirectos 
    while (!eof) {
        // actualizamos el rango 
        nRangoBL = obtener_nRangoBL(inodo, nBL, &ptr);
        nivel_punteros = nRangoBL;
        // llamada recursiva para cada rama de indirectos 
        liberados += liberar_indirectos_recursivo(&nBL, primerBL, ultimoBL, inodo, nRangoBL, nivel_punteros, &ptr, &eof); 
    }

    #if DEBUG_liberar_inodo
        fprintf(stderr, RED"[liberar_bloques_inodo()→ total bloques liberados:%d , total_breads: , total_bwrites: ]\n" RESET, liberados);
    #endif
    
    return liberados;
}

/*
 * Recorre y libera los bloques directos 
 * @param nBL numero de bloque logico a liberar 
 * @param ultimoBL ultimo bloque logico a liberar 
 * @param inodo inodo que vamos a liberar
 * @param eof 1 si es el final del fichero, 0 si no
 * @return total de bloques liberados
 */
int liberar_directos (unsigned int *nBL, unsigned int ultimoBL, inodo_t *inodo, int *eof) {
    int liberados = 0;

    // mientras estemos en rango de punteros directos y no sea el final del fichero
    while (*nBL < DIRECTOS && !*eof) {
        if (inodo->punterosDirectos[*nBL] != 0) { // si el BL directo no es 0 (no está vacio)
            #if DEBUG_liberar_inodo
                fprintf(stderr, GRAY "[liberar_bloques_inodo()→ liberado BF %d de datos para BL %d]\n" RESET, inodo->punterosDirectos[*nBL], *nBL);
            #endif
            liberar_bloque(inodo->punterosDirectos[*nBL]); // lo liberamos 
            inodo->punterosDirectos[*nBL] = 0; // ponemos el contenido a 0 (vacio)
            liberados ++;
        }

        *nBL = *nBL + 1; // pasamso al siguiente bloque logico

        // si ya hemos liberado todos los bloques marcamos final del fichero
        if (*nBL > ultimoBL) {
            *eof = 1;
        }
    }
 
    return liberados;
}

/*
 * Recorre y libera los bloques indirectos de manera recursiva, aplicando optimizacion de saltos  
 * @param nBL numero de bloque logico a liberar 
 * @param primerBL primer bloque logico a liberar 
 * @param ultimoBL ultimo bloque logico a liberar 
 * @param inodo inodo que vamos a liberar
 * @param nRangoBL ranog del bloque logido (0:D, 1:I0, 2:I1, 3:I2)
 * @param nivel_punteros nivel de profundidad 
 * @param ptr puntero al bloque de índices
 * @param eof 1 si es el final del fichero, 0 si no
 * @return total de bloques liberados
 */
int liberar_indirectos_recursivo (unsigned int *nBL, unsigned int primerBL, unsigned int ultimoBL, inodo_t *inodo, int nRangoBL, unsigned int nivel_punteros, unsigned int *ptr, int *eof) {
    int liberados = 0, modificado = 0;
    unsigned int bloquePunteros[NPUNTEROS], bufferCeros[NPUNTEROS];
    
    // buffer de ceros para comparar
    memset(bufferCeros, 0, BLOCKSIZE); 

    // si el puntero al bloque de indices es 0 saltamos todos los BL que ese puntero cubre  
    if (*ptr == 0) {
        fprintf(stderr, YELLOW "HOLA HOLA\n" RESET); // NUNCA LLEGA A ENTRAR EN EL IF
        switch (nRangoBL) {
            case 1: 
                #if DEBUG_liberar_inodo
                    fprintf(stderr, GRAY "[liberar_bloques_inodo()→ Saltamos del BL %d al BL %ld]\n" RESET, *nBL, INDIRECTOS0);
                #endif
                *nBL = INDIRECTOS0; 
                break;
            case 2: 
                #if DEBUG_liberar_inodo
                    fprintf(stderr, GRAY "[liberar_bloques_inodo()→ Saltamos del BL %d al BL %ld]\n" RESET, *nBL, INDIRECTOS1);
                #endif
                *nBL = INDIRECTOS1; 
                break;
            case 3: 
                #if DEBUG_liberar_inodo
                    fprintf(stderr, GRAY "[liberar_bloques_inodo()→ Saltamos del BL %d al BL %ld]\n" RESET, *nBL, INDIRECTOS2);
                #endif
                *nBL = INDIRECTOS2; 
                break;
        }

        return liberados;
    }
     
    int indice_inicial = obtener_indice(*nBL, nivel_punteros);

    // solo leemos el bloque si no estaba cargado ya previamente
    if (indice_inicial == 0 || *nBL == primerBL) { 
        // leemos el blqoue de punteros 
        if (bread(*ptr, bloquePunteros) == FALLO) return FALLO;
    }
    
   // Recorremos las entradas del bloque de punteros desde indice_inicial.
    for (int i = indice_inicial; i < NPUNTEROS && !*eof; i++) {
        if (bloquePunteros[i] != 0) {
            // caso base: puntero que apunta al bloque de datos 
            if (nivel_punteros == 1) {
                #if DEBUG_liberar_inodo
                    fprintf(stderr, GRAY "[liberar_bloques_inodo()→ liberado BF %d de datos para BL %d]\n" RESET, bloquePunteros[i], *nBL);
                #endif
                liberar_bloque(bloquePunteros[i]); // de datos
                bloquePunteros[i] = 0;
                modificado = 1;
                liberados ++;
                *nBL = *nBL +1; // avanzamos al siguiente BL
            } 
            // caos recursivo: estamso en el nivel 2 o 3, hay que bajar de nivel
            else {
                unsigned int ptr_antes = bloquePunteros[i];
                liberados += liberar_indirectos_recursivo(nBL, primerBL, ultimoBL, inodo, nRangoBL, nivel_punteros - 1, &bloquePunteros[i], eof);

                if (bloquePunteros[i] != ptr_antes) modificado = 1; // si despues de llamar a la funcion el puntero es diferente es que se ha modificado               
            }
        } else {
            // salto si encontramos un puntero a 0 dentro del bloque 
            switch (nivel_punteros) {
                case 1: 
                    
                    *nBL = *nBL + 1; 
                    
                    break;
                case 2: 
                    #if DEBUG_liberar_inodo
                        fprintf(stderr, BLUE "[liberar_bloques_inodo()→ Saltamos del BL %d " RESET, *nBL);
                    #endif
                    *nBL += NPUNTEROS; 
                    #if DEBUG_liberar_inodo
                        fprintf(stderr, BLUE "al BL %d]\n" RESET, *nBL);
                    #endif
                    break;
                case 3: 
                    #if DEBUG_liberar_inodo
                        fprintf(stderr, YELLOW "[liberar_bloques_inodo()→ Saltamos del BL %d " RESET, *nBL);
                    #endif
                    *nBL += NPUNTEROS * NPUNTEROS; 
                    #if DEBUG_liberar_inodo
                        fprintf(stderr, YELLOW "al BL %d]\n" RESET, *nBL);
                    #endif
                    break;
            }
        }

        // si ya hemos liberado todos los bloques marcamos final del fichero
        if (*nBL > ultimoBL) *eof = 1;
    }

    // si el bloque de punteros ha quedado vacio
    if (memcmp(bloquePunteros, bufferCeros, BLOCKSIZE) == 0) {
        #if DEBUG_liberar_inodo
            fprintf(stderr, ORANGE "[liberar_bloques_inodo()→ liberado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, *ptr, nivel_punteros, *nBL); // ESTE ULTIMO VALOR MAL 
        #endif
        liberar_bloque(*ptr); // liberamos el bloque de punteros 
        *ptr = 0; // ponemos a 0 el puntero del nivel superior
        liberados++;
    } else if (modificado == 1) {
        // si el bloque no esta vacion pero ha habido modificaciones lo slavamso
        if (bwrite(*ptr, bloquePunteros) == FALLO) return FALLO; 
    }

    return liberados;
}