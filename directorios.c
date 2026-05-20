/**
 * @file directorios.c
 * @author Paula Cristina Morro Negre 
 * @author Maria Teresa Sbert Gomila
*/

#include "directorios.h"

// 0: sin caché
// 1: última L/E  (1 entrada para escritura + 1 para lectura)
// 2: tabla FIFO  (CACHE_SIZE entradas para escritura + CACHE_SIZE para lectura)
// 3: tabla LRU   (sello de tiempo)
#define USARCACHE 3
 
#if USARCACHE
    struct UltimaEntrada {
        char camino[TAMNOMBRE * PROFUNDIDAD];
        int  p_inodo;
        #if USARCACHE == 3
        struct timeval ultima_consulta;
        #endif
    };
    
    #if USARCACHE == 1
        // creamos variables para una sola entrada
        static struct UltimaEntrada UltimaEntradaEscritura;
        static struct UltimaEntrada UltimaEntradaLectura;
    #else
        #define CACHE_SIZE 3
        // podemos guardar hasta tres rutas 
        static struct UltimaEntrada UltimasEntradasEscritura[CACHE_SIZE];
        static struct UltimaEntrada UltimasEntradasLectura[CACHE_SIZE];
    #endif
#endif
 
#if USARCACHE == 2
    static int IdxInsercionEscritura = CACHE_SIZE - 1; // para saber es que posición nos toca escribir
    static int IdxInsercionLectura = CACHE_SIZE - 1;
    static int EntradasEscritura = 0; // cuántas posiciones están ya ocupadas
    static int EntradasLectura = 0;
#endif

/*
 *  Divide el camino en la parte inicial y en la parte final y da el tipo de inicial
 *  @param camino ruta de directorios para llegar al fichero/directorio indicado
 *  @param inicial porcion de *camino comprendida entre los dos primeros '/' 
 *  @param final resto de *camino a partir del segundo '/' inclusive
 *  @param tipo se le asignara ‘d’ o ‘f’ en función de si en *inicial hay un nombre de directorio o un nombre de fichero
 *  @return 0 si todo ha ido bien, -2 si el camino era incorrecto (no comineza por /)
 */
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    // error si no comienza por /
    if (camino[0] != '/') {
        return ERROR_CAMINO_INCORRECTO;
    }
    
    const char *caminoAux = camino + 1; // camino sin el primero /

    // strchr() - > busca la primera aparición de un caracter 
    // camino + 1 para saltarnos la primera barra que siempre esta
    const char *segunda_barra = strchr(caminoAux, '/'); // apunta a la segunda aparicion de / en camino (primera en caminoAux)

    if (segunda_barra) { 
        *tipo = 'd'; // si hay segunda barra es un directorio

        // añadimos a inicial todo el contenido de caminoAux1 entre los primeros '/'
        strncpy(inicial, caminoAux, segunda_barra - caminoAux); 
        inicial[segunda_barra - camino - 1] = '\0'; // strncpy no añade el nulo automáticamentea al final si cortamos
    
        // ponemos en final el resto de camino desde el segundo /
        strcpy(final, segunda_barra);
    } else {
        *tipo = 'f'; // si no hay segunda barra es un fichero

        // inicial es todo lo que hay después de la primera barra
        strcpy(inicial, caminoAux);

        // final es string vacio
        strcpy(final,"");
    }

    return EXITO;    
}

