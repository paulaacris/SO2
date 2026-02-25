#include "ficheros_basico.h"

#define NIVEL2 0

int main(int argc, char **argv) {

    if(argc != 2){
        perror(RED"Error de sintaxis. \nUso: ./leer_sf <nombre_dispositivo> <nbloques>\n"); printf(RESET);
        return FALLO;
    }

    char *nombreDispositivo=argv[1]; 
    
    if(bmount(nombreDispositivo) == FALLO) return FALLO;

    struct superbloque SB;
    if(bread(posSB,&SB) == FALLO) return FALLO;
    
    printf("DATOS DEL SUPERBLOQUE\n");
    printf("posPrimerBloqueMB = %d\n", SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB = %d\n", SB.posUltimoBloqueMB);
    printf("posPrimerBloqueAI = %d\n", SB.posPrimerBloqueAI);
    printf("posUltimoBloqueAI = %d\n", SB.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos = %d\n", SB.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos = %d\n", SB.posUltimoBloqueDatos );
    printf("posInodoRaiz = %d\n", SB.posInodoRaiz );
    printf("posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre );
    printf("cantBloquesLibres = %d\n", SB.cantBloquesLibres );
    printf("cantInodosLibres = %d\n", SB.cantInodosLibres );
    printf("totBloques = %d\n", SB.totBloques );
    printf("totInodos = %d\n", SB.totInodos );

    printf ("\nsizeof struct superbloque is: %lu\n", sizeof(struct superbloque));
    printf ("sizeof struct inodo is: %lu\n", sizeof(inodo_t));

    if(NIVEL2){
        printf("\nRECORRIDO LISTA ENLAZADA DE INODOS LIBRES\n");
        
        inodo_t inodos[BLOCKSIZE/INODOSIZE]; // buffer para leer un bloque de inodos (8 inodos)
        int inodoLibre = SB.posPrimerInodoLibre; // empezamos por el primer inodo libre

        // en que bloque fisico está el primero inodo libre
        int bloque = SB.posPrimerBloqueAI + (inodoLibre / (BLOCKSIZE / INODOSIZE));
  
        int encontradoFinal = 0;

        for(int i = bloque; i <= SB.posUltimoBloqueAI && !encontradoFinal; i++){
            // leemos el bloque con los 8 inodos
            if(bread(i,inodos) == FALLO) return FALLO;

            // para cada uno de los 8 inodos de dentro de un bloque
            for(int j = 0; j < (BLOCKSIZE / INODOSIZE); j++){
                // imprimimos el pintero al siguiente
                printf("%d - ", inodos[j].punterosDirectos[0]);
                
                // si es el ultimo inodo (total no del bloque) acabamos 
                if(inodos[j].punterosDirectos[0] == UINT_MAX){
                    encontradoFinal = 1;
                    break;
                }
            }
        }
    }

    if(bumount() == FALLO) return FALLO;
    
    return EXITO;
}