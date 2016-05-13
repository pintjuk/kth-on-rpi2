#include "shared_alloc.h"
#include "maxflow.h"    
#include "ttas.h"

    /***********************
    * heap header
    * 0-4: magic number, signifies that heap is initilized
    * 4-8: heap lock
    ***********************/
#define HEAP_MAGIC (0xABC)
static uint32_t* heap_magic=(uint32_t*)(SHARED_BASE);
static uint32_t* big_heap_lock=(uint32_t*)(SHARED_BASE+4);
static uint32_t* heap_start=(uint32_t*)(SHARED_BASE+8);
static uint32_t* heap_end=(uint32_t*)(SHARED_END);
#define ALLOCATED_BLOCK (1<<31)

#define DEBUG_ALLOC

#ifdef DEBUG_ALLOC
    #define DEBUG1(a) printf(a)
    #define DEBUG2(a, b) printf(a, b)
    #define DEBUG3(a, b, c) printf(a, b, c)
#else
    #define DEBUG1(a)
    #define DEBUG2(a, b) 
    #define DEBUG3(a, b, c) 
#endif


void init_heap()
{
    *big_heap_lock=1;
    *heap_magic=HEAP_MAGIC;
    DEBUG1("*initilizing heap\n");
    DEBUG3("heap_magic %x\nheap_start %x\n", heap_magic, heap_start);
    DEBUG3("heap_trailer %x\n aoeu %x \n", ((uint32_t*)SHARED_END)-1, 1<<32);
    *heap_start = (heap_end-heap_start)&(~ALLOCATED_BLOCK);
    DEBUG2("header of initial block %x\n", *heap_start); 
    *(heap_end-1) = *heap_start;
    *big_heap_lock=0;

}

void print_heap()
{
    if(*heap_magic!=HEAP_MAGIC)
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

void * sm_alloc(size_t size)
{

    if(*heap_magic!=HEAP_MAGIC)
        return NULL;

    ttas_lock(big_heap_lock);

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
    ttas_unlock(big_heap_lock);
    return result;
    
}



void sm_free(void* ptr)
{
    if(*heap_magic!=HEAP_MAGIC)
        return NULL;

    ttas_lock(big_heap_lock);
    
    uint32_t* current_block = (uint32_t*)ptr;
    current_block--;
    DEBUG2("current block: %x\n", current_block);
    (*current_block)=(*current_block)&(~ALLOCATED_BLOCK);
    *(current_block+*current_block-1)=*current_block;
    uint32_t* nextblock=current_block+(*current_block);
    printf("next block %x", nextblock);
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
    printf("privious block: %x\n", privius);
    printf("heapstart %x\n", heap_start);
    if(privius>=heap_start)
    {
        printf("content of privious trailer: %x\n", *(current_block-1));
        if(!(*(current_block-1)&ALLOCATED_BLOCK))
        {
            DEBUG1("starting to merg privious block\n");
            // merg this and privius blocksize
            (*privius)=(*privius)+(*current_block);
            *(privius+(*privius)-1)=*privius;
            
        }
    }

    ttas_unlock(big_heap_lock);
}
