#ifndef _MAXFLOW_H_    
#define _MAXFLOW_H_

extern void syscall_get_pid(uint32_t* pid);
extern BOOL store(uint32_t, uint32_t);
extern uint32_t load(uint32_t);
extern uint32_t getAndDicrement(addr_t);
extern uint32_t getAndIncrement(addr_t);
extern uint32_t CAS(addr_t, uint32_t, uint32_t);
uint32_t getPid();

#define SHARED_BASE (0xE0000000)
#define SHARED_SIZE (0x00100000)
#define SHARED_END (SHARED_BASE+SHARED_SIZE)


#endif
