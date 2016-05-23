    .code 32
    .balign 4
    
.global syscall_get_pid
syscall_get_pid:
    svc #1337
    bx lr

.global store
store:
    //ldrex r3, [r1]
    //mov r2,r0
    //strex r0, r2, [r1]
    str r0, [r1]   
    bx lr

/*.global lock
lock:
    LDREX R5, [R0]       // read lock
    CMP R5, #0           // check if 0
    WFENE                // sleep if the lock is held
    STREXEQ R5, R1, [R0] // attempt to store new value
    CMPEQ R5, #0         // test if store suceeded
    BNE lock             // retry if not
    DMB // ensures that all subsequent accesses are observed after the
        // gaining of the lock is observed
        // loads and stores in the critical region can now be performed
    bx lr

.global unlock
unlock:
    MOV R1, #0
    DMB             // ensure all previous accesses are observed before the lock is cleared
    STR R1, [R0]    // clear the lock.
    DSB             // ensure completion of the store that cleared the lock before sending the event
    SEV
    bx lr
*/

.global lock
lock:
    //r0: lock addrs, r1: 1, r2: initial the lock value then status of strex
    mov r1, #1
    LDREX R2, [R0]       // read lock
    CMP R2, #0           // check if 0
    WFENE                // sleep if the lock is held
    STREXEQ R2, R1, [R0] // attempt to store new value
    CMPEQ R2, #0         // test if store suceeded
    BNE lock             // retry if not
    DMB // ensures that all subsequent accesses are observed after the
        // gaining of the lock is observed
        // loads and stores in the critical region can now be performed
    bx lr

.global unlock
unlock:
    MOV R1, #0
    DMB             // ensure all previous accesses are observed before the lock is cleared
    STR R1, [R0]    // clear the lock.
    DSB             // ensure completion of the store that cleared the lock before sending the event
    SEV
    bx lr


.global TAS 
TAS:
    //r0= addres, r1= old value,r2=succes?, r3=true
    mov r3, #1
tas_try:
    ldrex r1, [r0]
    strex r2, r3, [r0]
    cmp r2, #0
    moveq r0, r1
    bxeq lr
    b tas_try

.global CAS
CAS:
    //r0= addres, r1= expected, r2= sorce
    push {r4}

cas_try:
    ldrex r3, [r0]
    //if unexpected return
    cmp r1, r3
    bne cas_return

    strex r4, r2, [r0]
    cmp r4, #0
    // if attomic return 
    beq cas_return
    // if not attomic retry
    b cas_try

cas_return:
    mov r0, r3
    pop {r4}
    bx lr

.global getAndDicrement
getAndDicrement:
// r0=address, r1=oldvalue,r3=newvalue, r2=success
    dmb
    ldrex r1, [r0]
    sub r3, r1, #1
    dmb
    strex r2, r3, [r0]
    //if stored return old value
    cmp r2, #0
    moveq r0, r1
    bxeq lr
    // otherwise retry
    b getAndDicrement


.global getAndIncrement
getAndIncrement:
// r0=address, r1=oldvalue, r2=success, r3=new value
    dmb
    ldrex r1, [r0]
    add r3, r1, #1
    dmb
    strex r2, r3, [r0]
    //if stored return old value
    cmp r2, #0
    moveq r0, r1
    bxeq lr
    // otherwise retry
    b getAndIncrement

.global load
load:
    ldr r0, [r0]
    bx lr

.global syscall_dmmu
syscall_dmmu:
    svc #666
    bx lr
