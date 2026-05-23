/**
 * @file simulacion.c
 * @author Paula Cristina Morro Negre
 * @author Maria Teresa Sbert Gomila
 *
 * Crea N procesos y cada uno realiza X escrituras 
*/

#include "simulacion.h"

int acabados = 0; // para llevar la cuenta de procesos terminados

// Función enterrador - se la llama cada vez que un hijo muere 
void reaper(){
    pid_t ended;
    signal(SIGCHLD, reaper);
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) {
        acabados++;
    }
}

int main(int argc, char **argv) {
    // Asociar la señal SIGCHLD al enterrador
    signal(SIGCHLD, reaper);

    // Comprobar la sintaxis del comando
    if (argc != 2) {
        fprintf(stderr, RED "Sintaxis: simulacion <disco>\n" RESET);
        return FALLO;
    }

    // Montar el dispositivo virtual (padre)
    if (bmount(argv[1]) == FALLO) {
        return FALLO;
    }

    // Crear el directorio de simulación /simul_aaaammddhhmmss/
    char camino_simul[100]; // donde guardaremos el nombre de la carpeta
    time_t t = time(NULL); // obtenemos la hora actual
    struct tm *tm = localtime(&t);
    sprintf(camino_simul, "/simul_%04d%02d%02d%02d%02d%02d/",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);
    // creamos el directorio simul
    if (mi_creat(camino_simul, 6) < 0) {
        fprintf(stderr, RED "Error al crear el directorio de simulación.\n" RESET);
        bumount();
        return FALLO;
    }

    printf(YELLOW "*** SIMULACIÓN DE %d PROCESOS REALIZANDO CADA UNO %d ESCRITURAS ***\n" RESET, NUMPROCESOS, NUMESCRITURAS);

    // bucle para lanzar procesos hijos
    for (int proceso = 1; proceso <= NUMPROCESOS; proceso++) {
        pid_t pid = fork();
        
        if (pid == 0) { // si el pid es 0 es que somos el hijo
            // el hijo vuelve a montar el dispositivo virtual 
            bmount(argv[1]);

            // Crear el directorio del proceso hijo añadiendo el PID al nombre
            char camino_proceso[200];
            sprintf(camino_proceso, "%sproceso_%d/", camino_simul, getpid());
            mi_creat(camino_proceso, 6); 

            // Crear el fichero prueba.dat
            char camino_fichero[300];
            sprintf(camino_fichero, "%sprueba.dat", camino_proceso);
            mi_creat(camino_fichero, 6);

            // Inicializar la semilla de números aleatorios
            srand(time(NULL) + getpid());

            // Bucle de escritura del hijo 
            for (int nescritura = 1; nescritura <= NUMESCRITURAS; nescritura++) {
                struct REGISTRO registro;
                registro.fecha = time(NULL);
                registro.pid = getpid();
                registro.nEscritura = nescritura;
                registro.nRegistro = rand() % REGMAX; // [0, 499.999]

                // Escribir el registro en el fichero en una posicion aleatoria
                mi_write(camino_fichero, &registro, registro.nRegistro * sizeof(struct REGISTRO), sizeof(struct REGISTRO));

                #if DEBUG_simulacion
                    // Elegir un color dependiendo del número de proceso
                    char *color = "";
                    switch (proceso % 3) {
                        case 1: color = ROSE; break;
                        case 2: color = CYAN; break;
                        case 0: color = MAGENTA; break; 
                    }

                    printf("%s[simulación.c → Escritura %d en %s]\n" RESET, color, nescritura, camino_fichero);
                #endif

                // Esperar 0,05 segundos (50.000 microsegundos)
                usleep(50000); 
            }

            // Avisar por pantalla que el proceso ha finalizado
            printf(GREEN "[Proceso %d: Completadas %d escrituras en %s]\n" RESET, proceso, NUMESCRITURAS, camino_fichero);

            // Desmontar dispositivo (hijo) y salir
            bumount();
            exit(0);  // Necesario para que se emita la señal SIGCHLD
        }

        // esperar 0,15 segundos (150.000 microsegundos) antes del siguiente hijo
        usleep(150000); 
    }

    // Permitir que el padre espere por todos los hijos
    while (acabados < NUMPROCESOS) {
        pause();
    }

    // Desmontar el dispositivo (padre)
    bumount();
    
    // Salir del simulador con éxito
    exit(0);
}