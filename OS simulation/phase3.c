/*
 * Author: Angel Benavides
 * File: phase3.c
 * Assingnment: phase 3
 * Purpose: Implement mechanisms to support user-mode processes, specifically
 * system calls. This is done by assigning trampoline functions to each index
 * on the systemCallVec array that was produced in phase 2. 
 */

#include "myphase3.h"

struct SEM SEM_ARR[MAXSEMS];
struct sPCB SPDT[MAXPROC];
int lock;


/**
 * Prepare any data structure program will use for the execution of phase 3.
 */
void phase3_init(void) {
  memset(SPDT, 0, sizeof(SPDT));
  memset(SEM_ARR, 0, sizeof(SEM_ARR));
  systemCallVec[SYS_SPAWN] = sys_spawn;
  systemCallVec[SYS_WAIT] = sys_wait;
  systemCallVec[SYS_TERMINATE] = sys_terminate;
  systemCallVec[SYS_SEMCREATE] = sys_semcreate;
  systemCallVec[SYS_SEMP] = sys_semp;
  systemCallVec[SYS_SEMV] = sys_semv;
  systemCallVec[SYS_GETTIMEOFDAY] = sys_time;
  systemCallVec[SYS_GETPROCINFO] = sys_proc;
  systemCallVec[SYS_GETPID] = sys_pid; 
  lock = MboxCreate(1,0); // Create lock with mailbox
  MboxSend(lock, NULL, 0); // Make lock available
 
}

void phase3_start_service_processes(void) {
  // Did not need.
}

/**
 * A syscall handler for GetTimeofDay user code function.
 * 
 * @outparam arg1, time of day.
 */
void sys_time(USLOSS_Sysargs *args){
  args -> arg1 = (void *)(long) currentTime();
  userOn();
}

/**
 * A syscall handler for CPUTime user code function.
 *
 * @outparam arg1, cpu time.
 */
void sys_proc(USLOSS_Sysargs *args){
  args -> arg1 = (void *)(long) readtime();
  userOn();
}

/**
 * A syscall handler for GetPID user code function.
 *
 * @outparam arg1, pid.
 */
void sys_pid(USLOSS_Sysargs *args){
  args -> arg1 = (void *)(long) getpid();
  userOn();
}


/**
 * A syscall handler for Spawn user code function.
 *
 * @param args, stuct containing necceary parameters for fork1 call.
 *
 * @outparam arg1, pid of newly created child process.
 * @outparam arg4, 0 for succes. 
 */
void sys_spawn(USLOSS_Sysargs *args) {
  // Exract args from struct and cast.
  int (*func)(char *) = (int (*)(char *)) args -> arg1;
  char *funcArg = (char *) args -> arg2;
  int stack_size = (int)(long) args -> arg3;
  int priority = (int)(long) args -> arg4;
  char *name = (char *) args -> arg5;
  
  getlock(); // lock
  int pid = fork1(name, forkbox, funcArg, stack_size, priority); 
  int index = pid % MAXPROC;
  sPCB p = {1, pid, func, funcArg};
  SPDT[index] = p;
  args -> arg1 = (void *)(long) pid; 
  args -> arg4 = 0;
  unlock();

  userOn();
}

/*
 * Syscall handler for Wait user code function. Calls join and inputs results
 * into parameters from args.
 *
 * @param args, struct containing out parameters.
 *
 * @outparam arg1, PID of cleaned process.
 * @outparam arg2, status of the cleaned up process.
 * @outparam arg4, -2 if no children; 0 otherwise.
 */
void sys_wait(USLOSS_Sysargs *args) {
  int status;
  //print("Starting kid process\n");
  int pid = join(&status);
  if(pid == -2) {
    args -> arg4 = (void *)(long) pid; //return pid and status remain undefined
    return;
  }
  userOn();
  args -> arg1 = (void *)(long) pid;
  args -> arg2 = (void *)(long) status;
  args -> arg4 = 0;
}

/**
 * Syscall handler for Terminate user code function. Calls join until it
 * recieves -2 and then calls quit.
 *
 * @param args, struct containing parameters.
 */
void sys_terminate(USLOSS_Sysargs *args) {
  int quitStatus = (int)(long) args -> arg1;
  int status;
  while(join(&status) != -2) {
    ;
  }
  quit(quitStatus);
}

