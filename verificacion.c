#include "verificacion.h"

int main(int argc, char **argv) {
    // Comprobar la sintaxis
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: <nombre_dispositivo> <directorio_simulacion>\n" RESET);
        return FALLO;
    }

    // Montar el dispositivo virtual
    if (bmount(argv[1]) == FALLO) {
        return FALLO;
    }

    char *camino_simul = argv[2];

    // sacamos la info del directorio de simulacion para saber cuanto ocupa
    struct STAT stat;
    if (mi_stat(camino_simul, &stat) < 0) {
        fprintf(stderr, RED "Error leyendo el directorio de simulación\n" RESET);
        bumount();
        return FALLO;
    }

    // numeor de entradas que hay en el directorio de simulacion
    int num_entradas = stat.tamEnBytesLog / sizeof(struct entrada);
    if (num_entradas != NUMPROCESOS) { // si no hay las mismas entradas que el NUMPROCESOS es que algo ha ido mal
        fprintf(stderr, RED "Error: El número de entradas (%d) no coincide con NUMPROCESOS (%d).\n" RESET, num_entradas, NUMPROCESOS);
        bumount();
        return FALLO;
    }

    // Crear el fichero "informe.txt" dentro del directorio de simulación
    char camino_informe[256];
    sprintf(camino_informe, "%sinforme.txt", camino_simul);
    if (mi_creat(camino_informe, 6) < 0) {
        fprintf(stderr, RED "Error creando informe.txt\n" RESET);
        bumount();
        return FALLO;
    }

    // Leer los directorios correspondientes a los procesos
    struct entrada entradas[num_entradas]; // array de struct entradas, una pos por proceso
    if (mi_read(camino_simul, entradas, 0, sizeof(entradas)) < 0) { // leemos el contenido de camino_simul y lo volcamos en entradas
        fprintf(stderr, RED "Error leyendo las entradas del directorio de simulación\n" RESET);
        bumount();
        return FALLO;
    }

    // Para cada entrada del directorio hacer
    for (int i = 0; i < num_entradas; i++) {
        // Extraer el PID a partir del nombre de la entrada
        char *pid_str = strchr(entradas[i].nombre, '_');
        if (pid_str == NULL) continue; // por si lee algo sin _
        pid_str++; // Saltar el _ para quedarnos con el número
        int pid = atoi(pid_str);

        // preparar estructura para guardar los resultados de este proceso
        struct INFORMACION info;
        info.pid = pid;
        info.nEscrituras = 0;

        char camino_prueba[TAMNOMBRE * PROFUNDIDAD]; // tamaño maximo de ruta
        sprintf(camino_prueba, "%s%s/prueba.dat", camino_simul, entradas[i].nombre);

        // Buffer para leer N registros a la vez 
        struct REGISTRO buffer_escrituras[256]; // 
        memset(buffer_escrituras, 0, sizeof(buffer_escrituras));

        int offset = 0; // empezamos a leer en 0 
        int bytes_leidos; // leemos hatsa que esto sea > 0 
        
        // Recorrer secuencialmente el fichero prueba.dat
        while ((bytes_leidos = mi_read(camino_prueba, buffer_escrituras, offset, sizeof(buffer_escrituras))) > 0) {
            // cuantos registros enteros ha devuelto la lectura
            int num_registros = bytes_leidos / sizeof(struct REGISTRO);
            
            // miramos uno por uno los registros que hemos leido 
            for (int r = 0; r < num_registros; r++) {
                // Si la escritura es válida (el PID coincide con el del proceso)
                if (buffer_escrituras[r].pid == info.pid) {
                    if (info.nEscrituras == 0) {
                        // Si es la primera escritura validada, inicializamos todo
                        info.PrimeraEscritura = buffer_escrituras[r];
                        info.UltimaEscritura = buffer_escrituras[r];
                        info.MenorPosicion = buffer_escrituras[r]; 
                        info.MayorPosicion = buffer_escrituras[r];
                    } else { // si ya habiamos validado alguna escritura 
                        // Comparar nº de escritura para actualizar primera y última
                        if (buffer_escrituras[r].nEscritura < info.PrimeraEscritura.nEscritura) {
                            info.PrimeraEscritura = buffer_escrituras[r];
                        }
                        if (buffer_escrituras[r].nEscritura > info.UltimaEscritura.nEscritura) {
                            info.UltimaEscritura = buffer_escrituras[r];
                        }
                        // La mayor posición se actualiza siempre porque leemos secuencialmente
                        info.MayorPosicion = buffer_escrituras[r];
                    }
                    info.nEscrituras++; // sumamos una escritura validada
                }
            }
            offset += bytes_leidos; // avanzamos el offset para la siguiente vuelta
            memset(buffer_escrituras, 0, sizeof(buffer_escrituras)); // Limpiar buffer
        }

        // cunaod ya hemos acabado con esa entrada

        // Formatear las fechas eliminando el salto de línea que añade asctime por defecto
        char fecha_primera[64], fecha_ultima[64], fecha_menor[64], fecha_mayor[64];
        
        strcpy(fecha_primera, asctime(localtime(&info.PrimeraEscritura.fecha)));
        fecha_primera[strlen(fecha_primera) - 1] = '\0';
        
        strcpy(fecha_ultima, asctime(localtime(&info.UltimaEscritura.fecha)));
        fecha_ultima[strlen(fecha_ultima) - 1] = '\0';
        
        strcpy(fecha_menor, asctime(localtime(&info.MenorPosicion.fecha)));
        fecha_menor[strlen(fecha_menor) - 1] = '\0';
        
        strcpy(fecha_mayor, asctime(localtime(&info.MayorPosicion.fecha)));
        fecha_mayor[strlen(fecha_mayor) - 1] = '\0';

        // Añadir la información del struct info al fichero informe.txt
        char buffer_informe[1024];
        memset(buffer_informe, 0, sizeof(buffer_informe));

        sprintf(buffer_informe, 
            "PID: %d\n"
            "Numero de escrituras: %d\n"
            "Primera Escritura\t%d\t%d\t%s\n"
            "Ultima Escritura\t%d\t%d\t%s\n"
            "Menor Posición\t\t%d\t%d\t%s\n"
            "Mayor Posición\t\t%d\t%d\t%s\n\n",
            info.pid, info.nEscrituras,
            info.PrimeraEscritura.nEscritura, info.PrimeraEscritura.nRegistro, fecha_primera,
            info.UltimaEscritura.nEscritura, info.UltimaEscritura.nRegistro, fecha_ultima,
            info.MenorPosicion.nEscritura, info.MenorPosicion.nRegistro, fecha_menor,
            info.MayorPosicion.nEscritura, info.MayorPosicion.nRegistro, fecha_mayor
        );

        // Calcular el offset actual de informe.txt para añadir al final
        int offset_informe = 0;
        struct STAT stat_informe;
        if (mi_stat(camino_informe, &stat_informe) == EXITO) {
            offset_informe = stat_informe.tamEnBytesLog;
        }
        
        // Escribir el informe de este proceso a partir del offset que hemos calculado antes 
        if (mi_write(camino_informe, buffer_informe, offset_informe, strlen(buffer_informe)) < 0) {
            fprintf(stderr, RED "Error escribiendo en informe.txt\n" RESET);
        }
        
        // Mostrar por pantalla que el proceso se ha validado
        printf("[%d) %d escrituras validadas en %s]\n", i+1, info.nEscrituras, camino_prueba);
    
        // pasa al siguiente proceso
    }

    // desmontar el dispositivo virtual
    bumount();
    
    return EXITO;
}