/*
 *  Busca una determinada entrada entre las entradas del inodo correspondiente al inodo padre de la entrada
 *  @param camino_principal ruta de directorios para llegar al fichero/directorio indicado
 *  @param p_inodo_dir inodo del directorio donde buscamos la entrada 
 *  @param p_inodo guardamos el inodo del archivo final si lo encontramos
 *  @param p_entrada numero de entrada dentro del inodo directorio que lo contiene
 *  @param reservar si esta a 1 y la entrada no existe se crea
 *  @param permisos permisos de la entrada
 *  @return 
 */
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos) {
    struct entrada entrada;
    inodo_t inodo_dir;
    struct superbloque SB;
    char inicial[sizeof(entrada.nombre)];
    char final[strlen(camino_parcial)];
    char tipo;
    int cant_entradas_inodo, num_entrada_inodo;

    // si camnino principal es / (directorio raiz)
    if (strcmp(camino_parcial, "/") == 0) { 
        *p_inodo = 0; // nuestra raiz siempre estará asociada al inodo 0
        *p_entrada = 0;
        return EXITO;
    }
    
    // extraemos la primera parte del camino 
    if (extraer_camino(camino_parcial, inicial, final, &tipo) < 0) return ERROR_CAMINO_INCORRECTO;
     

    #if DEBUG_buscar_entrada
        fprintf(stderr, GRAY"[buscar_entrada() -> inicial: %s, final: %s, reservar: %d]\n" RESET, inicial, final, reservar);
    #endif

    // buscamos la entrada cuyo nombre se encuentra en inicial
    // leemos el inodo directorio
    if (leer_inodo(*p_inodo_dir, &inodo_dir) < 0) return ERROR_PERMISO_LECTURA;
    // comprobar si el directorio tiene permisos de lectura
    if ((inodo_dir.permisos & PERMISO_LECTURA) != PERMISO_LECTURA) {
        #if DEBUG_buscar_entrada
            fprintf(stderr, GRAY"[buscar_entrada() -> el inodo %u no tiene permisos de lectura]\n"RESET, *p_inodo_dir);
        #endif
        return ERROR_PERMISO_LECTURA;
    } 

    // inicializamos el buffer de lectura con 0s
    // buffer para leer bloques de entradas y no ir una por una
    struct entrada buffer_entradas[BLOCKSIZE / sizeof(struct entrada)]; // array de las entradas que caben en un bloque
    memset(buffer_entradas, 0, BLOCKSIZE);

    // calcuar la cantidad de entradas que contiene un inodo
    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);
    num_entrada_inodo = 0; // numero de entrada inicial 

    entrada = buffer_entradas[0];

    // si el directorio no esta vacio (si hay al menos una entrada) 
    // buscamos el nombre inicial
    if (cant_entradas_inodo > 0) { 
        // leer primer bloque de entradas del directorio 
        if (mi_read_f(*p_inodo_dir, buffer_entradas, 0, BLOCKSIZE) < 0) return FALLO;
        
        entrada = buffer_entradas[0]; // primera entrada del directorio 

        // mientras no hayamos recorrido todas las entradas y el nombre no coincida
        while ((num_entrada_inodo < cant_entradas_inodo) && strcmp(inicial, entrada.nombre)) {
            num_entrada_inodo ++; // pasamos a la siguiente entrada
            
            // si hemos llegado al final del buffer actual
            if (num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada)) == 0) {
                memset(buffer_entradas, 0, BLOCKSIZE);
                // leemos el siguiente bloque del directorio
                if (mi_read_f(*p_inodo_dir, buffer_entradas, num_entrada_inodo * sizeof(struct entrada), BLOCKSIZE) < 0) return FALLO;
            }

            // actualizamos la entrada
            entrada = buffer_entradas[num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada))];  
        }
    }
    
    // si la entrada no existe 
    if (strcmp(inicial, entrada.nombre) && (num_entrada_inodo == cant_entradas_inodo)) {
        switch (reservar) {
            // modo consulta. si no existe retornamos error
            case 0: 
                return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
            // modo escritura: Creamos la entrada en el directorio referenciado por *p_inodo_dir
            case 1: 
                // un fichero no puede contener entradas (solo los directorios)
                if (inodo_dir.tipo == 'f') return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;

                // para crear una entrada necesitamos permiso de escritura en el directorio padre
                if ((inodo_dir.permisos & PERMISO_ESCRITURA) != PERMISO_ESCRITURA) return ERROR_PERMISO_ESCRITURA;

                strcpy(entrada.nombre, inicial); // Asignamos el nombre buscado a la nueva entrada 

                int inodo_reservado = 0; // para comprobar si hay que liberar en caso de fallo 

                if (tipo == 'd') {
                    if (strcmp(final, "/") == 0) {
                        // reservar inodo como directorio y asignarlo a la entrada 
                        entrada.ninodo = reservar_inodo('d', permisos);
                        inodo_reservado = 1;
                        #if DEBUG_buscar_entrada
                            fprintf(stderr, GRAY"[buscar_entrada() -> reservado inodo %d tipo %c con permisos %d para %s]\n"RESET, entrada.ninodo, 'd', permisos, entrada.nombre);
                        #endif
                    } else {
                        return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
                    }
                }
                else {// es un fuchero
                    // reservar un inodo como fichero y asignarlo a la entrada
                    entrada.ninodo = reservar_inodo('f', permisos);  
                    inodo_reservado = 1;
                    #if DEBUG_buscar_entrada
                        fprintf(stderr, GRAY"[buscar_entrada() -> reservado inodo %d tipo %c con permisos %d para %s]\n"RESET, entrada.ninodo, 'f', permisos, entrada.nombre);
                    #endif
                }

                // escribir la entrada en el directorio padre (por el final) 
                memset(buffer_entradas, 0, BLOCKSIZE);
                buffer_entradas[0] = entrada;
                // si ha habido fallo en la escritura 
                if (mi_write_f(*p_inodo_dir, buffer_entradas, inodo_dir.tamEnBytesLog, sizeof(entrada)) == FALLO) {
                    // y ya habiamos reservado un inodo
                    if (inodo_reservado) {
                        // lo liberamos 
                        liberar_inodo(entrada.ninodo);
                    }
                    return FALLO;
                }
                #if DEBUG_buscar_entrada
                    fprintf(stderr, GRAY"[buscar_entrada() -> creada entrada: %s, %d]\n"RESET, entrada.nombre, entrada.ninodo);
                #endif
            break;
        }
    }

    // si hemos llegado al final del camino
    if ((strcmp(final, "") == 0) || (strcmp(final, "/") == 0)) { 
        // si ya existía y estábamos intentando crear
        if ((num_entrada_inodo < cant_entradas_inodo) && (reservar == 1)) {
            return ERROR_ENTRADA_YA_EXISTENTE;
        }

        // cortamos la recursividad
        *p_inodo = entrada.ninodo;
        *p_entrada = num_entrada_inodo;
    } 
    // aun queda camino ppr recorrer 
    else {
        *p_inodo_dir = entrada.ninodo;
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }

    return EXITO;
}

