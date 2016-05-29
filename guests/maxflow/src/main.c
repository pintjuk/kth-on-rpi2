#include <lib.h>
#include <types.h>

#include "goldberg.h"
#include "maxflow.h"
#include "shared_alloc.h"
#include "dtest.h"
#include "ttas.h"
#include "buffer.h"

struct barrier main_berrier shared;
uint32_t print_lock shared;


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

void barrier1(struct barrier* b/*address to array of length= number of threads*/){
    uint32_t* count = (uint32_t*) b;
    static uint32_t mysence = 1;
    int position = getAndIncrement(&(b->count));
    
   // PRINTF("addrs position %x, sence %x, mysence %x, pid %x\n", position, (*sence), mysence, getPid());
    if(position == 3)
    {
        DMB;
        b->count=0;
        b->sence=mysence;
        DMB;
        SEV;
    }
    else
    {
        
        for(;;)
        {
            if((b->sence)==mysence)
                break;
            printf("");
            WFE;
        }
    }
   // PRINTF("new sence %x, pid %x\n", (*sence), getPid());
    mysence=!(b->sence);
}
uint32_t shared_base[100] shared;
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

void test_fail_lock()
{
    
    volatile int* mylock;
    volatile int* evidence;
    volatile uint32_t* ba;
    int pid     = getPid();
    ba      = &shared_base[1];
    mylock      = &shared_base[20];
    evidence    = &shared_base[30];
    if(pid==0)
    {
        mylock[0]=0;
        evidence[0]=0;
    }
        // sunchronize
    int i;
    //barier(ba);
    PRINTF("lel %x\n", mylock);

    int k;
    for(k=0; k<100;k++)
    {
        //lock(mylock);
        PRINTF("mybe is pid:%x\n", getPid());
        int j;
        for(j=0; j<10000; j++)
        {
            evidence[0]++;
        }
        for(j=0; j<10000; j++)
        {
            evidence[0]--;
        }

        //unlock(mylock);
        
       // barier(ba);
        if(0==pid)
        {
            PRINTF("***************\n");
            if(evidence[0]!=0)
            {
                PRINTF("FAILD!\n evedince %x\n", evidence[0]);
                return;
            }
        }
    }

    if(getPid()==0)
    {
        if(evidence[0]==0)
        {
            PRINTF("SUCCESS!\n");
        }
        else
        {
            printf("\n");
        }
        PRINTF("evidence: %i\n", evidence[0]);

    }

}

void test_fail_IncrimentDicriment()
{
    
    volatile int* from;
    volatile int* to;
    volatile int* core0;
    volatile int* core1;
    volatile int* core2;
    volatile int* core3;
    int old;
    int pid = getPid();
    from=&shared_base[20];
    to=&shared_base[30];
    core0=&shared_base[12];
    core1=&shared_base[13];
    core2=&shared_base[14];
    core3=&shared_base[15];
        if(pid==0)
    {
        old=*from=100000000;
        *to=0;
        *core0=0;
        *core1=0;
        *core2=0;
        *core3=0;
    }
    // sunchronize
    shared_base[pid]=1;
    int i;
    for(i=0; i<4; i++)
        while(1!=shared_base[i]){};
  //  if(pid!=0)
  //      return;  

    for(;;)
    {
        int old = *from;
        if(0>=old)
            break;
        (*from)--;
        (*to)++;
        core0[pid]++;
    }
    if(pid==0)
    {
        if(old==*to && old==((*core0)+(*core1)+(*core2)+(*core3)))
            printf("SUCCESS!\n");
        printf("old: %i, now %i, to: %i\ncore0:%i, core1:%i, core2:%i, core3:%i\n", old,*from, *to, *core0, *core1, *core2, *core3);

    }

}
void fail_testIncrimentDicriment2()
{
   int calc=10000000; 
    volatile int* from;
    volatile int* to;
    volatile int* core0;
    volatile int* core1;
    volatile int* core2;
    volatile int* core3;
    int old;
    int pid = getPid();
    from=&shared_base[20];
    to=&shared_base[30];
    core0=&shared_base[12];
    core1=&shared_base[13];
    core2=&shared_base[14];
    core3=&shared_base[15];
       
    if(pid==0)
    {
        old=*from=10000000;
        *to=0;
        *core0=0;
        *core1=0;
        *core2=0;
        *core3=0;
    }
        volatile int e=*from;
        e=(*to);
        e=(*core0);
        e=(*core1);
        e=(*core2);
        e=(*core3);


    // sunchronize
    shared_base[pid]=1;
    int i;
    for(i=0; i<4; i++)
        while(1!=shared_base[i]){};
        for(i=0;i<calc;i++)
        {
        
            (*to)++;
            core0[pid]++;

        }
    shared_base[pid]=2;
    for(i=0; i<4; i++)
        while(2!=shared_base[i]){};
    if(pid==0)
    {
        if(calc*4==*to && calc*4==((*core0)+(*core1)+(*core2)+(*core3)))
            printf("SUCCESS!\n");
        printf("old: %i,now %i, to: %i\ncore0:%i, core1:%i, core2:%i, core3:%i\n", old,*from, *to, *core0, *core1, *core2, *core3);

    }

}

