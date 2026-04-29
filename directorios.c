/**
 * @file directorios.c
 * @author Paula Cristina Morro Negre 
 * @author Maria Teresa Sbert Gomila
*/

#include "directorios.h"

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

    // leemos el SB
    if (bread(posSB, &SB) == FALLO) return FALLO;
    
    // si camnino principal es / (directorio raiz)
    if (strcmp(camino_parcial, "/") == 0) { 
        *p_inodo = SB.posInodoRaiz; // nuestra raiz siempre estará asociada al inodo 0
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
    // leemos el SB para saber donde esta el inodo raiz 
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;
    
    unsigned int p_inodo_dir = SB.posInodoRaiz, p_inodo = 0, p_entrada = 0;
    
    // buscar_entrada() con reservar=1 para que cree el directorio si no existe 
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);

    // esta función devuelve lo que devuelva buscar_entrada
    return error;
}

/*
 *  
 *  @param camino cambino del fichero/directorio
 *  @param buffer
 *  @param tipo 
 *  @param flag
 *  @return 
 */
int mi_dir(const char *camino, char *buffer, char tipo, char flag) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;
    unsigned int p_inodo_dir = SB.posInodoRaiz, p_inodo, p_entrada;
    int err;

    if ((err = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0)) < 0)
        return err;

    inodo_t inodo_dir, inodo;
    if (leer_inodo(p_inodo, &inodo_dir) == FALLO)
        return FALLO;

    if (inodo_dir.tipo != tipo) {
        fprintf(stderr, RED "Error: la sintaxis no concuerda con el tipo\n" RESET);
        return FALLO;
    }

    if (tipo == 'f') {
        // si es un fichero, leer el inodo del directorio que lo contiene
        if (leer_inodo(p_inodo_dir, &inodo_dir) == FALLO)
            return FALLO;
    } else {
        p_inodo_dir = p_inodo;
    }

    struct entrada buffer_entradas[BLOCKSIZE / sizeof(struct entrada)];
    memset(buffer_entradas, 0, BLOCKSIZE);
    struct entrada entrada = buffer_entradas[0];

    int cant_entradas_inodo, num_entrada_inodo;
    if (tipo == 'd') {
        cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);
        num_entrada_inodo = 0;
    } else {
        cant_entradas_inodo = p_entrada + 1;
        num_entrada_inodo = p_entrada;
    }

    for (; num_entrada_inodo < cant_entradas_inodo; num_entrada_inodo++) {
        if (tipo == 'd') {
            if (num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada)) == 0) {
                memset(buffer_entradas, 0, BLOCKSIZE);
                if (mi_read_f(p_inodo_dir, buffer_entradas, num_entrada_inodo * sizeof(struct entrada), BLOCKSIZE) < 0)
                    return FALLO;
            }
            entrada = buffer_entradas[num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada))];  // leer siguiente entrada
        } else {
            memset(buffer_entradas, 0, BLOCKSIZE);
            int num_bloque = num_entrada_inodo * sizeof(struct entrada) / BLOCKSIZE;
            if (mi_read_f(p_inodo_dir, buffer_entradas, num_bloque * BLOCKSIZE, BLOCKSIZE) < 0)
                return FALLO;
            entrada = buffer_entradas[num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada))];  // leer siguiente entrada
        }

        if (leer_inodo(entrada.ninodo, &inodo) == FALLO)
            return FALLO;

        if (flag == 'l') {
            char tmp[BLOCKSIZE]; 
            // Importante: No uses strcat directamente sin asegurar que tmp está limpio y empieza de cero
            sprintf(tmp, "%c\t", inodo.tipo); // USAR inodo.tipo, NO tipo
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
            sprintf(tmp, "%d\t", inodo.tamEnBytesLog);
            strcat(buffer, tmp);
        }

        // asignar colores al nombre de la entrada en función del tipo
        if (inodo.tipo == 'd')  // es un directorio
            strcat(buffer, BLUE);
        else if (inodo.permisos & 1)  // es un fichero ejecutable
            strcat(buffer, GREEN);

        strcat(buffer, entrada.nombre);
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
    // leemos el SB para saber donde esta el inodo raiz 
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

    unsigned int p_inodo_dir = SB.posInodoRaiz, p_inodo = 0, p_entrada = 0;
    
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
    // leemos el SB para saber donde esta el inodo raiz 
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

    unsigned int p_inodo_dir = SB.posInodoRaiz, p_inodo = 0, p_entrada = 0;
    
    // llamar a buscar_entrada para encontrar el inodo del fichero/directorio
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0); // con 0 porque solo queremos mirar que exista, no crearlo

    // si buscar_entrada devuelve algo que no sea EXITO devolvemos el error
    if (error != EXITO) return error;

    // si la entrada existe llamamos a mi_stat_f
    return mi_stat_f(p_inodo, p_stat); // devolvemos lo que devuelva mi_stst_f
}

