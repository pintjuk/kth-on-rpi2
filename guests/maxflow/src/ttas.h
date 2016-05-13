#ifndef __TTAS_H__
#define __TTAS_H__
 #include <lib.h>
#include <types.h>

   

void ttas_lock(uint32_t* lock);
void ttas_unlock(uint32_t* lock);

#endif
