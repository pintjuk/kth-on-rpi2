#include "ttas.h"
#include "maxflow.h"
#include <lib.h>
#include <types.h>

void ttas_lock(uint32_t* state)
{
    for(;;){
        while(*state){}
        if(!TAS(state))
            return;
    }
}

void ttas_unlock(uint32_t* state)
{
    *state=0;
}
