#include "directorios.h"
#include <sys/wait.h>
#include <signal.h>

#define NUMPROCESOS 100
#define NUMESCRITURAS 50
#define REGMAX 500000

struct REGISTRO { 
    time_t fecha; // Precisión segundos
    pid_t pid; // PID del proceso que lo ha creado
    int nEscritura; // Entero con el nº de escritura (de 1 a 50)
    int nRegistro; // Entero con el nº del registro dentro del fichero
};