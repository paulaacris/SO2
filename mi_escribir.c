/**
 * @file mi_escribir.c
 * @author Paula Cristina Morro Negre
 * @author Maria Teresa Sbert Gomila
 *
 * Sintaxis: ./mi_escribir <disco> </ruta_fichero> <texto> <offset>
 * Escribe texto en un fichero desde el offset indicado.
 */

#include "directorios.h"

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, RED "Sintaxis: ./mi_escribir <nombre_dispositivo> </ruta_fichero> <texto> <offset>\n" RESET);
        return FALLO;
    }

    const char *disco = argv[1];
    const char *ruta = argv[2];
    const char *texto = argv[3];
    int offset = atoi(argv[4]);

    if (offset < 0) {
        fprintf(stderr, RED "Error: El offset debe ser un valor no negativo.\n" RESET);
        return FALLO;
    }

    int nbytes = strlen(texto);

    if (bmount(disco) == FALLO) {
        fprintf(stderr, RED "Error: no se pudo montar el dispositivo '%s'\n" RESET, disco);
        return FALLO;
    }

    // llamamos a mi_write para escribir el contenido en el fichero
    int error = mi_write(ruta, contenido, 0, strlen(contenido));
    if (error != EXITO) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }

    // desmontamos el dispositivo
    if (bumount() == FALLO) return FALLO;
    return EXI

int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    // leemos el SB para saber donde esta el inodo raiz 
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

    unsigned int p_inodo_dir = SB.posInodoRaiz, p_inodo = 0, p_entrada = 0;
    
    // llamar a buscar_entrada para encontrar el inodo del fichero/directorio
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0); // con 0 porque solo queremos mirar que exista, no crearlo

    // si buscar_entrada devuelve algo que no sea EXITO devolvemos el error
    if (error != EXITO) return error;

    // si la entrada existe llamamos a mi_write_f
    return mi_write_f(p_inodo, buf, offset, nbytes); // devolvemos lo que devuelva mi_write_f

}

