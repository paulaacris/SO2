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
    if ((inodo_dir.permisos & 4) != PERMISO_LECTURA) {
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
                if ((inodo_dir.permisos & 2) != PERMISO_ESCRITURA) return ERROR_PERMISO_ESCRITURA;

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

