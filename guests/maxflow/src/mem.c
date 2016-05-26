/*
 * mem.c
 *
 *  Created on: Mar 13, 2013
 *      Author: viktordo
 */

/*
 * THE HEAP
 */
#include <memlib.h>
#include <lib.h>
#include "maxflow.h"

extern addr_t __heap_start__, __heap_end__, __shared_heap_start__, __shared_heap_end__, __shared_heap_lock__;
 
static heap_ctxt_t heap;

void free(void *adr){

	heap_free(&heap, adr);
}

void *malloc(uint32_t size){

	return heap_alloc(&heap, size);

}

void *calloc(uint32_t num, uint32_t size){
	/*Unefficient calloc*/

	void *pointer;
	pointer = malloc(num*size);
	memset(pointer,'\0',num*size);
	return pointer;

}

void init_heap(){
    printf("heap cntxt size %i\n", sizeof(heap_ctxt_t));
    addr_t start = &__heap_start__;
    addr_t end   = &__heap_end__;
    printf("heap start %x, heap end %x\n", start, end);
    printf("sheap %x, %x, %x \n", &__shared_heap_start__, &__shared_heap_end__, &__shared_heap_lock__);
    heap_init( &heap, end - start - 1, (void *)start);
}

/*
 * THE SHARED HEAP
 */

extern heap_ctxt_t __shared_heap_ctx__;

void s_free(void *adr){
    lock(&__shared_heap_lock__);
	
    heap_free(&__shared_heap_ctx__, adr);
    
    unlock(&__shared_heap_lock__);
}

void *s_malloc(uint32_t size){

    lock(&__shared_heap_lock__);
    void * result  = heap_alloc(&__shared_heap_ctx__, size);

    unlock(&__shared_heap_lock__);
    return result;
}

void *s_calloc(uint32_t num, uint32_t size){
	/*Unefficient calloc*/

    lock(&__shared_heap_lock__);
	void *pointer;
	pointer = s_malloc(num*size);
	memset(pointer,'\0',num*size);
    unlock(&__shared_heap_lock__);
    return pointer;
}

void s_init_heap(){
    lock(&__shared_heap_lock__);
    addr_t start = &__shared_heap_start__;
    addr_t end   = &__shared_heap_end__;
    heap_init( &__shared_heap_ctx__, end - start - 1, (void *)start);
    unlock(&__shared_heap_lock__);
}
