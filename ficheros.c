#include "ficheros.h"

/*
 *  Escribe el contenido del buffer en un fichero/directorio 
 *  @param ninodo inodo al que corresponde el fichero/directorio
 *  @param buf_original buffer donde está el contenido a escribir
 *  @param offset posición inicial de escritura en bytes logicos 
 *  @param nbytes tamaño en bytes del buffer
 *  @return cantidad de bytes escritos realmente
 */
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    inodo_t inodo;
    unsigned char buf_bloque[BLOCKSIZE];
    int bytes_escritos = 0;

    // leer el inodo
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        perror(RED "Error al leer el inodo en mi_write_f\n"); printf(RESET);
        return FALLO;
    }

    /* ESTO NO SE SI HAY QUE HACERLO 
    // comprobamos que el offset no es mayor que el tamaño lógico del inodo
    if (offset > inodo.tamEnBytesLog) {
        fprintf(stderr, RED "Error: offset mayor que el tamaño lógico del inodo en mi_write_f\n" RESET);
        return FALLO;
    }
    */
    
    // comprobamos que tenga los permisos necesarios
    if ((inodo.permisos & 2) != PERMISO_ESCRITURA) { // el segundo bit tiene que estar a 1 (010, 011, 110, 111)
        perror(RED "No hay permisos de escritura en mi_write_f\n"); printf(RESET);
        return FALLO;
    }

    int primerBL = offset/BLOCKSIZE; // bloque logico en el que empezaremos a escribir
    int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE; // ultimo bloque logico a escribir 

    int desp1 = offset % BLOCKSIZE; // byte relativo a primerBL donde se empieza a escribir (incluido)
    int desp2 = (offset + nbytes -1) % BLOCKSIZE; // byte relativo a primerBL donde se termina de escribir (incluido)
    
    // obtenemos el bloque fisico correspondiente al bloque logico primerBL
    int nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
    if (nbfisico == FALLO) {
        perror(RED "Error al traducir el bloque en mi_write_f\n"); printf(RESET);
        return FALLO;
    } 

    // CASO 1: ESCRIBIMOS EN UN SOLO BLOQUE 
    if (primerBL == ultimoBL) {    
        // leemos ese bloque fisico 
        // porque hay que preservar la parte que no vamos a escribir
        if (bread(nbfisico, buf_bloque) == FALLO) {
            perror(RED "Error al leer el bloque en mi_write_f\n"); printf(RESET);
            return FALLO;
        }

        memcpy(buf_bloque+desp1, buf_original, nbytes); // (donde, desde donde, tamaño)

        // escribimos buf_bloque modificado en el bloque fisico 
        if (bwrite(nbfisico, buf_bloque) == FALLO) {
            perror(RED "Error al escribir el bloque en mi_write_f\n"); printf(RESET);
            return FALLO;
        }
    }

    // CASO 2: ESCRIBIMOS EN MAS DE UN BLOQUE
    if (primerBL != ultimoBL) {
        // PRIMER BLOQUE 
        if (bread(nbfisico, buf_bloque) == FALLO) {
            perror(RED "Error al leer el bloque en mi_write_f\n"); printf(RESET);
            return FALLO;
        }

        memcpy(buf_bloque + desp1, buf_original, BLOCKSIZE - desp1);

        if (bwrite(nbfisico, buf_bloque) == FALLO) {
            perror(RED "Error al escribir el bloque en mi_write_f\n"); printf(RESET);
            return FALLO;
        }

        bytes_escritos += (BLOCKSIZE - desp1);

        // BLOQUES INTERMEDIOS 
        for (int bl = primerBL + 1; bl < ultimoBL; bl ++) {
            // traducimos el blque 
            nbfisico = traducir_bloque_inodo(ninodo, bl, 1);
            if (nbfisico == FALLO) {
                perror(RED "Error al traducir el bloque en mi_write_f\n"); printf(RESET);
                return FALLO;
            } 

            // sobreescibimos todo el bloque 
            bwrite(nbfisico, buf_original + bytes_escritos);
            bytes_escritos += BLOCKSIZE;
        }

        // ULTIMO BLOQUE
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 1);

        if (bread(nbfisico, buf_bloque) == FALLO) {
            perror(RED "Error al leer el bloque en mi_write_f\n"); printf(RESET);
            return FALLO;
        }  
        
        memcpy(buf_bloque, buf_original + bytes_escritos, BLOCKSIZE - desp2);

        if (bwrite(nbfisico, buf_bloque) == FALLO) {
            perror(RED "Error al escribir el bloque en mi_write_f\n"); printf(RESET);
            return FALLO;
        }

        bytes_escritos += (desp2 + 1);
    } 

    // actualizamos el inodo 
    if (offset + nbytes > inodo.tamEnBytesLog) { // solo actualizamos si hemos escrito mas alla del final del fichero 
        inodo.tamEnBytesLog = offset + nbytes;
        inodo.ctime = time(NULL); // porque modificamos el inodo
    }
    inodo.mtime = time(NULL); // porque modificamos los datos siempre 

    if (escribir_inodo(ninodo, &inodo) == FALLO){
        perror(RED "Error al escribir el inodo en mi_write_f\n"); printf(RESET);
        return FALLO;
    }
    
    return bytes_escritos;
}

