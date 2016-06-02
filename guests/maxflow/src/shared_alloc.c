#include "shared_alloc.h"
#include "maxflow.h"    
#include "ttas.h"

#define ALLOCATED_BLOCK (1<<31)
#define HEAP_MAGIC 123123

//#define DEBUG_ALLOC

#ifdef DEBUG_ALLOC
    #define DEBUG(...) printf(__VA_ARGS__)
    #define DEBUG1(a) printf(a)
    #define DEBUG2(a, b) printf(a, b)
    #define DEBUG3(a, b, c) printf(a, b, c)
#else
    #define DEBUG(...)
    #define DEBUG1(a)
    #define DEBUG2(a, b) 
    #define DEBUG3(a, b, c) 
#endif


void sm_init_heap(struct sm_heap_ctxt_t* heap_ctxt, uint32_t* heap_start, uint32_t* heap_end )
{
    heap_ctxt->inited=HEAP_MAGIC; 
    DEBUG1("*initilizing heap\n");
    DEBUG3("heap_magic %x\nheap_start %x\n", heap_ctxt->inited, heap_start);
    DEBUG3("heap_trailer %x\n aoeu %x \n", (heap_end)-1, 1<<32);
    heap_ctxt->start=heap_start;
    heap_ctxt->end=heap_end;
    *(heap_ctxt->start) = (heap_end-heap_start)&(~ALLOCATED_BLOCK);
    DEBUG2("header of initial block %x\n", *heap_start); 
    *(heap_ctxt->start-1) = *heap_start;

    DEBUG2("trailer of initial block %x\n", *heap_start); 
}

void sm_print_heap(struct sm_heap_ctxt_t* ctxt)
{
    uint32_t* heap_start = ctxt->start;
    uint32_t* heap_end = ctxt->end;
    
    if(ctxt->inited!=HEAP_MAGIC)
    {
        printf("Heap not initilized!\n");
        return;
    }
    printf("---------------------\nheap start: %x, heap end: %x\n--------------------------\n", heap_start, heap_end);
    uint32_t* current_block=heap_start;
    for(;;){
        if(current_block>=heap_end){
            printf("---------------------\n");
            return;
        }

        printf("Block %x-%x -", current_block, current_block+((*current_block)&(~ALLOCATED_BLOCK))-1);
        if(*current_block&ALLOCATED_BLOCK)
            printf("allocated\n");
        else
            printf("free\n");
        
        current_block=(current_block+(*current_block));
    }
}

void * sm_alloc(struct sm_heap_ctxt_t* ctxt, size_t size)
{
    uint32_t* heap_start = ctxt->start;
    uint32_t* heap_end = ctxt->end;
    
    if(ctxt->inited!=HEAP_MAGIC)
        return NULL;

    DEBUG2("*sm alloc\n size=%x\n", size);
    uint32_t aligned_size = (size+3)/4;
    
    uint32_t* current_block=heap_start;
    void* result=NULL;
    for(;;)
    {
        DEBUG2("current_block %x,\n", current_block);
        if(current_block>=heap_end){
            DEBUG1("heap ended, returning null\n");
            result=NULL;
            break;
        }
        
        uint32_t header = ( *current_block );
        uint32_t blocksize = header & ( ~ALLOCATED_BLOCK );
        DEBUG2("current blocksize: %x\n", blocksize);
        DEBUG2("aligned_size: %x\n", aligned_size);
        if(!(header & ALLOCATED_BLOCK) && ((blocksize)>=(aligned_size+2)))
        {
            /***** found usable block *******/
            BOOL nosplit = (aligned_size+4)>=blocksize; //is block to little to split in two?
            DEBUG2("nosplit: %x\n", nosplit);
            uint32_t tobealocated=(nosplit)? blocksize:aligned_size+2;
            DEBUG2("tobealocated: %x\n", tobealocated);
            result= (void*)(current_block+1);
            DEBUG2("result %x\n", result);
            *current_block=(tobealocated|ALLOCATED_BLOCK);
            DEBUG2("new block value: %x\n", *current_block);
            *(current_block+tobealocated-1)=*current_block;
            DEBUG2("adders of trailer %x\n", (current_block+tobealocated));

            if(!nosplit)
            {
                *(current_block+tobealocated)=(blocksize-tobealocated)&(~ALLOCATED_BLOCK);
                *(current_block+blocksize-1)=*(current_block+tobealocated);
            }
            break;
        }
        else
        {
            DEBUG1("333333333333\n");
            current_block=current_block+blocksize;
            continue;
        }
        
    }
    return result;
}



void sm_free(struct sm_heap_ctxt_t* ctxt, void* ptr)
{
    uint32_t* heap_start = ctxt->start;
    uint32_t* heap_end = ctxt->end;
  
   if(ctxt->inited!=HEAP_MAGIC)
        return NULL;

    
    uint32_t* current_block = (uint32_t*)ptr;
    current_block--;
    DEBUG2("current block: %x\n", current_block);
    (*current_block)=(*current_block)&(~ALLOCATED_BLOCK);
    *(current_block+*current_block-1)=*current_block;
    uint32_t* nextblock=current_block+(*current_block);
    DEBUG2("next block %x", nextblock);
    if(nextblock<heap_end)
    {
        if(!((*nextblock)&ALLOCATED_BLOCK))
        {
            // merg this and next block 
            (*current_block)=(*current_block)+(*nextblock);
            *(current_block+(*current_block)-1)=(*current_block);
            DEBUG1("finish mergin next block\n");
        }
    }
    uint32_t priviousBlockLen=*(current_block-1);
    priviousBlockLen=priviousBlockLen&(~ALLOCATED_BLOCK);
    uint32_t* privius=(current_block-priviousBlockLen);
    DEBUG2("privious block: %x\n", privius);
    DEBUG2("heapstart %x\n", heap_start);
    if(privius>=heap_start)
    {
        DEBUG2("content of privious trailer: %x\n", *(current_block-1));
        if(!(*(current_block-1)&ALLOCATED_BLOCK))
        {
            DEBUG1("starting to merg privious block\n");
            // merg this and privius blocksize
            (*privius)=(*privius)+(*current_block);
            *(privius+(*privius)-1)=*privius;
            
        }
    }

}