void mostrar_error_buscar_entrada(int error) {
    switch (error) {
        case -2 : fprintf(stderr, RED "Error: Camino incorrecto.\n"RESET); break;
        case -3 : fprintf(stderr, RED "Error: Permiso denegado de lectura.\n"RESET); break;
        case -4 : fprintf(stderr, RED "Error: No existe el archivo o el directorio.\n"RESET); break;
        case -5 : fprintf(stderr, RED "Error: No existe algún directorio intermedio.\n"RESET); break;
        case -6 : fprintf(stderr, RED "Error: Permiso denegado de escritura.\n"RESET); break;
        case -7 : fprintf(stderr, RED "Error: El archivo ya existe.\n"RESET); break;
        case -8 : fprintf(stderr, RED "Error: No es un directorio.\n"RESET); break;    
    }
}

/*
 *  Función que crea un fichero/directorio y su entrada de directorio
 *  @param camino cambino del fichero/directorio
 *  @param permisos permisos que tendrá el fichero/directorio
 *  @return 
 */
int mi_creat(const char *camino, unsigned char permisos) {    
    mi_waitSem();
    
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    
    // buscar_entrada() con reservar=1 para que cree el directorio si no existe 
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);

    mi_signalSem();

    // esta función devuelve lo que devuelva buscar_entrada
    return error;
}

/*  
 *  Lista el contenido de un directorio o la información de un fichero
 *  @param camino ruta del fichero/directorio
 *  @param buffer donde guardaremos el listado resultante
 *  @param tipo 'd' para directorio, 'f' para fichero
 *  @param flag 'l' para listado largO, 'n' para listado simple
 *  @return número de entradas listadas o FALLO
 */
int mi_dir(const char *camino, char *buffer, char tipo, char flag) {
    unsigned int p_inodo_dir = 0, p_inodo, p_entrada;
    int err;

    // buscamos la ruta en el disco 
    if ((err = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0)) < 0)
        return err;

    inodo_t inodo_dir, inodo;
    // leemos el inodo que hemso encontrado
    if (leer_inodo(p_inodo, &inodo_dir) == FALLO)
        return FALLO;

    if (inodo_dir.tipo != tipo) {
        fprintf(stderr, RED "Error: la sintaxis no concuerda con el tipo\n" RESET);
        return FALLO;
    }

    // si es un fichero listamos su entrada en el padre 
    if (tipo == 'f') {
        // leemos el padre para ver la entrada del fichero
        if (leer_inodo(p_inodo_dir, &inodo_dir) == FALLO)
            return FALLO;
    } else {
        p_inodo_dir = p_inodo; // si es directorio el inodo a recorrer es el mismo
    }

    struct entrada buffer_entradas[BLOCKSIZE / sizeof(struct entrada)]; // buffer para leer bloques de entradas de directorio
    memset(buffer_entradas, 0, BLOCKSIZE);
    struct entrada entrada = buffer_entradas[0];

    int cant_entradas_inodo, num_entrada_inodo;
    // si es un directorio listamos su contenido
    if (tipo == 'd') {
        cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada); // num de entradas que tiene el directorio
        num_entrada_inodo = 0;
    } else {
        // si es un fichero solo queremos mostrar esa entrada 
        cant_entradas_inodo = p_entrada + 1;
        num_entrada_inodo = p_entrada;
    }

    // bucle para recorrer las entradas del directorio
    for (; num_entrada_inodo < cant_entradas_inodo; num_entrada_inodo++) {
        if (tipo == 'd') {
            // solo leemos el disco cuando cambiamos de bloque de entradas 
            if (num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada)) == 0) {
                memset(buffer_entradas, 0, BLOCKSIZE);
                if (mi_read_f(p_inodo_dir, buffer_entradas, num_entrada_inodo * sizeof(struct entrada), BLOCKSIZE) < 0) return FALLO;
            }
            entrada = buffer_entradas[num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada))];  // leer siguiente entrada
        } else {
            // s es un fichero leemos solo el bloque donde esta la entrada del fichero
            memset(buffer_entradas, 0, BLOCKSIZE);
            int num_bloque = num_entrada_inodo * sizeof(struct entrada) / BLOCKSIZE;
            if (mi_read_f(p_inodo_dir, buffer_entradas, num_bloque * BLOCKSIZE, BLOCKSIZE) < 0)
                return FALLO;
            entrada = buffer_entradas[num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada))];  // leer siguiente entrada
        }

        // leemos el inodo de la entrada actual 
        if (leer_inodo(entrada.ninodo, &inodo) == FALLO)
            return FALLO;

        // si es 'l' mostramos la info detallada 
        if (flag == 'l') {
            char tmp[BLOCKSIZE]; 
            sprintf(tmp, "%c\t", inodo.tipo); // añadimos tipo (f o d)
            strcat(buffer, tmp);

            // Permisos
            strcat(buffer, (inodo.permisos & 4) ? "r" : "-");
            strcat(buffer, (inodo.permisos & 2) ? "w" : "-");
            strcat(buffer, (inodo.permisos & 1) ? "x" : "-");
            strcat(buffer, "\t");

            // Fecha
            struct tm *tm;
            tm = localtime(&inodo.mtime);
            sprintf(tmp, "%d-%02d-%02d %02d:%02d:%02d\t", tm->tm_year + 1900, 
                    tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
            strcat(buffer, tmp);

            // Tamaño
            sprintf(tmp, "%d\t\t", inodo.tamEnBytesLog);
            strcat(buffer, tmp);
        }

        // asignar colores al nombre de la entrada en función del tipo
        if (inodo.tipo == 'd') strcat(buffer, BLUE); // es un directorio
            
        else strcat(buffer, GREEN); // es un fichero 
        
        strcat(buffer, entrada.nombre); // nombre 
        strcat(buffer, RESET);  // resetear color de la entrada
        strcat(buffer, (flag == 'l') ? "\n" : "\t");
    }

    return (tipo == 'd') ? cant_entradas_inodo : 1;
}

