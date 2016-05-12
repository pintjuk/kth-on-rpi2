#include <lib.h>
#include <types.h>

#include "dtest.h"
extern void syscall_get_pid(uint32_t* pid);
extern BOOL store(uint32_t, uint32_t);
extern uint32_t load(uint32_t);
extern uint32_t getAndDicrement(addr_t);
extern uint32_t getAndIncrement(addr_t);
extern uint32_t CAS(addr_t, uint32_t, uint32_t);
#define SHARED_BASE 0xE0000000
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
            }
        }*/
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
    testCAS();
    asm("b .");
}

/*Each guest must provide a handler rpc*/
void handler_rpc(unsigned callNum, void *params)
{

}
