/**
 * @file leer.c
 * @author Paula Cristina Morro Negre 
 * @author Maria Teresa Sbert Gomila
*/

#include "directorios.h"

#define TAM_BUFFER 1500 // Tamaño del buffer para leer (valor razonable para la lectura)

int main(int argc, char *argv[]) {
    // Validamos el número de argumentos introducidos
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: leer <nombre_dispositivo> <ninodo>\n" RESET);
        return FALLO;
    }
    
    // Guardamos el primer argumento (nombre del dispositivo)
    char *nombre_dispositivo = argv[1];
    // Guardamos el segundo argumento (ninodo)
    int ninodo = atoi(argv[2]);

    // Montamos el dispositivo
    if (bmount(nombre_dispositivo) == FALLO) return FALLO;  
    
    // Creamos un buffer para almacenar los datos leídos y variables para controlar la lectura
    unsigned char buffer[TAM_BUFFER];
    // offset: posición actual de lectura en bytes lógicos
    int offset = 0;
    // leidos: cantidad de bytes leídos en la última llamada a mi_read_f
    int leidos = 0;
    // total_leidos: cantidad total de bytes leídos
    int total_leidos = 0;

    // Limpiamos el buffer inicialmente
    memset(buffer, 0, TAM_BUFFER);

    // Bucle de lectura: leemos de TAM_BUFFER en TAM_BUFFER hasta que mi_read_f devuelva 0 (EOF)
    while ((leidos = mi_read_f(ninodo, buffer, offset, TAM_BUFFER)) != 0) {
        if (leidos == FALLO) {
            fprintf(stderr, RED "No hay permisos de lectura\n" RESET);
            break;;
        }
        // Escribimos en la salida estándar (1) lo que hayamos leído
        write(1, buffer, leidos);
        // Actualizamos el total de bytes leídos y el offset para la siguiente lectura 
        total_leidos += leidos;
        offset += TAM_BUFFER;
        memset(buffer, 0, TAM_BUFFER); // Limpiamos para la siguiente iteración
    }

    // Mostramos por stderr el total de bytes leídos
    fprintf(stderr, "\ntotal_leidos: %d\n", total_leidos);

    // Mostramos stat para ver que el tamaño coincide
    struct STAT stat;
    if (mi_stat_f(ninodo, &stat) == FALLO) {
        bumount();
        return FALLO;
    }

    fprintf(stderr, "tamEnBytesLog: %u\n", stat.tamEnBytesLog);

    bumount();
    return EXITO;
}