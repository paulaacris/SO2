#include "ficheros.h"


/*
 *  Escribe el contenido procedente de un buffer de memoria de tamaño nbytes en un fichero/directorio
 *  @param ninodo numero de inodo del fichero/directorio donde queremos escribir
 *  @param buf_original buffer de memoria desde donde se van a escribir los datos
 *  @param offset offset dentro del fichero/directorio a partir del cual se van a escribir los datos
 *  @param nbytes cantidad de bytes que se van a escribir
 *  @return cantidad de bytes escritos, o -1 en caso de error (FALLO)
 */
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    inodo_t inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        perror(RED "Error al leer el inodo en mi_write_f\n"); printf(RESET);
        return FALLO;
    }

    // comprobamos que el offset no es mayor que el tamaño lógico del inodo
    if (offset > inodo.tamEnBytesLog) {
        fprintf(stderr, RED "Error: offset mayor que el tamaño lógico del inodo en mi_write_f\n" RESET);
        return FALLO;
    }

    // Comprobamos los permisos de escritura 
    if ((inodo.permisos & 2) != 2) {
        perror(RED "Error: el inodo no tiene permisos de escritura\n" RESET);
        return FALLO;

    }

    // Calculamos cual va a ser el primer bloque lógico
    unsigned int primerBL = offset / BLOCKSIZE;

    // Calculamos cuál va a ser el último bloque lógico
    unsigned int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;

    // Calculamos el desplazamiento desp1 en el bloque para el offset
    int desp1 = offset % BLOCKSIZE;

    // Calculamos el despleazamiento desp2 en el bloque para ver donde llegan los nbytes escritos
    int desp2 = (offset + nbytes - 1) % BLOCKSIZE;


}