// ============================================================
// CACHÉ DE DIRECTORIOS
// 0: sin caché
// 1: última L/E (1 entrada para escritura + 1 para lectura)
// 2: tabla FIFO  (CACHE_SIZE entradas para E + CACHE_SIZE para L)
// 3: tabla LRU   (ídem con sello de tiempo)
// ============================================================
#define USARCACHE 2
 
#if USARCACHE
struct UltimaEntrada {
    char camino[TAMNOMBRE * PROFUNDIDAD];
    int  p_inodo;
    #if USARCACHE == 3
    struct timeval ultima_consulta;
    #endif
};
 
#if USARCACHE == 1
static struct UltimaEntrada UltimaEntradaEscritura;
static struct UltimaEntrada UltimaEntradaLectura;
#else
#define CACHE_SIZE 3
static struct UltimaEntrada UltimasEntradasEscritura[CACHE_SIZE];
static struct UltimaEntrada UltimasEntradasLectura[CACHE_SIZE];
#endif
#endif
 
// Para FIFO: índice circular donde se insertará la PRÓXIMA entrada.
// Se inicializa a CACHE_SIZE-1 para que el primer (idx+1)%CACHE_SIZE == 0,
// haciendo que la primera inserción vaya a cache[0].
#if USARCACHE == 2
static int IdxInsercionEscritura = CACHE_SIZE - 1;
static int IdxInsercionLectura   = CACHE_SIZE - 1;
static int EntradasEscritura     = 0; // cuántas posiciones están ocupadas
static int EntradasLectura       = 0;
#endif

// ============================================================
// NIVEL 9 — mi_write
//
// Escribe nbytes de buf en el fichero indicado por camino
// a partir de offset. Usa caché de directorios para evitar
// llamar a buscar_entrada() si el camino ya está cacheado.
// Devuelve los bytes escritos o un código de error negativo.
// ============================================================
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
 
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO; 
 
    unsigned int p_inodo_dir = SB.posInodoRaiz, p_inodo = 0, p_entrada = 0;
    int cache_hit = 0;
 
// ---------- USARCACHE == 1: una sola entrada para escritura ----------
#if USARCACHE == 1
    if (strcmp(UltimaEntradaEscritura.camino, camino) == 0) {
        p_inodo   = UltimaEntradaEscritura.p_inodo;
        cache_hit = 1;
        #if DEBUG_mi_write
            fprintf(stderr, CYAN "[mi_write() -> Utilizamos la caché de escritura en vez de llamar a buscar_entrada()]\n" RESET);
        #endif
    }
 
// ---------- USARCACHE == 2: tabla FIFO ----------
#elif USARCACHE == 2
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (strcmp(UltimasEntradasEscritura[i].camino, camino) == 0) {
            p_inodo   = UltimasEntradasEscritura[i].p_inodo;
            cache_hit = 1;
            #if DEBUG_mi_write
                fprintf(stderr, CYAN "[mi_write() -> Utilizamos la caché de escritura en vez de llamar a buscar_entrada()]\n" RESET);
            #endif
            break;
        }
    }
 
// ---------- USARCACHE == 3: tabla LRU ----------
#elif USARCACHE == 3
    {
        int lru_idx = -1;
        struct timeval oldest = {LONG_MAX, LONG_MAX};
        for (int i = 0; i < CACHE_SIZE; i++) {
            if (strcmp(UltimasEntradasEscritura[i].camino, camino) == 0) {
                p_inodo   = UltimasEntradasEscritura[i].p_inodo;
                cache_hit = 1;
                // Actualizamos el sello de tiempo del hit
                gettimeofday(&UltimasEntradasEscritura[i].ultima_consulta, NULL);
                #if DEBUG_mi_write
                    fprintf(stderr, CYAN "[mi_write() -> Utilizamos la caché de escritura en vez de llamar a buscar_entrada()]\n" RESET);
                #endif
                break;
            }
            // Buscamos el candidato LRU (más antiguo o vacío)
            if (UltimasEntradasEscritura[i].camino[0] == '\0' ||
                timercmp(&UltimasEntradasEscritura[i].ultima_consulta, &oldest, <)) {
                oldest  = UltimasEntradasEscritura[i].ultima_consulta;
                lru_idx = i;
            }
        }
        if (!cache_hit) {
            // Guardamos el índice LRU para usarlo tras buscar_entrada
            // (se usará en el bloque de actualización de caché de abajo)
            // Almacenamos temporalmente en p_entrada el índice
            p_entrada = (unsigned int)lru_idx;
        }
    }
