/**
 * @file mi_stat.c
 * @author Paula Cristina Morro Negre
 * @author Maria Teresa Sbert Gomila
 *
 * Sintaxis: ./mi_stat <disco> </ruta>
 * Muestra los metadatos del inodo de un fichero o directorio.
 */

#include "directorios.h"

int main(int argc, char *argv[]) {
    // comprobamos los argumentos
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./mi_stat <nombre_dispositivo> </ruta>\n" RESET);
        return FALLO;
    }

    const char *disco = argv[1];
    const char *ruta  = argv[2];

    // montamos el dispositivo
    if (bmount(disco) == FALLO) return FALLO;
 
    // obtenemos los metadatos del inodo
    struct STAT stat;
    int error = mi_stat(ruta, &stat);
    if (error != EXITO) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }

    // buscamos el número de inodo para mostrarlo
    struct superbloque SB;
    bread(posSB, &SB);
    unsigned int p_inodo_dir = SB.posInodoRaiz, p_inodo = 0, p_entrada = 0;
    buscar_entrada(ruta, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);

    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];
    char btime[80];

    ts = localtime(&stat.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&stat.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&stat.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&stat.btime);
    strftime(btime,sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);

    printf(CYAN "Nº de inodo: %u\n" RESET, p_inodo);
    printf("tipo: %c\n", stat.tipo);
    printf("permisos: %u\n", stat.permisos);
    printf("atime: %s\n", atime);
    printf("mtime: %s\n", mtime);
    printf("ctime: %s\n", ctime);
    printf("btime: %s\n", btime);
    printf("nlinks: %u\n", stat.nlinks);
    printf("tamEnBytesLog: %u\n", stat.tamEnBytesLog);
    printf("numBloquesOcupados: %u\n", stat.numBloquesOcupados);

    if (bmount(disco) == FALLO) return FALLO;
    
    return EXITO;
}