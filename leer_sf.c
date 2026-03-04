#include "ficheros_basico.h"

#define NIVEL2 0
#define NIVEL3 1

int main(int argc, char **argv) {

    if(argc != 2){
        perror(RED"Error de sintaxis. \nUso: ./leer_sf <nombre_dispositivo> <nbloques>\n"); printf(RESET);
        return FALLO;
    }

    char *nombreDispositivo=argv[1]; 
    
    if(bmount(nombreDispositivo) == FALLO) return FALLO;

    struct superbloque SB;
    if(bread(posSB,&SB) == FALLO) return FALLO;
    
    printf(ROSE"DATOS DEL SUPERBLOQUE\n"RESET);
    printf("posPrimerBloqueMB = %d\n", SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB = %d\n", SB.posUltimoBloqueMB);
    printf("posPrimerBloqueAI = %d\n", SB.posPrimerBloqueAI);
    printf("posUltimoBloqueAI = %d\n", SB.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos = %d\n", SB.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos = %d\n", SB.posUltimoBloqueDatos );
    printf("posInodoRaiz = %d\n", SB.posInodoRaiz );
    printf(CYAN"posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre );
    printf("cantBloquesLibres = %d\n", SB.cantBloquesLibres );
    printf("cantInodosLibres = %d\n"RESET, SB.cantInodosLibres );
    printf("totBloques = %d\n", SB.totBloques );
    printf("totInodos = %d\n", SB.totInodos );

    printf ("\nsizeof struct superbloque is: %lu\n", sizeof(struct superbloque));
    printf ("sizeof struct inodo is: %lu\n", sizeof(inodo_t));

    # if NIVEL2
        printf(ROSE"\nRECORRIDO LISTA ENLAZADA DE INODOS LIBRES\n"RESET);
        
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
    # endif

    # if NIVEL3
        printf(ROSE"\nRESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n"RESET);
        int bloqueReservado = reservar_bloque();
        printf("Se ha reservado el bloque físico nº %d, que era el 1º libre indicado por el MB\n", bloqueReservado);
        if(bread(posSB,&SB)==FALLO) return FALLO;
        printf("SB.cantBloquesLibres = %d\n", SB.cantBloquesLibres);
        liberar_bloque(bloqueReservado);
        if(bread(posSB,&SB)==FALLO) return FALLO;
        printf("Liberamos ese bloque y después SB.cantBloquesLibres = %d\n", SB.cantBloquesLibres);

        printf(ROSE"\nMAPA DE BITS CON BLOQUES DE METADATOS OCUPADOS\n"RESET);
        printf("posSB: %d -> leer_bit(%d) = %d\n", posSB, posSB, leer_bit(posSB));
        printf("SB.posPrimerBloqueMB: %d -> leer_bit(%d) = %d\n", SB.posPrimerBloqueMB,SB.posPrimerBloqueMB, leer_bit(SB.posPrimerBloqueMB));
        printf("SB.posUltimoBloqueMB: %d -> leer_bit(%d) = %d\n", SB.posUltimoBloqueMB,SB.posUltimoBloqueMB, leer_bit(SB.posUltimoBloqueMB));
        printf("SB.posPrimerBloqueAI: %d -> leer_bit(%d) = %d\n", SB.posPrimerBloqueAI,SB.posPrimerBloqueAI, leer_bit(SB.posPrimerBloqueAI));
        printf("SB.posUltimoBloqueAI: %d -> leer_bit(%d) = %d\n", SB.posUltimoBloqueAI,SB.posUltimoBloqueAI, leer_bit(SB.posUltimoBloqueAI));
        printf("SB.posPrimerBloqueDatos: %d -> leer_bit(%d) = %d\n", SB.posPrimerBloqueDatos,SB.posPrimerBloqueDatos, leer_bit(SB.posPrimerBloqueDatos));
        printf("SB.posUltimoBloqueDatos: %d -> leer_bit(%d) = %d\n", SB.posUltimoBloqueDatos,SB.posUltimoBloqueDatos, leer_bit(SB.posUltimoBloqueDatos));
    
        inodo_t inodo;
        if(leer_inodo(SB.posInodoRaiz,&inodo)==FALLO) return FALLO;
        struct tm *ts;
        char atime[80];
        char mtime[80];
        char ctime[80];
        char btime[80];

        ts = localtime(&inodo.atime);
        strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&inodo.mtime);
        strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&inodo.ctime);
        strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&inodo.btime);
        strftime(btime, sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);

        printf(ROSE"\nDATOS DEL DIRECTORIO RAIZ\n"RESET);
        printf("tipo: %c\n", inodo.tipo);
        printf("permisos: %d\n", inodo.permisos);
        printf("atime: %s\n", atime);
        printf("mtime :%s\n", mtime);
        printf("ctime: %s\n", ctime);
        printf("btime: %s\n", btime);
        printf("nlinks: %d\n", inodo.nlinks);
        printf("tamEnBytesLog: %d\n", inodo.tamEnBytesLog);
        printf("numBloquesOcupados:%d\n", inodo.numBloquesOcupados);
    # endif

    if(bumount() == FALLO) return FALLO;
    
    return EXITO;
}