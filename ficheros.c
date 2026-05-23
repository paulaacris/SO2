/**
 * @file ficheros.c
 * @author Paula Cristina Morro Negre 
 * @author Maria Teresa Sbert Gomila
*/

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
    int nbfisico, bytes_escritos = 0;

    // leer el inodo
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;
    
    
    // comprobamos que tenga los permisos necesarios
    // el segundo bit tiene que estar a 1 (010, 011, 110, 111)
    if ((inodo.permisos & PERMISO_ESCRITURA) != PERMISO_ESCRITURA) return FALLO;
    

    int primerBL = offset/BLOCKSIZE; // bloque logico en el que empezaremos a escribir
    int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE; // ultimo bloque logico a escribir 
    int desp1 = offset % BLOCKSIZE; // byte relativo a primerBL donde se empieza a escribir (incluido)
    int desp2 = (offset + nbytes -1) % BLOCKSIZE; // byte relativo a primerBL donde se termina de escribir (incluido)

    // CASO 1: ESCRIBIMOS EN UN SOLO BLOQUE 
    if (primerBL == ultimoBL) {   
        mi_waitSem();
        // obtenemos el bloque fisico correspondiente al bloque logico primerBL
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
        mi_signalSem();
        
        if (nbfisico == FALLO) return FALLO;
        
        // leemos ese bloque fisico 
        // porque hay que preservar la parte que no vamos a escribir
        if (bread(nbfisico, buf_bloque) == FALLO) return bytes_escritos;
        
        memcpy(buf_bloque + desp1, buf_original, nbytes); // (donde, desde donde, tamaño)

        // escribimos buf_bloque modificado en el bloque fisico 
        if (bwrite(nbfisico, buf_bloque) == FALLO) return bytes_escritos;

        bytes_escritos = nbytes;
    }

    // CASO 2: ESCRIBIMOS EN MAS DE UN BLOQUE
    else {
        mi_waitSem();
        // obtenemos el bloque fisico correspondiente al bloque logico primerBL
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
        mi_signalSem();
        
        if (nbfisico == FALLO) return FALLO;
        
        // PRIMER BLOQUE 
        if (bread(nbfisico, buf_bloque) == FALLO) return bytes_escritos;
        
        memcpy(buf_bloque + desp1, buf_original, BLOCKSIZE - desp1);

        if (bwrite(nbfisico, buf_bloque) == FALLO) return bytes_escritos;
        
        bytes_escritos += (BLOCKSIZE - desp1);

        // BLOQUES INTERMEDIOS 
        for (int bl = primerBL + 1; bl < ultimoBL; bl ++) {
            mi_waitSem();
            // traducimos el blque 
            nbfisico = traducir_bloque_inodo(ninodo, bl, 1);
            mi_signalSem();

            if (nbfisico == FALLO) return FALLO;
            
            // sobreescibimos todo el bloque 
            if (bwrite(nbfisico, buf_original + bytes_escritos) == FALLO) return bytes_escritos;
            
            bytes_escritos += BLOCKSIZE;
        }

        mi_waitSem();
        // ULTIMO BLOQUE
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 1);
        mi_signalSem();

        if (bread(nbfisico, buf_bloque) == FALLO) return bytes_escritos;
       
        memcpy(buf_bloque, buf_original + bytes_escritos, desp2 + 1);

        if (bwrite(nbfisico, buf_bloque) == FALLO) return bytes_escritos;

        bytes_escritos += (desp2 + 1);
    } 

    mi_waitSem();
    // leemos porque en traducir se modifica el inodo, si no no lo tenemos actualizado 
    leer_inodo(ninodo, &inodo);
    // actualizamos el inodo 
    if (offset + nbytes > inodo.tamEnBytesLog) { // solo actualizamos si hemos escrito mas alla del final del fichero 
        inodo.tamEnBytesLog = offset + nbytes;
        inodo.ctime = time(NULL); // porque modificamos el inodo
    }
    inodo.mtime = time(NULL); // porque modificamos los datos siempre 

    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;

    mi_signalSem();
    
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
    int nbfisico = 0, bytes_leidos = 0;

    mi_waitSem();

    // leer el inodo
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }
    
    // comprobamos que tenga los permisos necesarios
    // el tercer bit tiene que estar a 1 (100, 101, 110, 111)
    if ((inodo.permisos & PERMISO_LECTURA) != PERMISO_LECTURA) {
        mi_signalSem();
        return FALLO;
    }

    // actualizamos el inodo 
    inodo.atime = time(NULL);

    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    mi_signalSem();

    if (offset >= inodo.tamEnBytesLog) {
        return bytes_leidos;
    }

    if (offset + nbytes >= inodo.tamEnBytesLog) {
        nbytes = inodo.tamEnBytesLog - offset; // leemos hasta donde podamos desde offstes hasta EOF
    }
    
    int primerBL = offset / BLOCKSIZE;
    int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    int desp1 = offset % BLOCKSIZE;
    int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    // CASO 1: LLEMOS DE UN SOLO BLOQUE 
    if (primerBL == ultimoBL) {
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);
        
        if (nbfisico != FALLO) {
            if (bread(nbfisico, buf_bloque) == FALLO) return bytes_leidos;
            memcpy(buf_original, buf_bloque + desp1, nbytes);
        }

        bytes_leidos = nbytes;
    }

    // CASO 2: LEEMOS DE MAS DE UN BLOQUE
    if (primerBL != ultimoBL) {
        // PRIMER BLOQUE 
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);

        if (nbfisico != FALLO) {
            if (bread(nbfisico, buf_bloque) == FALLO) return bytes_leidos;
            memcpy(buf_original, buf_bloque + desp1, BLOCKSIZE - desp1);
        }

        bytes_leidos += BLOCKSIZE - desp1;
        
        // BLOQUES INTERMEDIOS 
        for (int bl = primerBL + 1; bl < ultimoBL; bl++) {
            nbfisico = traducir_bloque_inodo(ninodo, bl, 0); 
            if (nbfisico != FALLO) {
                if (bread(nbfisico, buf_bloque) == FALLO) return bytes_leidos;
                memcpy(buf_original + bytes_leidos, buf_bloque, BLOCKSIZE);
            } 
            bytes_leidos += BLOCKSIZE;
        } 

        // ULTIMO BLOQUE
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 0);

        if (nbfisico != FALLO) {
            if (bread(nbfisico, buf_bloque) == FALLO) return FALLO;
            memcpy(buf_original + bytes_leidos, buf_bloque, desp2 + 1);
        }
        bytes_leidos += (desp2 + 1);
    } 
    
    return bytes_leidos;
}