#endif
 
    // ---- Miss: llamar a buscar_entrada y actualizar la caché ----
    if (!cache_hit) {
        int err = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (err < 0) return err; 
 
#if USARCACHE == 1
        strcpy(UltimaEntradaEscritura.camino, camino);
        UltimaEntradaEscritura.p_inodo = p_inodo;
        #if DEBUG_mi_write
            fprintf(stderr, YELLOW "[mi_write() -> Actualizamos la caché de escritura]\n" RESET);
        #endif
 
#elif USARCACHE == 2
        // Inserción circular FIFO: avanzamos el índice ANTES de insertar
        IdxInsercionEscritura = (IdxInsercionEscritura + 1) % CACHE_SIZE;
        strcpy(UltimasEntradasEscritura[IdxInsercionEscritura].camino, camino);
        UltimasEntradasEscritura[IdxInsercionEscritura].p_inodo = p_inodo;
        if (EntradasEscritura < CACHE_SIZE) EntradasEscritura++;
        #if DEBUG_mi_write
            fprintf(stderr, YELLOW "[mi_write() -> Actualizamos la caché de escritura]\n" RESET);
        #endif
 
#elif USARCACHE == 3
        {
            // Buscamos el índice LRU (vacío o con el timestamp más antiguo)
            int lru_idx = 0;
            struct timeval oldest = {LONG_MAX, LONG_MAX};
            for (int i = 0; i < CACHE_SIZE; i++) {
                if (UltimasEntradasEscritura[i].camino[0] == '\0') {
                    lru_idx = i;
                    break;
                }
                if (timercmp(&UltimasEntradasEscritura[i].ultima_consulta, &oldest, <)) {
                    oldest  = UltimasEntradasEscritura[i].ultima_consulta;
                    lru_idx = i;
                }
            }
            strcpy(UltimasEntradasEscritura[lru_idx].camino, camino);
            UltimasEntradasEscritura[lru_idx].p_inodo = p_inodo;
            gettimeofday(&UltimasEntradasEscritura[lru_idx].ultima_consulta, NULL);
            #if DEBUG_mi_write
                fprintf(stderr, YELLOW "[mi_write() -> Actualizamos la caché de escritura]\n" RESET);
            #endif
        }
#endif
    }
 
    int escritos = mi_write_f(p_inodo, buf, offset, nbytes);
    return escritos; // FALLO (-1) o nº de bytes escritos
}
 
// ============================================================
// NIVEL 9 — mi_read
//
// Lee nbytes del fichero indicado por camino a partir de offset
// y los copia en buf. Usa caché de directorios análoga a mi_write.
// Devuelve los bytes leídos o un código de error negativo.
// ============================================================
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes) {
 
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;
 
    unsigned int p_inodo_dir = SB.posInodoRaiz, p_inodo = 0, p_entrada = 0;
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
                fprintf(stderr, CYAN "\n[mi_read() -> Utilizamos la caché de lectura en vez de llamar a buscar_entrada()]\n" RESET);
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
                fprintf(stderr, CYAN "\n[mi_read() -> Utilizamos la caché de lectura en vez de llamar a buscar_entrada()]\n" RESET);
            #endif
            break;
        }
    }
#endif
 
    // ---- Miss: llamar a buscar_entrada y actualizar la caché ----
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
            fprintf(stderr, YELLOW "[mi_read() -> Actualizamos la caché de lectura]\n" RESET);
        #endif
 
#elif USARCACHE == 3
        {
            int lru_idx = 0;
            struct timeval oldest = {LONG_MAX, LONG_MAX};
            for (int i = 0; i < CACHE_SIZE; i++) {
                if (UltimasEntradasLectura[i].camino[0] == '\0') {
                    lru_idx = i;
                    break;
                }
                if (timercmp(&UltimasEntradasLectura[i].ultima_consulta, &oldest, <)) {
                    oldest  = UltimasEntradasLectura[i].ultima_consulta;
                    lru_idx = i;
                }
            }
            strcpy(UltimasEntradasLectura[lru_idx].camino, camino);
            UltimasEntradasLectura[lru_idx].p_inodo = p_inodo;
            gettimeofday(&UltimasEntradasLectura[lru_idx].ultima_consulta, NULL);
            #if DEBUG_mi_read
                fprintf(stderr, YELLOW "[mi_read() -> Actualizamos la caché de lectura]\n" RESET);
            #endif
        }
#elif !USARCACHE
        // sin caché: nada que hacer
        (void)0;
#endif
    }
 
    return mi_read_f(p_inodo, buf, offset, nbytes);
}