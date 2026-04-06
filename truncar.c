/**
 * @file truncar.c
 * @author Paula Cristina Morro Negre 
 * @author Maria Teresa Sbert Gomila
*/

#include "ficheros.h"

int main(int argc, char const *argv[]) {
    // Validamos el número de argumentos introducidos
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: truncar <nombre_dispositivo> <ninodo> <nbytes>\n" RESET);
        return FALLO;
    }
    
    const char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);
    unsigned int nbytes = atoi(argv[3]);

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;  
    
    // si nbytes = 0 liberamos el inodo entero 
    if (nbytes == 0) {
        liberar_inodo(ninodo);  
    // Si nbytes > 0, truncamos el fichero
    } else {
        mi_truncar_f(ninodo, nbytes);
    }

    printf("\n");

    // imprimir el inodo
    struct STAT stat;
    mi_stat_f(ninodo, &stat);

    char atime[80];
    char mtime[80];
    char ctime[80];
    char btime[80];

    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", localtime(&stat.atime));
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", localtime(&stat.mtime));
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", localtime(&stat.ctime));
    strftime(btime,sizeof(btime), "%a %Y-%m-%d %H:%M:%S", localtime(&stat.btime));

    printf("DATOS INODO %d\n", ninodo);
    printf("tipo = %c\n", stat.tipo);
    printf("permisos = %d\n", stat.permisos);
    printf("atime: %s\n", atime);
    printf("mtime: %s\n", mtime);
    printf("ctime: %s\n", ctime);
    printf("btime: %s\n", btime);
    printf("nlinks = %d\n", stat.nlinks);
    printf("tamEnBytesLog = %d\n", stat.tamEnBytesLog);
    printf("numBloquesOcupados = %d\n", stat.numBloquesOcupados);

    // desmontar dispositivo
    if (bumount() == FALLO) return FALLO;
    
    return EXITO;
}
