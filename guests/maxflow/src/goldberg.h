#ifndef __GOLDBERG_H_
#define __GOLDBERG_H_

#include <lib.h>
#include <types.h>


struct r_edge
{
    uint32_t from;
    uint32_t to;
    uint32_t residual;
    struct r_edge*  back_edge;
};

struct node
{
    uint32_t id;
    uint32_t lable;
    uint32_t excess;
};

struct nighbors
{
    uint32_t capacity;
    uint32_t size;
    struct r_edge* list;
};

void testflow();

#endif