/*
 *  Función que cambia los permisos de un fichero/directorio
 *  @param camino cambino del fichero/directorio
 *  @param permisos permisos que tendrá el fichero/directorio
 *  @return 
 */
int mi_chmod(const char *camino, unsigned char permisos) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    
    // llamar a buscar_entrada para encontrar el inodo del fichero/directorio
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, permisos); // con 0 porque solo queremos mirar que exista, no crearlo

    // si buscar_entrada devuelve algo que no sea EXITO devolvemos el error
    if (error != EXITO) return error;

    // llamar a mi_chmod_f para cambiar los permisos del inodo
    return mi_chmod_f(p_inodo, permisos); // devolvemos lo que devuelva mi_chmod_f 
}

/*
 *  Función para mostrar la información de un inodo de un fichero/directorio
 *  @param camino cambino del fichero/directorio
 *  @param permisos permisos que tendrá el fichero/directorio
 *  @return 
 */
int mi_stat(const char *camino, struct STAT *p_stat) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    
    // llamar a buscar_entrada para encontrar el inodo del fichero/directorio
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0); // con 0 porque solo queremos mirar que exista, no crearlo

    // si buscar_entrada devuelve algo que no sea EXITO devolvemos el error
    if (error != EXITO) return error;

    // si la entrada existe llamamos a mi_stat_f
    return mi_stat_f(p_inodo, p_stat); // devolvemos lo que devuelva mi_stst_f
}

