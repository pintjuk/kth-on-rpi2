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

extern addr_t __heap_start__, __heap_end__, __shared_heap_start__, __shared_heap_end__;
 
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
    addr_t start = &__heap_start__;
    addr_t end   = &__heap_end__;
    heap_init( &heap, end - start - 1, (void *)start);
}

/*
 * THE SHARED HEAP
 */

static heap_ctxt_t sheap shared;
static uint32_t sheap_lock shared;


void s_free(void *adr){
    lock(&sheap_lock);
	
    heap_free(&sheap, adr);
    
    unlock(&sheap_lock);
}

void *s_malloc(uint32_t size){

    lock(&sheap_lock);
    void * result  = heap_alloc(&sheap, size);

    unlock(&sheap_lock);
    return result;
}

void *s_calloc(uint32_t num, uint32_t size){
	/*Unefficient calloc*/
    void *pointer;
    pointer = s_malloc(num*size);
    memset(pointer,'\0',num*size);
    return pointer;
}

void s_init_heap(){
    lock(&sheap_lock);
    addr_t start = &__shared_heap_start__;
    addr_t end   = &__shared_heap_end__;
    heap_init( &sheap, end - start - 1, (void *)start);
    unlock(&sheap_lock);
}
