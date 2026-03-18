<<<<<<< HEAD
#include "ficheros_basico.h"

#define PERMISO_ESCRITURA 2
#define PERMISO_LECTURA 4

int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes);
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes);
//int mi_stat_f(unsigned int ninodo, struct STAT *p_stat);
int mi_chmod_f(unsigned int ninodo, unsigned char permisos);
=======
#include "bloques.h"

#include <time.h>   //para los sellos de tiempo
#include <stdio.h>
>>>>>>> 2f1049cfae7fe8c4185ca7878fc364e51301e0e2
