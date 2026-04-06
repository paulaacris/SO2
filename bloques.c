/**
 * @file bloques.c
 * @author Paula Cristina Morro Negre 
 * @author Maria Teresa Sbert Gomila
*/

#include "bloques.h"

static int descriptor = 0;

/*
 *  "monta" el dispositivo virtual 
 *  @param camino direccion del dispositivo virtual
 *  @return descriptor del fichero o -1 en caso de error
 */
int bmount(const char *camino){
    // creamos/abrimos el fichero con los permisos O_RDWR | O_CREAT
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);

    if (descriptor == FALLO) {
        fprintf(stderr, RED"Error al abrir el fichero en bmount\n"RESET);
    }
    
    return descriptor;
}

/*
 *  "cierra" el dispositivo virtual 
 *  @return descriptor del fichero o -1 en caso de error
 */
int bumount() {
    // cerramos el fichero 
    descriptor = close(descriptor);

    if (descriptor == FALLO) {
        fprintf(stderr, RED"Error al cerrar el fichero en bumount\n"RESET);
        return FALLO; 
    }

    return descriptor; 
}

/*
 *  escribe un bloque en el dispositivo virtual (bloque físico especificado por nbloque)
 *  @param nbloque donde lo quieres escribir
 *  @param buf lo que quieres escribir
 *  @return numero de bytes que ha podido escribir (BLOCKSIZE) o -1 si ha habido error
 */
int bwrite(unsigned int nbloque, const void *buf) {
    // movemos el puntero al offset correcto 
    if (lseek(descriptor, nbloque*BLOCKSIZE, SEEK_SET) == FALLO) return FALLO; // descriptor, dondenos queremos move, donde empezamos (incio dle fichero)
    
    // escribimos el bloque
    if (write(descriptor, buf, BLOCKSIZE) == FALLO) return FALLO;      

    return BLOCKSIZE;
}

/*
 *  lee el bloque físico especificado en nbloque
 *  @param nbloque bloque físico que queremos leer
 *  @param buf donde se vuelva el contenido 
 *  @return numero de bytes que ha podido escribir (BLOCKSIZE) o -1 si ha habido error
 */
int bread(unsigned int nbloque, void *buf) {
    // movemos el puntero al offset correcto
    if (lseek(descriptor, nbloque*BLOCKSIZE, SEEK_SET) == FALLO) return FALLO;
    
    // leemos el bloque y lo volcamos en el buffer
    if (read(descriptor, buf, BLOCKSIZE) == FALLO) return FALLO;   

    return BLOCKSIZE;
}