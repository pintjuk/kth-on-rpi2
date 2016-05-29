#ifndef _MAXFLOW_H_    
#define _MAXFLOW_H_

extern void syscall_get_pid(uint32_t* pid);
extern BOOL store(uint32_t, uint32_t);
extern uint32_t load(uint32_t);
extern uint32_t getAndDicrement(addr_t);
extern uint32_t getAndIncrement(addr_t);
extern uint32_t CAS(addr_t, uint32_t, uint32_t);
uint32_t getPid();


extern void lock(uint32_t*);
extern void unlock(uint32_t*);
extern void s_init_heap();
extern void s_free(void*);
extern void* s_malloc(uint32_t size);
extern void* s_calloc(uint32_t num, uint32_t size);


#define SHARED_BASE (0xE0000000)
#define SHARED_SIZE (0x00100000)
#define SHARED_END (SHARED_BASE+SHARED_SIZE)



#define DMB asm volatile ("dmb")
#define WFE asm volatile ("wfe")
#define SEV asm volatile ("sev") 


#define shared __attribute__((section(".shared_bss")))

struct barrier {
    uint32_t count;
    uint32_t sence;    
};
    
extern struct barrier main_berrier shared;
extern uint32_t print_lock shared;

#define PRINTF(...) { lock(&print_lock); printf(__VA_ARGS__); unlock(&print_lock); }

#endif

