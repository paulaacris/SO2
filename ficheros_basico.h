#include "bloques.h"

#include <time.h>   //para los sellos de tiempo
#include <stdio.h>
#include <limits.h> // para usar UINT_MAX

#define posSB 0 // el superbloque se escribe en el primer bloque de nuestro FS
#define tamSB 1
#define INODOSIZE 128 // tamaño en bytes de un inodo

struct superbloque {
    unsigned int posPrimerBloqueMB;          // Posición absoluta del primer bloque del mapa de bits
    unsigned int posUltimoBloqueMB;          // Posición absoluta del último bloque del mapa de bits
    unsigned int posPrimerBloqueAI;            // Posición absoluta del primer bloque del array de inodos
    unsigned int posUltimoBloqueAI;            // Posición absoluta del último bloque del array de inodos
    unsigned int posPrimerBloqueDatos;     // Posición absoluta del primer bloque de datos
    unsigned int posUltimoBloqueDatos;     // Posición absoluta del último bloque de datos
    unsigned int posInodoRaiz;                     // Posición del inodo del directorio raíz (relativa al AI)
    unsigned int posPrimerInodoLibre;        // Posición del primer inodo libre (relativa al AI)
    unsigned int cantBloquesLibres;            // Cantidad de bloques libres (en todo el disco)
    unsigned int cantInodosLibres;              // Cantidad de inodos libres (en el AI)
    unsigned int totBloques;                          // Cantidad total de bloques del disco
    unsigned int totInodos;                            // Cantidad total de inodos (heurística)
    char padding[BLOCKSIZE - 12 * sizeof(unsigned int)]; // Relleno para ocupar el bloque completo
};

typedef union _inodo {
    struct {
        unsigned char tipo;     // Tipo ('l':libre, 'd':directorio o 'f':fichero)
        unsigned char permisos; // Permisos (lectura y/o escritura y/o ejecución)
        
        time_t atime; // Fecha y hora del último acceso a datos: atime
        time_t mtime; // Fecha y hora de la última modificación de datos: mtime
        time_t ctime; // Fecha y hora de la última modificación del inodo: ctime
        time_t btime; // Fecha y hora de creación del inodo: btime (birth)
    
        unsigned int nlinks;             // Cantidad de enlaces de entradas en directorio
        unsigned int tamEnBytesLog;      // Tamaño en bytes lógicos
        unsigned int numBloquesOcupados; // Cantidad de bloques ocupados zona de datos
        unsigned int punterosDirectos[12];
        unsigned int punterosIndirectos[3];
    };
    char padding[INODOSIZE];
} inodo_t;

int tamMB (unsigned int nbloques);
int tamAI(unsigned int ninodos);
int initSB(unsigned int nbloques, unsigned int ninodos);
int initMB(); 
int initAI();