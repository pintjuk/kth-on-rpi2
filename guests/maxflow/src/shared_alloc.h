#ifndef _SHARED_ALLOC_H_    
#define _SHARED_ALLOC_H_

#include <lib.h>
#include <types.h>
void sm_init_heap();
void* sm_alloc(size_t size);
void sm_free(void* ptr);
void sm_print_heap(); 
#endif
