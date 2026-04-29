/**
 * @file mi_cat.c
 * @author Paula Cristina Morro Negre
 * @author Maria Teresa Sbert Gomila
 *
 * Sintaxis: ./mi_cat <disco> </ruta_fichero>
 * Muestra todo el contenido de un fichero.
 */

#include "directorios.h"

#define TAMBUFFER_MI_CAT 1500

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./mi_cat <nombre_dispositivo> </ruta_fichero>\n" RESET);
        return FALLO;
    }

    const char *disco = argv[1];
    const char *ruta = argv[2];

    if (bmount(disco) == FALLO) return FALLO;

    // obtenemos los metadatos del fichero para comprobar tipo y tamaño lógico
    struct STAT stat;
    int error = mi_stat(ruta, &stat);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }

    // Solo podemos hacer cat sobre ficheros, no directorios
    if (stat.tipo != 'f') {
        fprintf(stderr, RED "Error: la ruta indicada no es un fichero.\n" RESET);
        bumount();
        return FALLO;
    }

    unsigned char buffer[TAMBUFFER_MI_CAT];
    unsigned int offset = 0;
    int leidos = 0;
    unsigned int total_leidos = 0;

    // Leemos en bloques de TAMBUFFER hasta que no haya más datos
    while ((leidos = mi_read(ruta, buffer, offset, TAMBUFFER_MI_CAT)) > 0) {
        if (write(1, buffer, leidos) != leidos) {
            perror(RED "Error al escribir en la salida estándar" RESET);
            bumount();
            return FALLO;
        }
        total_leidos += leidos;
        offset += leidos;
    }

    if (leidos == FALLO) {
        fprintf(stderr, RED "Error: no hay permisos de lectura o fallo en mi_read.\n" RESET);
        bumount();
        return FALLO;
    }

    if (total_leidos != stat.tamEnBytesLog) {
        fprintf(stderr, RED "Advertencia: bytes leídos (%u) no coinciden con tamEnBytesLog (%u).\n" RESET, total_leidos, stat.tamEnBytesLog);
    }

    fprintf(stderr, "\nTotal_leidos %u\n", total_leidos);

    bumount();
    return EXITO;
}
