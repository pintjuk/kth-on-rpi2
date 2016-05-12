#ifndef _SHARED_ALLOC_H_    
#define _SHARED_ALLOC_H_

#include <lib.h>
#include <types.h>

void* sm_alloc(size_t size);
void sm_free(void* ptr);
void print_heap(); 
#endif
