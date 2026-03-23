#include "ficheros.h"

int main (int argc, char *argv[]) {

    int offsets[] = {9000, 209000, 30725000, 409605000, 480000000};

    // Validamos el número de argumentos introducidos
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: escribir <nombre_dispositivo> <\"texto\"> <diferentes_inodos>\n" RESET); 
        fprintf(stderr, RED "Offsets: %d, %d, %d, %d, %d\n" RESET, offsets[0], offsets[1], offsets[2], offsets[3], offsets[4]); 
        fprintf(stderr, RED "Si diferentes_inodos = 0 se reserva un solo inodo para todos los offsets\n" RESET); 
        return FALLO;
    }

    // Guardamos el primer argumento (nombre del dispositivo)
    char *nombre_dispositivo = argv[1];
    // Guardamos el segundo argumento (texto a escribir)
    char *texto = argv[2];
    // Guardamos el tercer argumento (offset)
    int diferentes_inodos = atoi(argv[3]);
    // Calculamos el número de bytes a escribir
    int nbytes = strlen(texto);
    printf("longitud texto: %d\n\n", nbytes);

    // Montamos el dispositivo
    if (bmount(nombre_dispositivo) == FALLO) {
        perror(RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    unsigned int ninodo; 

    for (int i = 0; i < (sizeof(offsets) / sizeof(int)); i++) {
        // Reservamos un inodo para el nuevo fichero
        if (diferentes_inodos == 1 || i == 0) {
            ninodo = reservar_inodo('f', 6);
            if (ninodo == FALLO) {
                perror(RED "Error al reservar el inodo\n" RESET);
                bumount();
                return FALLO;
            }
        }

        printf("\nNº inodo reservado: %d\n",ninodo);
        printf("Offset: %d\n", offsets[i]);

        // Escribimos los datos en el inodo reservado
        int escritos = mi_write_f(ninodo, texto, offsets[i], nbytes);
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

        inodo_t inodo;
        leer_inodo(ninodo, &inodo);
    }

    // Desmontamos el dispositivo
    if (bumount() == FALLO) {
        perror(RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }

    return EXITO;
}