/*  
 *  Escribe nbytes de buf en el fichero indicado por camino a partir de offset.
 *  @param camino ruta del fichero/directorio
 *  @param buf donde guardaremos el listado resultante
 *  @param offset a partir de donde queremos escribir
 *  @param nbytes cuantos bytes queremos escribir
 *  @return bytes escritos o FALLO
 */
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int cache_hit = 0; // para saber si ya hemos encontraod ese camino y lo tenemos en la cache
 
    // ---------- USARCACHE == 1: una sola entrada para escritura ----------
    #if USARCACHE == 1
        if (strcmp(UltimaEntradaEscritura.camino, camino) == 0) { // si la ruta que buscamso es igual a la unica que tenemos guardada
            p_inodo   = UltimaEntradaEscritura.p_inodo; // copiamos el numero de inodo
            cache_hit = 1; // marcamos como que lo hemos encontrado
            #if DEBUG_mi_write
                fprintf(stderr, CYAN "[mi_write() -> Utilizamos la caché de escritura en vez de llamar a buscar_entrada()]\n" RESET);
            #endif
        }
    
    // ---------- USARCACHE == 2: tabla FIFO ----------
    #elif USARCACHE == 2
        // revisamos las tres posiciones de la cache
        for (int i = 0; i < CACHE_SIZE; i++) {
            if (strcmp(UltimasEntradasEscritura[i].camino, camino) == 0) { // si las rutas son iguales
                p_inodo   = UltimasEntradasEscritura[i].p_inodo; // copiamos el numero de inodo
                cache_hit = 1; // marcamos que lo hemos encontrado
                #if DEBUG_mi_write
                    fprintf(stderr, CYAN "[mi_write() -> Utilizamos cache[%d]: %s]\n" RESET, i, camino);
                #endif
                break;
            }
        }
    
    // ---------- USARCACHE == 3: tabla LRU ----------
    #elif USARCACHE == 3
        for (int i = 0; i < CACHE_SIZE; i++) {
            if (strcmp(UltimasEntradasEscritura[i].camino, camino) == 0) {
                p_inodo   = UltimasEntradasEscritura[i].p_inodo;
                cache_hit = 1;
                // Actualizamos el sello de tiempo del hit a la hora actual
                gettimeofday(&UltimasEntradasEscritura[i].ultima_consulta, NULL);
                #if DEBUG_mi_write
                    fprintf(stderr, CYAN "[mi_write() -> Utilizamos cache[%d]: %s]\n" RESET, i, camino);
                #endif
                break;
            }
        }
    #endif
 
    // ---------- si no estaba en la cache ----------
    if (!cache_hit) {
        // buscamos la entrada 
        int err = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (err < 0) return err; 
 
    #if USARCACHE == 1
        // guardamos la nueva ruta 
        strcpy(UltimaEntradaEscritura.camino, camino);
        UltimaEntradaEscritura.p_inodo = p_inodo;
        #if DEBUG_mi_write
            fprintf(stderr, YELLOW "[mi_write() -> Actualizamos la caché de escritura]\n" RESET);
        #endif
    
    #elif USARCACHE == 2
            // avanzamos el índice antes de poner la entrada en la cache
            IdxInsercionEscritura = (IdxInsercionEscritura + 1) % CACHE_SIZE;
            strcpy(UltimasEntradasEscritura[IdxInsercionEscritura].camino, camino);
            UltimasEntradasEscritura[IdxInsercionEscritura].p_inodo = p_inodo;
            if (EntradasEscritura < CACHE_SIZE) EntradasEscritura++;
            #if DEBUG_mi_write
                fprintf(stderr, YELLOW "[mi_write() -> Reemplazamos cache[%d]: %s]\n" RESET, IdxInsercionEscritura, camino);
            #endif
    
    #elif USARCACHE == 3
        // Buscamos el índice LRU (vacío o con el timestamp más antiguo)
        int lru_idx = 0;
        struct timeval oldest = {LONG_MAX, LONG_MAX};
        for (int i = 0; i < CACHE_SIZE; i++) {
            if (UltimasEntradasEscritura[i].camino[0] == '\0') {
                lru_idx = i;
                break;
            }
            if (UltimasEntradasLectura[i].ultima_consulta.tv_sec < oldest.tv_sec || (UltimasEntradasLectura[i].ultima_consulta.tv_sec == oldest.tv_sec && UltimasEntradasLectura[i].ultima_consulta.tv_usec < oldest.tv_usec)) {
                oldest  = UltimasEntradasEscritura[i].ultima_consulta;
                lru_idx = i;
            }
        }
        // ponemos la ruta nueva
        strcpy(UltimasEntradasEscritura[lru_idx].camino, camino);
        UltimasEntradasEscritura[lru_idx].p_inodo = p_inodo;
        // con el tiempo actual
        gettimeofday(&UltimasEntradasEscritura[lru_idx].ultima_consulta, NULL);
        #if DEBUG_mi_write
            fprintf(stderr, YELLOW "[mi_write() -> Reemplazamos cache[%d]: %s]\n" RESET, lru_idx, camino);
        #endif
    #endif
    }
 
    int escritos = mi_write_f(p_inodo, buf, offset, nbytes);
    return escritos; 
}