/*
 *  Lee información de un fichero/directorio 
 *  @param ninodo inodo al que corresponde el fichero/directorio
 *  @param buf_original buffer donde guardaremos lso datos leidos
 *  @param offset posición inicial de de lectura en bytes logicos 
 *  @param nbytes tamaño en bytes a leer
 *  @return cantidad de bytes leidos realmente
 */
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    inodo_t inodo;
    unsigned char buf_bloque[BLOCKSIZE];
    int bytes_leidos = 0;

    // leer el inodo
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        perror(RED "Error al leer el inodo en mi_write_f\n"); printf(RESET);
        return FALLO;
    }
    
    // comprobamos que tenga los permisos necesarios
    if ((inodo.permisos & 4) != PERMISO_LECTURA) { // el tercer bit tiene que estar a 1 (100, 101, 110, 111)
        perror(RED "No hay permisos de lectura en mi_write_f\n"); printf(RESET);
        return FALLO;
    }

    if (offset >= inodo.tamEnBytesLog) {
        perror(RED "No se puede empezar a leer mas alla del tamaño en bytes logicos del inodo en mi_read_f\n"); printf(RESET);
        return bytes_leidos;
    }

    if (offset + nbytes >= inodo.tamEnBytesLog) {
        nbytes = inodo.tamEnBytesLog - offset; // leemos hasta donde podamos desde offstes hasta EOF
    }
    
    int primerBL = offset / BLOCKSIZE;
    int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    int desp1 = offset % BLOCKSIZE;
    int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    // CASO 1: ESCRIBIMOS EN UN SOLO BLOQUE 
    if (primerBL == ultimoBL) {    
        int nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);
        
        if (nbfisico != FALLO) {
            if (bread(nbfisico, buf_bloque) == FALLO) {
                perror(RED "Error al leer el bloque en mi_read_f\n"); printf(RESET);
                return FALLO;
            } 
            memcpy(buf_original, buf_bloque + desp1, nbytes);
        }

        bytes_leidos = nbytes;
    }

    // CASO 2: ESCRIBIMOS EN MAS DE UN BLOQUE
    if (primerBL != ultimoBL) {
        // PRIMER BLOQUE 
        int nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);

        if (nbfisico != FALLO) {
            if (bread(nbfisico, buf_bloque) == FALLO) {
                perror(RED "Error al leer el bloque en mi_read_f\n"); printf(RESET);
                return FALLO;
            } 
            memcpy(buf_original, buf_bloque + desp1, BLOCKSIZE - desp1);
        }

        bytes_leidos += BLOCKSIZE - desp1;
        
        // BLOQUES INTERMEDIOS 
        for (int bl = primerBL + 1; bl < ultimoBL; bl ++) {
            // traducimos el blque 
            nbfisico = traducir_bloque_inodo(ninodo, bl, 1);
            if (nbfisico != FALLO) {
                if (bread(nbfisico, buf_bloque) == FALLO) {
                    perror(RED "Error al leer el bloque en mi_read_f\n"); printf(RESET);
                    return FALLO;
                } 
                memcpy(buf_original + bytes_leidos, buf_bloque, BLOCKSIZE);
            } 

            bytes_leidos += BLOCKSIZE;
        }

        // ULTIMO BLOQUE
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 0);

        if (nbfisico != FALLO) {
            if (bread(nbfisico, buf_bloque) == FALLO) {
                perror(RED "Error al leer el bloque en mi_read_f\n"); printf(RESET);
                return FALLO;
            } 
            memcpy(buf_original + bytes_leidos, buf_bloque, desp2 + 1);
        }
        bytes_leidos += (desp2 + 1);
    } 

    // actualizamos el inodo 
    inodo.atime = time(NULL);

    if (escribir_inodo(ninodo, &inodo) == FALLO){
        perror(RED "Error al escribir el inodo en mi_read_f\n"); printf(RESET);
        return FALLO;
    }
    
    return bytes_leidos;
}


//int mi_stat_f(unsigned int ninodo, struct STAT *p_stat);
int mi_chmod_f(unsigned int ninodo, unsigned char permisos);