#include <lib.h>
#include <types.h>

#include "maxflow.h"
#include "shared_alloc.h"
#include "dtest.h"
volatile uint32_t* shared_base=(uint32_t*)SHARED_BASE;


uint32_t getPid(){
    static int memorizepid= -1;
    if(-1!=memorizepid)
    {
        return memorizepid;
    }
    else
    {
        syscall_get_pid(&memorizepid);
        return memorizepid;
    }
}

void testCAS()
{
    volatile int* from;
    volatile int* to;
    volatile int* core0;
    volatile int* core1;
    int old;
    int pid = getPid();
    from=&shared_base[20];
    to=&shared_base[30];
    core0=&shared_base[12];
    core1=&shared_base[13];

    if(pid==0)
    {
        old=shared_base[20]=10000000;
        shared_base[30]=0;
        shared_base[12]=0;
        shared_base[13]=0;
    }

    // sunchronize
    shared_base[1+pid]=1;
    while(1!=shared_base[2-pid]){};

    if(pid==0)
    {

        for(;;)
        {
            int old = *from;
            if(old<=0)
                break;

         /*   int nold=CAS(from, old, old-1);
            if(old==nold)
            {
                for(;;){
                    old = *to;
                    nold = CAS(to, old, old+1);
                    if(old==nold)
                        break;
                }
                (*core0)++;
            }*/
        }
    }
    if(pid==1)
    {
        for(;;)
        {
            int old = *from;
            if(old<=0)
                break;

            int nold=CAS(from, old, old-1);
            if(old==nold)
            {
                for(;;){
                    old = *to;
                    nold = CAS(to, old, old+1);
                    if(old==nold)
                        break;
                }
                (*core1)++;
            }
        }
        if(old==*to && old==((*core0)+(*core1)))
            printf("SUCCESS!\n");
        printf("old: %i, to: %i\ncore0:%i, core1:%i\n", old, *to, *core0, *core1);

    }


}


void testIncrimentDicriment()
{

    volatile int* from;
    volatile int* to;
    volatile int* core0;
    volatile int* core1;
    int old;
    int pid = getPid();
    from=&shared_base[20];
    to=&shared_base[30];
    core0=&shared_base[12];
    core1=&shared_base[13];

    if(pid==0)
    {
        old=shared_base[20]=10000000;
        shared_base[30]=0;
        shared_base[12]=0;
        shared_base[13]=0;
    }

    // sunchronize
    shared_base[1+pid]=1;
    while(1!=shared_base[2-pid]){};

    if(pid==0)
    {

        for(;;)
        {
            int old = getAndDicrement(from);
            if(0>=old)
                break;
            getAndIncrement(to);
            (*core0)++;
        }
    }
    if(pid==1)
    {
        
        for(;;)
        { 
            int old = getAndDicrement(from);
            if(0>=old)
                break;
            getAndIncrement(to);
            (*core1)++;
        }
        if(old==*to && old==((*core0)+(*core1)))
            printf("SUCCESS!\n");
        printf("old: %i, to: %i\ncore0:%i, core1:%i\n", old, *to, *core0, *core1);

    }

}

void test_heap_singlecore()
{
    if(getPid()==1){
        char* testchar1;
        short* testshort;
        int* testint;
        int* intarray1;
        int* intarray2;
        int* intarray3;
        testshort=sm_alloc(sizeof(short));
        testchar1=sm_alloc(sizeof(char));
        print_heap();
        sm_free(testchar1);
        print_heap();
        testchar1 = sm_alloc(sizeof(char));
        print_heap();

        intarray1=sm_alloc(sizeof(int[30]));
        intarray2=sm_alloc(sizeof(int[30]));
        intarray3=sm_alloc(sizeof(int[20]));
        print_heap();
        sm_free(intarray2);
        print_heap();
        printf("allocating interray2 sizeof int[29]");
        intarray2=sm_alloc(sizeof(int[29]));
        print_heap();
        sm_free(intarray2);
        print_heap();
        printf("allocating interray2 sizeof int[28]");
        intarray2=sm_alloc(sizeof(int[28]));
        print_heap();
        sm_free(intarray2);
        print_heap();
        printf("allocating interray2 sizeof int[27]");
        intarray2=sm_alloc(sizeof(int[27]));
        print_heap();
        printf("freeing intarray2\n");
        sm_free(intarray2);
        print_heap();
        printf("freeing intarray3\n");
        sm_free(intarray3);
        print_heap();
        sm_free(testchar1);
        sm_free(testshort);
        sm_free(intarray1);
        print_heap();

        printf("trying to alloc to much\n");
        printf("result: %x\n", sm_alloc(sizeof(int[262144])));
        print_heap();

        printf("trying to alloc to much in two batches\n");
        intarray1=sm_alloc(sizeof(int[262141]));
        print_heap();
        intarray2=sm_alloc(sizeof(int[10]));
        printf("result of second alloc %x\n", intarray2);
        print_heap();

        sm_free(intarray1);

        printf("allocating allot, and memseting all the alocated stuff\n");
int i;
        int* ptrs[20];
        for(i=0; i<20; i++){
            int* thisptr=sm_alloc(sizeof(int[20000]));
            printf("allocated %x\n", thisptr);
            ptrs[i]=thisptr;
            if(thisptr!=NULL)
                memset(thisptr, 0,sizeof(int[20000]));
        }
        printf("now dealocating all the ptrs!\n");
        for(i=0; i<10; i++)
        {
            printf("curr ptr: %x\n", ptrs[i*2]);
            if(ptrs[i*2]!=NULL)
                sm_free(ptrs[i*2]);
        }
        print_heap();
        for(i=0; i<10; i++)
        {
            if(ptrs[i*2+1]!=NULL)
                sm_free(ptrs[i*2+1]);

        print_heap();
        }

        print_heap();


    }

}

void _main()
{
	uint32_t p0, p1, p2 , p3;
    __asm__ volatile (
         "mov %0, r3\n"
         "mov %1, r4\n"
         "mov %2, r5\n"
         "mov %3, r6\n"
         : "=r"(p0), "=r"(p1), "=r"(p2), "=r"(p3) : );
   // testIncrimentDicriment();
  //  testCAS();
  test_heap_singlecore();
    asm("b .");
}

/*Each guest must provide a handler rpc*/
void handler_rpc(unsigned callNum, void *params)
{

}