void testIncrimentDicriment2()
{
   int calc=10000000; 
    volatile int* from;
    volatile int* to;
    volatile int* core0;
    volatile int* core1;
    volatile int* core2;
    volatile int* core3;
    int old;
    int pid = getPid();
    from=&shared_base[20];
    to=&shared_base[30];
    core0=&shared_base[12];
    core1=&shared_base[13];
    core2=&shared_base[14];
    core3=&shared_base[15];
       
    if(pid==0)
    {
        old=*from=calc*4;
        *to=0;
        *core0=0;
        *core1=0;
        *core2=0;
        *core3=0;
    }
        volatile int e=*from;
        e=(*to);
        e=(*core0);
        e=(*core1);
        e=(*core2);
        e=(*core3);


    // sunchronize
    shared_base[pid]=1;
    int i;
    for(i=0; i<4; i++)
        while(1!=shared_base[i]){};
        for(i=0;i<calc;i++)
        {
        
            getAndIncrement(to);
            getAndDicrement(from);
            core0[pid]++;

        }
    shared_base[pid]=2;
    for(i=0; i<4; i++)
        while(2!=shared_base[i]){};

    if(pid==0)
    {
        if(calc*4==*to && calc*4==((*core0)+(*core1)+(*core2)+(*core3)))
            printf("SUCCESS!\n");
        printf("from %i, to: %i\ncore0:%i, core1:%i, core2:%i, core3:%i\n",*from, *to, *core0, *core1, *core2, *core3);

    }

}


void testIncrimentDicriment()
{
    
    volatile int* from;
    volatile int* to;
    volatile int* core0;
    volatile int* core1;
    volatile int* core2;
    volatile int* core3;
    int old;
    int pid = getPid();
    from=&shared_base[20];
    to=&shared_base[30];
    core0=&shared_base[12];
    core1=&shared_base[13];
    core2=&shared_base[14];
    core3=&shared_base[15];
       
    if(pid==0)
    {
        old=*from=10000000;
        *to=0;
        *core0=0;
        *core1=0;
        *core2=0;
        *core3=0;
    }
        volatile int e=*from;
        e=(*to);
        e=(*core0);
        e=(*core1);
        e=(*core2);
        e=(*core3);


    // sunchronize
    shared_base[pid]=1;
    int i;
    for(i=0; i<4; i++)
        while(1!=shared_base[i]){};
    if(pid!=0){
        for(;;)
        {
        
            int old = getAndDicrement(from);
            getAndIncrement(to);
            core0[pid]++;
            if(0>=old)
                break;

        }
    }
    while((*from)>0){}
    if(pid==0)
    {
        if(old==*to && old==((*core0)+(*core1)+(*core2)+(*core3)))
            printf("SUCCESS!\n");
        printf("old: %i,now %i, to: %i\ncore0:%i, core1:%i, core2:%i, core3:%i\n", old,*from, *to, *core0, *core1, *core2, *core3);

    }

}