/**
 * Create a new semaphore object. Finds a place on an array of semaphores
 * and then creates object.
 *
 * @param value, the intial value to set the semaphore too.
 * @param semaphore, an out parameter that takes the semaphore id.
 *
 * @return zero unless their is an error.
 */
void sys_semcreate(USLOSS_Sysargs *args) { 
  int id = find_sem(); // Look for empty spot
  int value = (int)(long) args -> arg1;
  if(id == -1 || value < 0) {
    args -> arg4 = (void*)(long) -1; 
    return;
  }
  // Put semaphore in array.
  SEM *s = &SEM_ARR[id];
  int mb_id = MboxCreate(5,0);
  SEM temp = {id, value, mb_id, 1}; // 1 indicates spot on array is taken.
  *s = temp;
  args -> arg1 = (void*)(long) id;
  args -> arg4 = 0;
  userOn();
}

/**
 * Decrements a semaphore.
 */
void sys_semp(USLOSS_Sysargs *args) {
  int id = (int)(long) args -> arg1;
  
  if(SEM_ARR[id].in_use == 0) {
    args -> arg4 = (void *)(long) -1;
    return;
  }
  
  SEM *s = &SEM_ARR[id];
  if(s -> value == 0) {
    MboxRecv(s -> mb_id, NULL, 0); // Blocks process. 
  } else {
    s -> value--; // Decrementes value.
  }
  args -> arg4 = 0;
  userOn();
}


/**
 * Increments a semaphore.
 */
void sys_semv(USLOSS_Sysargs *args) {
  int id = (int)(long) args -> arg1;
  if(SEM_ARR[id].in_use == 0) {
    args -> arg4 = (void *)(long) -1;
    return;
  }
  SEM *s = &SEM_ARR[id];

  // Checks if process was waiting. Incrementes value if no process was
  // waiting.
  int result = MboxCondSend(s -> mb_id, NULL, 0);
  if(result == -2) {
    s -> value++;
  }
  args -> arg4 = 0;
  userOn();
}


/* --------------------------- Helper functions -----------------------------*/


/**
 * Finds an open spot on semaphore array. 
 *
 * @return int, the id/index on the semaphore array. -1 if no spots avialable.
 */
int find_sem() {
  int index;
  for(index = 0; index < MAXSEMS; index++) {
    if(SEM_ARR[index].in_use == 0) return index;
  }
  return -1;
}

/**
 * A trampoline function that is given to fork. Used so process main function
 * is called in user mode.
 */
int forkbox(char *arg) {
  getlock();
  sPCB *p = &SPDT[getpid() % MAXPROC];
  int (*func)(char*) = p -> main;
  char *a = p -> arg;
  unlock();
  /* -------  Must be in User Mode  -------*/
  userOn();
  int code = func(a);
  Terminate(code);
  return 0;
}


/**                                                                                 
 * Enable User mode using mask and '&' operator.                                  
 */                                                                                 
void userOn() {                                                                   
  unsigned int psr = USLOSS_PsrGet();                                               
  if((psr & USLOSS_PSR_CURRENT_MODE) == 1) { // Checks if on.                     
    psr = (~USLOSS_PSR_CURRENT_MODE & psr) | USLOSS_PSR_CURRENT_INT;             
    int code = USLOSS_PsrSet(psr); // Sets to User mode.                             
    if(code == USLOSS_ERR_INVALID_PSR) { // Looks for Error.                        
      USLOSS_Console("ERROR: Invalid PSR\n");                                      
      USLOSS_Halt(1);                                                              
    }                                                                             
  }                                                                                 
} 

/**
 * Tries to obtain lock by receiving a message from a mailbox designated with a
 * id called lock. Gets blocked until message is recieved.
 */
void getlock() {
  MboxRecv(lock, NULL, 0);
}

/**
 * Releases lock by sending message to mailbox with the designated id called
 * lock.
 */
void unlock() {
  MboxSend(lock, NULL, 0);
}

/**
 * Clean sPCB slot, 
 */
void clean_sPCB(sPCB *p) {
  memset(p, 0, sizeof(sPCB));
    
}
