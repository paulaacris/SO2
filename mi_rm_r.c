/**
 * @file directorios.c
 * @author Paula Cristina Morro Negre 
 * @author Maria Teresa Sbert Gomila
 * 
 * Borrado recursivo
*/

#include "directorios.h"

// función auxiliar para vaciar el directorio
int borrar_recursivo(const char *ruta) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int err;
    inodo_t inodo;

    // buscamos la ruta actual 
    if ((err = buscar_entrada(ruta, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0)) < 0) return err;

    if (leer_inodo(p_inodo, &inodo) == FALLO) return FALLO;

    // si es un fichero lo borramos
    if (inodo.tipo == 'f') {
        return mi_unlink(ruta);
    }

    // si es un directorio leemos sus entradas 
    else if (inodo.tipo == 'd') {
        int cant_entradas = inodo.tamEnBytesLog / sizeof(struct entrada);
        
        if (cant_entradas > 0) {
            struct entrada buffer_entradas[BLOCKSIZE / sizeof(struct entrada)];
            memset(buffer_entradas, 0, BLOCKSIZE);
            
            int entradas_leidas = 0;
            while (entradas_leidas < cant_entradas) {
                if (mi_read_f(p_inodo, buffer_entradas, entradas_leidas * sizeof(struct entrada), BLOCKSIZE) == FALLO) return FALLO;
                
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
                    borrar_recursivo(nueva_ruta);
                }

                entradas_leidas += limite;
            }
        }
        // Una vez vacío el directorio (no tiene mas entradas) borramos el directorio actual
        int err = mi_unlink(ruta);
        if (err < 0) {
            mostrar_error_buscar_entrada(err);
            bumount();
            return FALLO;
        }

        return err;
    }

    return FALLO;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta_directorio>\n", argv[0]);
        return -1;
    }

    const char *disco = argv[1];
    const char *ruta = argv[2];

    // comprobar que la ruta sea la de un fichero
    if (ruta[strlen(ruta) - 1] != '/') {
        fprintf(stderr, "Error: La ruta especificada es un fichero. Utiliza mi_rm para ficheros.\n");
        return FALLO;
    }

    if (bmount(disco) == FALLO) return FALLO;

    borrar_recursivo(ruta);

    if (bumount() == FALLO) return FALLO;
    
    return EXITO;
}