void test_heap_singlecore()
{
   // if(getPid()==1){
        int pid=getPid();
        shared_base[pid]=1;
        int i;
        for(i=0; i<4; i++)
            while(1!=shared_base[i]){};

        sm_init_heap();
        char* testchar1;
        short* testshort;
        int* testint;
        int* intarray1;
        int* intarray2;
        int* intarray3;
        testshort=sm_alloc(sizeof(short));
        testchar1=sm_alloc(sizeof(char));
        sm_print_heap();
        sm_free(testchar1);
        sm_print_heap();
        testchar1 = sm_alloc(sizeof(char));
        sm_print_heap();

        intarray1=sm_alloc(sizeof(int[30]));
        intarray2=sm_alloc(sizeof(int[30]));
        intarray3=sm_alloc(sizeof(int[20]));
        sm_print_heap();
        sm_free(intarray2);
        sm_print_heap();
        printf("allocating interray2 sizeof int[29]");
        intarray2=sm_alloc(sizeof(int[29]));
        sm_print_heap();
        sm_free(intarray2);
        sm_print_heap();
        printf("allocating interray2 sizeof int[28]");
        intarray2=sm_alloc(sizeof(int[28]));
        sm_print_heap();
        sm_free(intarray2);
        sm_print_heap();
        printf("allocating interray2 sizeof int[27]");
        intarray2=sm_alloc(sizeof(int[27]));
        sm_print_heap();
        printf("freeing intarray2\n");
        sm_free(intarray2);
        sm_print_heap();
        printf("freeing intarray3\n");
        sm_free(intarray3);
        sm_print_heap();
        sm_free(testchar1);
        sm_free(testshort);
        sm_free(intarray1);
        sm_print_heap();

        printf("trying to alloc to much\n");
        printf("result: %x\n", sm_alloc(sizeof(int[262144])));
        sm_print_heap();

        printf("trying to alloc to much in two batches\n");
        intarray1=sm_alloc(sizeof(int[262140]));
        sm_print_heap();
        intarray2=sm_alloc(sizeof(int[10]));
        printf("result of second alloc %x\n", intarray2);
        sm_print_heap();

        sm_free(intarray1);

        printf("allocating allot, and memseting all the alocated stuff\n");
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
        sm_print_heap();
        for(i=0; i<10; i++)
        {
            if(ptrs[i*2+1]!=NULL)
                sm_free(ptrs[i*2+1]);
            sm_print_heap();
        }

        sm_print_heap();


   // }

}

void test_buffer(){
    struct Queue buf=new_nonconcurrent_ring_buffer(5, sizeof(int));
    int lel[10]={1,2,3,4,5,6,7,8,9};
    buf.Enqueue(&buf, &lel[0]);
    buf.Enqueue(&buf, &lel[1]);
    buf.Enqueue(&buf, &lel[2]);
    buf.Enqueue(&buf, &lel[3]);
    buf.Enqueue(&buf, &lel[4]);

    printf("try to allocate %x\n", buf.Enqueue(&buf, &lel[5]));
    printf("dequeing %i\n", *(int*)(buf.Dequeue(&buf)));
    printf("try to allocate %x\n", buf.Enqueue(&buf, &lel[1]));
    printf("try to allocate %x\n", buf.Enqueue(&buf, &lel[1]));
    printf("dequeing %i\n", *(int*)(buf.Dequeue(&buf)));
    printf("dequeing %i\n", *(int*)(buf.Dequeue(&buf)));
    printf("dequeing %i\n", *(int*)(buf.Dequeue(&buf)));
    printf("dequeing %i\n", *(int*)(buf.Dequeue(&buf)));
    printf("dequeing %i\n", *(int*)(buf.Dequeue(&buf)));

}

struct Queue shared_test_buffer shared;
void test_locked_buffer(){
    int i;
    
    if(0==getPid()){
        PRINTF("aoeu");
        shared_test_buffer=new_locked_ring_buffer(10000, sizeof(uint32_t));
        PRINTF("AOEU");
    }
    
    barrier1(&main_berrier);
    for(i=0; i < 5000; i++)
    {
        int w=(100000*getPid())+i;
        shared_test_buffer.Enqueue(&shared_test_buffer, &w);
    }

    barrier1(&main_berrier);
    uint32_t priv[4]={0,0,0,0};
    int popedcount=0; 

    PRINTF("aoeuaoeuaoeu: %i\n", shared_test_buffer.Size(&shared_test_buffer));
    for(;;)
    {
        uint32_t* res = shared_test_buffer.Dequeue(&shared_test_buffer);
        if(res== NULL)
            break;
        if(priv[(*res)/100000] > (*res)){
            PRINTF("FALED!!! ovorunvalue\n %i, %i\n", priv[(*res)/1000], (*res));
            return;
        }

        popedcount++;
        priv[(*res)/100000] = (*res);

       // PRINTF("Dequeued: %i, %i\n", *res, (*res)/1000);
        free(res);
    }
    barrier1(&main_berrier);
    PRINTF("=%i\n", popedcount);
    
}

void foo(uint32_t *i){
    PRINTF("*#*#*#*#*#*#: %i\n", *i);
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
    int i;
    barrier1(&main_berrier);

    init_heap();
    if(3==getPid())
    {
        s_init_heap();
        //test_buffer();
    }

    test_locked_buffer();
 PRINTF("lelelel\n");

   // barrier1(&main_berrier);
   // PRINTF("WHATHATFUCK!");
   // barier(&main_berrier);
    
    //test_locked_buffer();
    // testIncrimentDicriment2();
    // test_fail_lock();
    //testCAS();
    //test_fail_IncrimentDicriment();
    // test_heap_singlecore();
    asm("b .");
}

/*Each guest must provide a handler rpc*/
void handler_rpc(unsigned callNum, void *params)
{

}