/*  
 *  Lee nbytes de buf en el fichero indicado por camino a partir de offset.
 *  @param camino ruta del fichero/directorio
 *  @param buf donde guardaremos el listado resultante
 *  @param offset a partir de donde queremos leer
 *  @param nbytes cuantos bytes queremos leer
 *  @return bytes escritos o FALLO
 */
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int cache_hit = 0;
 
    // ---------- USARCACHE == 1 ----------
    #if USARCACHE == 1
        if (strcmp(UltimaEntradaLectura.camino, camino) == 0) {
            p_inodo   = UltimaEntradaLectura.p_inodo;
            cache_hit = 1;
            #if DEBUG_mi_read
                fprintf(stderr, CYAN "\n[mi_read() -> Utilizamos la caché de lectura en vez de llamar a buscar_entrada()]\n" RESET);
            #endif
        }
    
    // ---------- USARCACHE == 2: tabla FIFO ----------
    #elif USARCACHE == 2
        for (int i = 0; i < CACHE_SIZE; i++) {
            if (strcmp(UltimasEntradasLectura[i].camino, camino) == 0) {
                p_inodo   = UltimasEntradasLectura[i].p_inodo;
                cache_hit = 1;
                #if DEBUG_mi_read
                    fprintf(stderr, CYAN "[mi_read() -> Utilizamos cache[%d]: %s]\n" RESET, i, camino);
                #endif
                break;
            }
        }
    
    // ---------- USARCACHE == 3: tabla LRU ----------
    #elif USARCACHE == 3
        for (int i = 0; i < CACHE_SIZE; i++) {
            if (strcmp(UltimasEntradasLectura[i].camino, camino) == 0) {
                p_inodo   = UltimasEntradasLectura[i].p_inodo;
                cache_hit = 1;
                gettimeofday(&UltimasEntradasLectura[i].ultima_consulta, NULL);
                #if DEBUG_mi_read
                    fprintf(stderr, CYAN "[mi_read() -> Utilizamos cache[%d]: %s]\n" RESET, i, camino);
                #endif
                break;
            }
        }
    #endif
 
    // ---------- si no estaba en la cache ----------
    if (!cache_hit) {
        int err = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (err < 0) return err;
 
    #if USARCACHE == 1
        strcpy(UltimaEntradaLectura.camino, camino);
        UltimaEntradaLectura.p_inodo = p_inodo;
        #if DEBUG_mi_read
            fprintf(stderr, YELLOW "[mi_read() -> Actualizamos la caché de lectura]\n" RESET);
        #endif
    
    #elif USARCACHE == 2
        IdxInsercionLectura = (IdxInsercionLectura + 1) % CACHE_SIZE;
        strcpy(UltimasEntradasLectura[IdxInsercionLectura].camino, camino);
        UltimasEntradasLectura[IdxInsercionLectura].p_inodo = p_inodo;
        if (EntradasLectura < CACHE_SIZE) EntradasLectura++;
        #if DEBUG_mi_read
            fprintf(stderr, YELLOW "[mi_read() -> Reemplazamos cache[%d]: %s]\n" RESET, IdxInsercionLectura, camino);
        #endif
    
    #elif USARCACHE == 3
        int lru_idx = 0;
        struct timeval oldest = {LONG_MAX, LONG_MAX};
        for (int i = 0; i < CACHE_SIZE; i++) {
            if (UltimasEntradasLectura[i].camino[0] == '\0') {
                lru_idx = i;
                break;
            }
            if (UltimasEntradasLectura[i].ultima_consulta.tv_sec < oldest.tv_sec || (UltimasEntradasLectura[i].ultima_consulta.tv_sec == oldest.tv_sec && UltimasEntradasLectura[i].ultima_consulta.tv_usec < oldest.tv_usec)) {
                oldest  = UltimasEntradasLectura[i].ultima_consulta;
                lru_idx = i;
            }
        }
        strcpy(UltimasEntradasLectura[lru_idx].camino, camino);
        UltimasEntradasLectura[lru_idx].p_inodo = p_inodo;
        gettimeofday(&UltimasEntradasLectura[lru_idx].ultima_consulta, NULL);
        #if DEBUG_mi_read
            fprintf(stderr, YELLOW "[mi_read() -> Reemplazamos cache[%d]: %s]\n" RESET, lru_idx, camino);
        #endif
    #endif
    }
 
    return mi_read_f(p_inodo, buf, offset, nbytes);
}

/*  
 *  Crea el enlace de una entrada de directorio camino2 al inodo especificado por otra entrada de directorio camino1
 *  @param camino1
 *  @param camino2
 *  @return 
 */
