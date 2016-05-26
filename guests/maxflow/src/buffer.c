#include "buffer.h"
#include <lib.h>
#include <types.h>


    
/***************    nonconcurrent ring buffer   ***************/

static struct nonconcurrent_ring_buffer
{
    void* data;
    void* start;
    void* end;
    int count;
};

static BOOL Enqueue_nonconcurrent_ring_buffer (struct Queue* self, void* obj)
{
    struct nonconcurrent_ring_buffer* buff = (struct nonconcurrent_ring_buffer *) self->internals;
    if(buff->count==self->capacity)
        return false;

    memcpy(buff->end, obj, self->element_size);
    buff->count++;

    if(buff->end == buff->data+(self->capacity*self->element_size))
    {
        buff->end = buff->data;
    }
    else
    {
        buff->end += self->element_size;
    }
    return true;
}

static void* Dequeu_nonconcurrent_ring_buffer (struct Queue* self)
{
    struct nonconcurrent_ring_buffer* buff = (struct nonconcurrent_ring_buffer*) self->internals;
    if(0==buff->count)
        return NULL;

    void* result=malloc(self->element_size);
    memcpy(result, buff->start, self->element_size);
    buff->count--;
    if(buff->start==buff->data+(self->capacity*self->element_size))
    {
        buff->start=buff->data;
    }
    else
    {
        buff->start+=self->element_size;
    }
    return result;
}

static void* Peak_nonconcurrent_ring_buffer ( struct Queue* self)
{
    struct nonconcurrent_ring_buffer* buff = ( struct nonconcurrent_ring_buffer*) self->internals;
    if(0==buff->count)
        return NULL;

    void* result=malloc(self->element_size);
    memcpy(result, buff->start, self->element_size);
    return result;

}

static uint32_t Size_nonconcurrent_ring_buffer ( struct Queue* self )
{
    struct nonconcurrent_ring_buffer* buff = (struct nonconcurrent_ring_buffer*) self->internals;
    return buff->count;
}

struct Queue new_nonconcurrent_ring_buffer (int capacity, size_t element_size)
{
    struct Queue result;
    result.capacity     = capacity;
    result.element_size = element_size;
    result.internals    = malloc(sizeof(struct nonconcurrent_ring_buffer));
    struct nonconcurrent_ring_buffer* buff =result.internals;
    if(NULL==result.internals)
    {
        /*TODO: handle error or pass on the failiur*/
        result.Dequeue  = NULL;
        result.Enqueue  = NULL;
        result.Peak     = NULL;
        return result;
    }
    buff->data = calloc(capacity, element_size);
    if(NULL==buff->data)
    {
        free(result.internals);
        result.internals= NULL;
        result.Dequeue  = NULL;
        result.Enqueue  = NULL;
        result.Peak     = NULL;
        return result;

    }
    buff -> end = buff -> start = buff -> data;
    buff -> count = 0;
    result.Dequeue  = &Dequeu_nonconcurrent_ring_buffer;
    result.Enqueue  = &Enqueue_nonconcurrent_ring_buffer;
    result.Peak     = &Peak_nonconcurrent_ring_buffer;
    result.Size    = &Size_nonconcurrent_ring_buffer;

    return result;
}

void free_nonconcurrent_ring_buffer ( struct Queue* queue)
{
    free(((struct nonconcurrent_ring_buffer*)(queue->internals))->data);
    free(queue->internals);
}


/*********************************************************************************************/
static struct locked_ring_buffer
{
    uint32_t lock;
    void* data;
    void* start;
    void* end;
    int count;
};

static BOOL Enqueue_locked_ring_buffer (struct Queue* self, void* obj)
{
    struct nonconcurrent_ring_buffer* buff = (struct nonconcurrent_ring_buffer *) self->internals;
    if(buff->count==self->capacity)
        return false;

    memcpy(buff->end, obj, self->element_size);
    buff->count++;

    if(buff->end == buff->data+(self->capacity*self->element_size))
    {
        buff->end = buff->data;
    }
    else
    {
        buff->end += self->element_size;
    }
    return true;
}

static void* Dequeu_locked_ring_buffer (struct Queue* self)
{
    struct nonconcurrent_ring_buffer* buff = (struct nonconcurrent_ring_buffer*) self->internals;
    if(0==buff->count)
        return NULL;

    void* result=malloc(self->element_size);
    memcpy(result, buff->start, self->element_size);
    buff->count--;
    if(buff->start==buff->data+(self->capacity*self->element_size))
    {
        buff->start=buff->data;
    }
    else
    {
        buff->start+=self->element_size;
    }
    return result;
}

static void* Peak_locked_ring_buffer ( struct Queue* self)
{
    struct nonconcurrent_ring_buffer* buff = ( struct nonconcurrent_ring_buffer*) self->internals;
    if(0==buff->count)
        return NULL;

    void* result=malloc(self->element_size);
    memcpy(result, buff->start, self->element_size);
    return result;

}

static uint32_t Size_locked_ring_buffer ( struct Queue* self )
{
    struct nonconcurrent_ring_buffer* buff = (struct nonconcurrent_ring_buffer*) self->internals;
    return buff->count;
}

struct Queue new_locked_ring_buffer (int capacity, size_t element_size)
{
    struct Queue result;
    result.capacity     = capacity;
    result.element_size = element_size;
    result.internals    = malloc(sizeof(struct nonconcurrent_ring_buffer));
    struct nonconcurrent_ring_buffer* buff =result.internals;
    if(NULL==result.internals)
    {
        /*TODO: handle error or pass on the failiur*/
        result.Dequeue  = NULL;
        result.Enqueue  = NULL;
        result.Peak     = NULL;
        return result;
    }
    buff->data = calloc(capacity, element_size);
    if(NULL==buff->data)
    {
        free(result.internals);
        result.internals= NULL;
        result.Dequeue  = NULL;
        result.Enqueue  = NULL;
        result.Peak     = NULL;
        return result;

    }
    buff -> end = buff -> start = buff -> data;
    buff -> count = 0;
    result.Dequeue  = &Dequeu_nonconcurrent_ring_buffer;
    result.Enqueue  = &Enqueue_nonconcurrent_ring_buffer;
    result.Peak     = &Peak_nonconcurrent_ring_buffer;
    result.Size    = &Size_nonconcurrent_ring_buffer;

    return result;

}

void free_locked_ring_buffer (struct Queue * queue)
{
    free(((struct nonconcurrent_ring_buffer*)(queue->internals))->data);
    free(queue->internals);
}

