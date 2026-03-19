#include "ficheros.h"

int main (int argc, char *argv[]) {
    // Validamos el número de argumentos introducidos
    if (argc != 4) {
        perror(RED "Error: número de argumentos no válido. Sintaxis: escribir <nombre_dispositivo> <\"texto\"> <offset>\n" RESET); 
        return FALLO;
    }

    // Guardamos el primer argumento (nombre del dispositivo)
    char *nombre_dispositivo = argv[1];
    // Guardamos el segundo argumento (texto a escribir)
    char *texto = argv[2];
    // Guardamos el tercer argumento (offset)
    int offset = atoi(argv[3]);
    // Calculamos el número de bytes a escribir
    int nbytes = strlen(texto);

    // Montamos el dispositivo
    if (bmount(nombre_dispositivo) == FALLO) {
        perror(RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    // Reservamos un inodo para el nuevo fichero
    int ninodo = reservar_inodo('f', 6);
    if (ninodo == FALLO) {
        perror(RED "Error al reservar el inodo\n" RESET);
        bumount();
        return FALLO;
    }

    printf("Nº inodo reservado: %d\n", ninodo);
    printf("offset: %d\n", offset);

    // Escribimos los datos en el inodo reservado
    int escritos = mi_write_f(ninodo, texto, offset, nbytes);
    if (escritos == FALLO) {
        perror(RED "Error al escribir en el inodo\n" RESET);
    } else {
        printf("Bytes escritos: %d\n", escritos);
    }

    // Mostramos tam en bytes lógicos del inodo y el números de bloques ocupados
    struct STAT stat;
    if (mi_stat_f(ninodo, &stat) == FALLO) {
        perror(RED "Error al obtener la información del inodo\n" RESET);
    } else {
        printf("stat.tamEnBytesLog = %u\n", stat.tamEnBytesLog);
        printf("stat.numBloquesOcupados = %u\n", stat.numBloquesOcupados);
    }

    // Desmontamos el dispositivo
    if (bumount() == FALLO) {
        perror(RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }

    return EXITO;
}