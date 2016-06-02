#ifndef _SHARED_ALLOC_H_    
#define _SHARED_ALLOC_H_

#include <lib.h>
#include <types.h>


struct sm_heap_ctxt_t
{
    uint32_t* start;
    uint32_t* end;
    uint32_t* inited;
};

void sm_init_heap(struct sm_heap_ctxt_t* heap_ctxt, uint32_t* start, uint32_t* end);
void* sm_alloc(struct sm_heap_ctxt_t* heap_ctxt, size_t size);
void sm_free(struct sm_heap_ctxt_t* heap_ctxt, void* ptr);
void sm_print_heap(struct sm_heap_ctxt_t* heap_ctxt); 
#endif