/*
 *  
 *  @param ninodo inodo al que corresponde el fichero/directorio
 *  @param ninodo inodo al que corresponde el fichero/directorio
 *  @param p_stat puntero al stat donde guardaremos la metainformacion
 *  @return EXITO si todo ha ido bien, FALLO si ha habido algun error
 */
int mi_stat_f(unsigned int ninodo, struct STAT *p_stat) {
    inodo_t inodo;

    // leer el inodo
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;

    // devolver la metainformacion
    p_stat->tipo = inodo.tipo;
    p_stat->permisos = inodo.permisos;

    p_stat->atime = inodo.atime;
    p_stat->mtime = inodo.mtime;
    p_stat->ctime = inodo.ctime;
    p_stat->btime = inodo.btime;

    p_stat->nlinks = inodo.nlinks;
    p_stat->tamEnBytesLog = inodo.tamEnBytesLog;
    p_stat->numBloquesOcupados = inodo.numBloquesOcupados;

    return EXITO;
}

/*
 *  Cambia los permisos de un fichero/directorio 
 *  @param ninodo inodo al que corresponde el fichero/directorio
 *  @param permisos permiso por el que queremos sustituir el anterior
 *  @return EXITO si todo ha ido bien o FALLO si ha habido algun error 
 */
int mi_chmod_f(unsigned int ninodo, unsigned char permisos){
    mi_waitSem();
    
    inodo_t inodo;

    // leer el inodo
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;

    inodo.permisos = permisos; // cambiamos el permiso
    inodo.ctime = time(NULL); // actualizamos la fecha de la ultima modificacion

    // guardamos los cmabios 
    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;

    mi_signalSem();

    return EXITO;
}

/*
 *  Trunca un fichero/directorio a los bytes indicados, liberando los bloques necesarios
 *  @param ninodo número del inodo a truncar
 *  @param nbytes nuevo tamaño en bytes al que queremos dejar el fichero
 *  @return cantidad de bloques liberados 
 */
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes) {
    inodo_t inodo;
    int primerBL;

    // leer el inodo
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;
    
    // comprobamos que el inodo tenga permisos de escritura
    if ((inodo.permisos & PERMISO_ESCRITURA) != PERMISO_ESCRITURA) return FALLO;

    // calcular el primer BL a liberar
    if ((nbytes % BLOCKSIZE) == 0) { 
        primerBL = nbytes/BLOCKSIZE;
    } else { 
        primerBL = nbytes / BLOCKSIZE + 1;
    }

    // liberar los bloques sobrantes a partir del primer BL
    int liberados = liberar_bloques_inodo(primerBL, &inodo);

    // actualizar metadatos del inodo
    inodo.tamEnBytesLog = nbytes;
    inodo.numBloquesOcupados -= liberados;
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);

    // salvamos los cambios del inodo
    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;
    

    return liberados;
}