int mi_link(const char *camino1, const char *camino2) {
    mi_waitSem();
    
    unsigned int p_inodo_dir1 = 0, p_inodo1, p_entrada1;
    unsigned int p_inodo_dir2 = 0, p_inodo2, p_entrada2;
    int err;

    // comprobamso que la entrada camino1 exista y tenga permisos de lectura 
    if ((err = buscar_entrada(camino1, &p_inodo_dir1, &p_inodo1, &p_entrada1, 0, 0)) < 0) {
        mi_signalSem();
        return err;
    }
    
    // camino1 y camino2 han de referirse a un fichero
    if ((camino1[strlen(camino1) - 1] == '/') || (camino2[strlen(camino2) - 1] == '/')) {
        fprintf(stderr, RED "La ruta de los ficheros no puede ser un directorio\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    // camino2 no tiene que existir
    // la creamos con buscar_entrada() con permisos 6 y reservar=1
    if ((err = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, 1, 6)) < 0) {
        mi_signalSem();
        return err;
    }
    
    // leer el inodo asociado a camino2
    inodo_t inodo2;
    if (leer_inodo(p_inodo2, &inodo2) == FALLO) {
        mi_signalSem();
        return FALLO;
    }
        
    // Leemos la entrada creada correspondiente a camino2, o sea la entrada p_entrada2 de p_inodo_dir2
    struct entrada entrada2;
    if (mi_read_f(p_inodo_dir2, &entrada2, p_entrada2 * sizeof(struct entrada), sizeof(struct entrada)) == FALLO) { 
        mi_signalSem(); return FALLO;
    }
    
    // Creamos el enlace: Asociamos a esta entrada el mismo inodo que el asociado a la entrada de camino1, es decir p_inodo1
    entrada2.ninodo = p_inodo1;

    // Escribimos la entrada modificada en p_inodo_dir2
    if (mi_write_f(p_inodo_dir2, &entrada2, p_entrada2 * sizeof(struct entrada), sizeof(struct entrada)) == FALLO) { 
        mi_signalSem();
        return FALLO;
    }
    
    // Liberamos el inodo que se ha asociado a la entrada creada, p_inodo2
    if (liberar_inodo(p_inodo2) == FALLO) { 
        mi_signalSem();
        return FALLO;
    }
    
    // Incrementamos la cantidad de enlaces (nlinks) de p_inodo1, actualizamos el ctime y lo salvamos
    inodo_t inodo1;
    if (leer_inodo(p_inodo1, &inodo1) == FALLO) {
        mi_signalSem();
        return FALLO;
    }
    
    inodo1.nlinks++;
    inodo1.ctime = time(NULL);

    if (escribir_inodo(p_inodo1, &inodo1) < 0) {
        mi_signalSem();
        return FALLO;
    }

    mi_signalSem();

    return EXITO;
}

/*  
 *  Borra la entrada de directorio especificada y, si es el último enlace, borra el fichero/directorio
 *  @param camino
 *  @return 
 */
int mi_unlink(const char *camino) {    
    mi_waitSem();
    
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int err;

    // Hay que comprobar que la entrada camino exista
    if ((err = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0)) < 0) {
        mi_signalSem();
        return err;
    }
    
    // leer el inodo
    inodo_t inodo;
    if (leer_inodo(p_inodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    // Si es directorio y no está vacío, entonces no se puede borrar
    if (inodo.tipo == 'd' && inodo.tamEnBytesLog > 0) {
        fprintf(stderr, RED "Error: El directorio %s no está vacío\n" RESET, camino);
        mi_signalSem();
        return FALLO;
    }
    
    // leemos el inodo asociado al directorio que contiene la entrada que queremos eliminar (p_inodo_dir)
    inodo_t inodo_dir;
    if (leer_inodo(p_inodo_dir, &inodo_dir) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    // obtenemos el nº de entradas que tiene
    int num_entradas = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    struct entrada buffer_entradas[BLOCKSIZE / sizeof(struct entrada)];

    // si la entrada a eliminar no es la ultima 
    if (p_entrada != (num_entradas - 1)) {
        // Leer la última y escribirla en la posición (bloque) de la entrada que queremos eliminar
        int pos_ult_entrada = ((num_entradas - 1) * sizeof(struct entrada)) / BLOCKSIZE;
        memset(buffer_entradas, 0, BLOCKSIZE);
        
        // Leemos el bloque donde se encuentra última entrada
        if (mi_read_f(p_inodo_dir, buffer_entradas, pos_ult_entrada * BLOCKSIZE, BLOCKSIZE) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
        
        // La extraemos del bloque leído
        struct entrada ultima_entrada = buffer_entradas[(num_entradas - 1) % (BLOCKSIZE / sizeof(struct entrada))];

        // Bloque lógico donde se encuentra la entrada a eliminar
        int pos_entrada_elim = (p_entrada * sizeof(struct entrada)) / BLOCKSIZE;
        memset(buffer_entradas, 0, BLOCKSIZE);

        // Leemos bloque donde se encuentra
        mi_read_f(p_inodo_dir, buffer_entradas, pos_entrada_elim * BLOCKSIZE, BLOCKSIZE);

        // Sobrescribimos la entrada a eliminar con la última entrada
        buffer_entradas[p_entrada % (BLOCKSIZE / sizeof(struct entrada))] = ultima_entrada;
        mi_write_f(p_inodo_dir, buffer_entradas, pos_entrada_elim * BLOCKSIZE, BLOCKSIZE);
    }

    // si la entrada a eliminar es la ultima truncamos el inodo a su tamaño menos el tamaño de una entrada
    int nbytes = inodo_dir.tamEnBytesLog - sizeof(struct entrada);
    if (mi_truncar_f(p_inodo_dir, nbytes) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    //Decrementar el número de enlaces de p_inodo
    inodo.nlinks--;

    //Si no quedan enlaces, entonces liberamos el inodo
    if (inodo.nlinks == 0) {
        if (liberar_inodo(p_inodo) == FALLO) return FALLO;
    } else {  // en caso contrario actualizamos su ctime y escribimos el inodo.
        inodo.ctime = time(NULL);
        if (escribir_inodo(p_inodo, &inodo) == FALLO) return FALLO;
    }

    mi_signalSem();

    return EXITO;
}

/*  
 *  Renombra un fichero o directorio
 *  @param camino_antiguo ruta del fichero / directorio que queremso renombrar
 *  @param camino_nuevo nombre nuevo que le queremos poner
 *  @return 
 */
int mi_rn(const char *camino_antiguo, const char *nuevo) {
    mi_waitSem();
    
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int err;

    // copiamos el nombre nuevo en la variable
    char nombre_nuevo[TAMNOMBRE];
    memset(nombre_nuevo, 0, TAMNOMBRE);
    strcpy(nombre_nuevo, nuevo);
    
    // buscamso el antiguo 
    if ((err = buscar_entrada(camino_antiguo, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0)) < 0) {
        mi_signalSem();
        return err; 
    }
    
    // leemos el directorio padre del que queremos cambiar 
    inodo_t inodo_dir;
    if (leer_inodo(p_inodo_dir, &inodo_dir) < 0) {
        mi_signalSem();
        return FALLO;
    }

    // comprobamos que no exista ya /ruta/nuevo
    int cant_entradas = inodo_dir.tamEnBytesLog / sizeof(struct entrada);
    struct entrada entrada_act;

    // vamos leyendo las entradas una por una 
    for (int i = 0; i < cant_entradas; i++) {
        if (mi_read_f(p_inodo_dir, &entrada_act, i * sizeof(struct entrada), sizeof(struct entrada)) < 0) return FALLO;
        if (strcmp(entrada_act.nombre, nombre_nuevo) == 0) { 
            mi_signalSem();
            return ERROR_ENTRADA_YA_EXISTENTE; // mostrar_error_buscar_entrada() - El archivo ya existe
        }
    }
    
    // leemos la entrada original 
    if (mi_read_f(p_inodo_dir, &entrada_act, p_entrada * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
        mi_signalSem();
        return FALLO;
    }

    // cambiamos el nombre por el nuevo 
    strcpy(entrada_act.nombre, nombre_nuevo);

    // Escribimos la entrada actualizada en su misma posición
    if (mi_write_f(p_inodo_dir, &entrada_act, p_entrada * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
        mi_signalSem();
        return FALLO;
    }

    mi_signalSem();

    return EXITO;
}

/*  
 *  Borra recursivamente el contenido de un directorio y el propio directorio
 *  @param ruta ruta del directorio que queremso vaciar y borrar
 *  @return 
 */
int mi_rm_r(const char *ruta) {
    mi_waitSem();

    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int err;

    // buscamos la ruta actual 
    if ((err = buscar_entrada(ruta, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0)) < 0) {
        mi_signalSem();
        return err;
    }

    inodo_t inodo;
    if (leer_inodo(p_inodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    // si es un fichero lo borramos
    if (inodo.tipo == 'f') {
        int res = mi_unlink(ruta);
        mi_signalSem();
        return res;
    }

    // si es un directorio leemos sus entradas 
    else if (inodo.tipo == 'd') {
        int cant_entradas = inodo.tamEnBytesLog / sizeof(struct entrada);
        
        if (cant_entradas > 0) {
            struct entrada buffer_entradas[BLOCKSIZE / sizeof(struct entrada)];
            memset(buffer_entradas, 0, BLOCKSIZE);
            
            int entradas_leidas = 0;
            while (entradas_leidas < cant_entradas) {
                // leemos el directorio 
                if (mi_read_f(p_inodo, buffer_entradas, entradas_leidas * sizeof(struct entrada), BLOCKSIZE) == FALLO) {
                    mi_signalSem();
                    return FALLO;
                }
                
                // si las entradas que nos quedan por leer son mas de un bloque entero leemos el bloque enteor si no solo leemos las entradas que nos quedan (que no ocupan un bloque completo)
                int limite = (cant_entradas - entradas_leidas < (BLOCKSIZE / sizeof(struct entrada))) ? 
                             (cant_entradas - entradas_leidas) : (BLOCKSIZE / sizeof(struct entrada));
                
                for (int i = 0; i < limite; i++) {
                    char nueva_ruta[TAMNOMBRE * PROFUNDIDAD]; // tamaño máximo de ruta
                    // ruta nueva = "ruta antigua + entrada"
                    strcpy(nueva_ruta, ruta);
                    strcat(nueva_ruta, buffer_entradas[i].nombre);
                    
                    // Si es un subdirectorio, añadimos la barra final
                    inodo_t sub_inodo;
                    leer_inodo(buffer_entradas[i].ninodo, &sub_inodo);
                    if (sub_inodo.tipo == 'd') strcat(nueva_ruta, "/");
                    
                    // Llamada recursiva
                    mi_rm_r(nueva_ruta);
                }

                entradas_leidas += limite;
            }
        }
        // Una vez vacío el directorio (no tiene mas entradas) borramos el directorio actual
        int err = mi_unlink(ruta);
        if (err < 0) {
            mostrar_error_buscar_entrada(err);
            mi_signalSem();
            return FALLO;
        }

        mi_signalSem();
        return err;
    }

    mi_signalSem();

    return FALLO;
}