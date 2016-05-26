#ifndef __BUFFER_H__
#define __BUFFER_H__
#include <lib.h>
#include <types.h>
#define true (1)
#define false (0)

   struct Queue 
    {
        int     capacity;
        size_t  element_size;
        void*   internals;
        BOOL    (*Enqueue)  (struct Queue* self, void*);
        /*************************************
         * removes element on the head of the queue 
         * and retorns pointer to a copy,
         * caller responsabel to free memory
         *************************************/
        void*   (*Dequeue)  (struct Queue* self);
        void*   (*Peak)     (struct Queue* self);
        uint32_t   (*Size)     (struct Queue* self);
    };


/*************  nonconcurrent ring buffer ******************************/
    struct Queue new_nonconcurrent_ring_buffer (int capacity, size_t element_size);
    void free_nonconcurrent_ring_buffer (struct Queue * queue);

/************* locked ring buffer *************************************/
    struct Queue new_locked_ring_buffer (int capacity, size_t element_size);
    void free_locked_ring_buffer (struct Queue * queue);
#endif
