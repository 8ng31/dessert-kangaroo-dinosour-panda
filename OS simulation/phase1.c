/**
 * Author: Angel Benavides 
 * File: phase1.c
 * Assignment: Phase1
 * Purpose: Create a Process Data Table to keep track of what processes exist.
 * Uses USLOSS to simulate the operation of a single core CPU. Simulates the
 * context switching of unique processes through the use of a schedular, called
 * dispatcher. Implements round robin scheduling.
 *
 */ 
#include <phase1.h>
#include "myphase1.h"

struct PCB PDT[MAXPROC];
struct Scheduler dispatcher; 
int open_slots = MAXPROC;
int pid_count = 1; // Used to create a pid.
int currPID = 0; // Keeps track of the current process runnnig. 
int TIME_SLICE = 80000;
int startTime;

/**
 * Function used to start entire OS.
 * Calls dispatcher, never returns
 */
void startProcesses(void) { 
  checkMode("startProcess"); 

  callDispatch(); // Call dispatcher
  while(1) {} 
}

/*
 * Initializes Process Data Table.
 * Sets PSR to Kernal mode.
 * Disables interrupts
 * Creates 'init' Process Control Block
 */
void phase1_init(void) { 
  kernalOn(); 
  interOff(); // Disables interrupts
  USLOSS_IntVec[USLOSS_CLOCK_DEV] = clockHandler;
  checkMode(); // Check if OS is in kernal mode.
  
  // Initializes data structures
  memset(PDT, 0, sizeof(PDT));
  memset(&dispatcher, 0, sizeof(dispatcher));
  for(int i = 1; i < 8; i++) initializeQueue(&dispatcher.queue[i]); 
  // Creates init process.
  int pid = pidGen();
  int slot = getSlot(pid);
  struct Queue q;
  initializeQueue(&q); 
  struct PCB init_pcb = {"init", 1, pid, slot, 6, 0, 0, {0}, NULL, NULL, 0,
    NULL, NULL, NULL, NULL, NULL, NULL, q, 0, 0, 0, 0, 0};
  PDT[slot] = init_pcb; 
  USLOSS_ContextInit(&PDT[slot].context, malloc(USLOSS_MIN_STACK), 
      USLOSS_MIN_STACK, NULL, init_wrapper);

  // Adds init pc to dispatcher.
  addToDispatch(pid, 6);
  restorePSR();

} 

/*
 *  Creates child process Process Control Block with given arguements.
 *  Generates pid and assigns slot.
 *  Assigns PCB to the Process Data Table.
 *  Creates a context for the PCB using a trampoline function to support the
 *  use of any function.
 *
 *  @param name, name of process
 *  @param func, the address to a funcion
 *  @param arg, the arguement to a function, can be NULL if function does not
 *  have a parameter.
 *  @param stacksize, the amount of memory the function needs.
 *  @param priority, for the dispatcher to use in scheduling.
 *
 *  @return pid, generated Process ID.
 */
int fork1(char *name, int(*func)(char *), char *arg, int stacksize, 
    int priority) {
 

  checkMode("fork1"); // Check for kernal mode
  interOff(); // Disable interrupts

  // Check for errors
  if(stacksize < USLOSS_MIN_STACK) return -2;
  if(open_slots == 0 || (priority > 5 && strcmp(name, "sentinel") != 0)|| 
     priority < 1) return -1;
  if(func == NULL || name == NULL || strlen(name) > MAXNAME) return -1;
  
  // Starts PCB construction
  int pid = pidGen(); // generate pid
  int slot = getSlot(pid); // Finde empty index
  struct Queue q;
  initializeQueue(&q);
  struct PCB pcb = {{0}, 1, pid, slot, priority, 0, 0, {0}, func, arg, 0, 
    &PDT[getSlot(getpid())], NULL, NULL, NULL, NULL, NULL, q, 0, 0};
  strcpy(pcb.name, name); // insert name to pcb
  PDT[slot] = pcb; // Assign PCB to index in PDT
  USLOSS_ContextInit(&PDT[slot].context, malloc(stacksize), 
      stacksize, NULL, fork1_wrapper); // Create context w/ usloss function.
  add_child(&PDT[slot]); // Adds this pcb to its parent's child list.
  
  // Adds to dispatcher and checks if needs to be ran.
  addToDispatch(pid, priority); 
  callDispatch();
  
  restorePSR(); // Enable inter
  return pid;
}

/*
 * Iterates through children list of process that called function to look for
 * child process that has completed. If a processes found to have completed, 
 * the status of the parent process will be set to the childrens status. The 
 * PCB will then be reset for futher use.
 *
 * @param status, out parameter used to give child status to parent.
 * 
 * @return pid, the pid of the process that was cleaned up.
 */
int join(int *status) {
	checkMode("join"); // Check for kernal mode
  interOff(); // Disable interrupts.
  

  // Get curr process control myphase1.
  PCB *parent = &PDT[getSlot(getpid())]; 
  parent->joinBlock = 1;
  if(parent -> child_count == 0) return -2;
  /* DEBUG: USLOSS_Console("Has children\n");*/
  
  //Check the children for finished process.
  PCB *child = get_quit_proc(parent -> child);
  while(child == NULL) {
    blockMe(11);
    child = get_quit_proc(parent -> child); 
  }

    
  parent -> child_count--;   // Decrement child count
  *status = child -> status; // Set out parameter
  int pid = child -> pid;
  clean_slot(child); // Resets PCB
  restorePSR(); // Enable interrupts
  return pid; 
}

/**
 * Sets quit flag on the PCB of process to on, this allows 'join' to clean up
 * process. If process has children, error message will print and halt program. 
 * Switches to different process when process PCB has been updated.
 *
 * @param status, the exit status the program finished will be saved to process
 * PCB.
 * @param switchToPid, the next process to switch. 
 */
void quit(int status) {
  checkMode("quit");
  interOff(); 
  PCB *pc = getPCB(getpid());

  // Check if process has children, halts program w/ error msg if has children.
  if(pc -> child_count != 0) {
   USLOSS_Console("ERROR: Process pid %d called quit() while it still had children.\n", pc -> pid);
   USLOSS_Halt(1);
  }

  pc -> quit++; // Change the quit field to 1, indicating program quit.  
  pc -> status = status; // Update states of process.
  if(pc -> parent -> status == 11) {
    PCB* parent = getPCB(pc -> parent -> pid);
    addToDispatch(parent->pid, parent->priority);
  }
  Queue* zappers = &pc -> zappers; 
  while(peek(zappers) != -1) {
    PCB* zapper = getPCB(pop(zappers));
    if(zapper -> pid != pc -> parent -> pid ) {
      zapper -> status = 0;
      zapper -> zapBlock = 0;
      addToDispatch(zapper -> pid, zapper -> priority); 
    }  
  }
  callDispatch();
  while(1) {}
}

/**
 * Calls dispatcher to context switch to the highest priority process.
 * Iterates through q's with priority of 1 -7.
 */
int callDispatch() {
  checkMode("callDispatch");
  interOff(); 

  PCB* pc = getPCB(getpid());

  for(int i = 1; i <= 7; i++) {
    if(pc != NULL) {     
      if(pc -> priority == i && (pc -> timeSlice + elapsedTime()) < TIME_SLICE &&
          !pc -> status && !pc -> quit) {
        return 0;
      }
    }
    Queue *q = &dispatcher.queue[i];
    if(peek(q) != -1) {
      PCB *process = getPCB(dequeue(q));
      switchTo(process); 
      return 0;
    } 
  }
  restorePSR();
  return -2;
}

/**
 * Return a pointer to a PCB.
 */
PCB* getPCB(int pid) {
  if(pid == 0) return NULL;
  return &PDT[getSlot(pid)];
}


/**
 * Used to add process to dispatcher.
 * 
 * @param process, a pointer to an address.
 */
void addToDispatch(int pid, int priority) {
  Queue *queue = &dispatcher.queue[priority];
  enqueue(queue, pid);
}

/**
 * Prints out info from the process table. Prints process in order of PDT index
 * starting at 1 and wrapping around to 0. 
 * Includes: 
 *    Name 
 *    PID   
 *    ParentPID   
 *    Priority 
 *    Punnable Status - 0=runnable, > 0 = blocked
*/
void dumpProcesses() {
  checkMode();
  interOff();
  int slot = 0;
    int ppid = 0;
    int pid = 0;

    USLOSS_Console(" PID  PPID  NAME              PRIORITY  STATE\n");

  while (slot < MAXPROC){

        if (PDT[slot].slot_closed == 0){ // Checks if index is empty
            slot++;
            continue;
        }
        pid = PDT[slot].pid;
        USLOSS_Console("%4d", pid);
        if (slot != 1){
           ppid = PDT[slot].parent->pid;
        }
        else{
           ppid = 0; // exception for init process.
        }
        USLOSS_Console("     %d  %-18s%d         ", ppid, PDT[slot].name, PDT[slot].priority);

        if (pid == getpid()){
            USLOSS_Console("Running\n");
        }
        else if (PDT[slot].status == 0){
            USLOSS_Console("Runnable\n");
        }
        else if (PDT[slot].joinBlock == 1 && !PDT[slot].quit){
          USLOSS_Console("Blocked(waiting for child to quit)\n");
        }
        else if (PDT[slot].zapBlock == 1 && !PDT[slot].quit) {
            USLOSS_Console("Blocked(waiting for zap target to quit)\n");
        }
        else if (PDT[slot].status >=  11 && !PDT[slot].quit) {
            USLOSS_Console("Blocked(%d)\n", PDT[slot].status);
        }
        else{
            USLOSS_Console("Terminated(%d)\n", PDT[slot].status);
         }

        slot++;
    }
  restorePSR();
}

/*Asks another function to terminate. sets flag to tell a process that 
 *it should quit() ASAP. if process is blocked, it will quit when unblocked
 *DOES NOT UNBLOCK TARGET PROCESS
 *
 * @param pid, the pid of the process to zap.
 */
void zap(int pid) {
  PCB *process = getPCB(pid);
  //if nonexistant or itself, print error and halt  
	if (pid == 1){
		USLOSS_Console("ERROR: Attempt to zap() init.\n");
		USLOSS_Halt(1);
	}	
	else if (pid <= 0){
		USLOSS_Console("ERROR: Attempt to zap() a PID which is <=0.  other_pid = %d\n", pid);
		USLOSS_Halt(1);
	}
	else if (process -> pid != pid || process == NULL){
        	USLOSS_Console("ERROR: Attempt to zap() a non-existent process.\n");
        	USLOSS_Halt(1);
  }
	else if(process->quit == 1){
		USLOSS_Console("ERROR: Attempt to zap() a process that is already in the process of dying.\n");
		USLOSS_Halt(1);
	}
	else if (pid == getpid()){
        USLOSS_Console("ERROR: Attempt to zap() itself.\n");
        USLOSS_Halt(1);
  }


  PCB *curr = &PDT[getSlot(getpid())]; //grab the current proc
  curr->zapBlock = 1;
  enqueue(&process->zappers, curr->pid);
  blockMe(11);
}


/*
 * Asks if the current procces has been zapped by another
 *
 */
int isZapped(void) {
  // DEBUG: USLOSS_Console("INSIDE ZZAPPPYYY");
  //get curr proc
  PCB *curr = getPCB(getpid()); //grab the current proc
  if(isEmpty(&curr->zappers)){
    //curr proc has been zapped;
    return 0;
  }
  return 1; 
}


/*
 * Used by other phases to ask that current process be blocked for a while
 * 
 * @param block_status, the status to set the pcb too.
 */
void blockMe(int block_status) {
    interOff();
    if (block_status <= 10){
        USLOSS_Console("error: block status too low\n");
				USLOSS_Halt(1);
    }	
    //set the blocked status
    PCB *process = &PDT[getSlot(getpid())];
    process->status = block_status;
    //call the dispatcher to switch to an unblocked process
		restorePSR();
    //dumpProcesses();
    callDispatch();
}

int unblock(int pid){
  interOff(); 

  PCB *process = getPCB(pid);
	int status = process->status;
	//if process was not blocked, does not exist, or is blocked on status <= 10
	if (!status || !process->slot_closed || status <= 10){
		return -2;
	}
  addToDispatch(pid, process -> priority);
  process->zapBlock = 0;
  process->joinBlock = 0;
	process->status = 0;
	restorePSR();
  return 0;
}

/*
 * Unblocks a process that was previously blocked with blockMe()
 *
 * @param pid, the pid to unblock 
 */
int unblockProc(int pid) {
  int block = unblock(pid);
  if (block == -2){
  return -2;
  }
  callDispatch();
  return 0;
}


/**
 * Helper function for  callDispatch, switches context in CPU. 
 * Updates the context of the current process, then switches to another
 * process.
 *
 * @param pid, the next process to switch too. 
 */
void switchTo(PCB *newPc) {
  if(currPID == 0) { 
    currPID = newPc -> pid;
    readCurStartTime();
    USLOSS_ContextSwitch(NULL, &newPc->context);
    return;
  }
  PCB* prevPc = getPCB(getpid()); 
  if(!prevPc -> status && !prevPc -> quit) {
    addToDispatch(prevPc -> pid, prevPc -> priority);
  }
  currPID = newPc -> pid;
  // USLOSS function to switch contexts in cpu, updates context of current
  // process before switch.
  prevPc -> totalTime += elapsedTime();
  if(prevPc -> timeSlice >= TIME_SLICE) prevPc -> timeSlice = 0; 
  readCurStartTime();
  USLOSS_ContextSwitch(&prevPc->context, &newPc->context);

}



/** --------------------------- Time functions -----------------------------**/

/**
 * Reads the current time running in USLOSS. Uses DeviceInput to do so
 * has outparameter to return time.
 *
 * @return startTime, the current time.
 */
int readCurStartTime(void) {
  startTime = currentTime(); 
  return startTime;
}

/*
 * Checks how long a process has been running in the CPU and calls 
 * dispatcher if time has exceded.
 */
void timeSlice(void) {
  PCB* pc = getPCB(getpid());
  int time = elapsedTime();
  if (pc -> timeSlice + time > TIME_SLICE){
    callDispatch();
  }
}

/*
 * Read the total amount of cpu time used by curr process.
 *
 * @return cpuTime, the total amount of cpuTime used. 
 */
int readtime(void) {
    int cpuTime = 0; 
    return cpuTime;
}


/**
 * Function to read the current wall-clock time from CLOCK device
 * 
 * @return retval, the current time on the CLOCK device.
 */
int currentTime() {
    int retval;
    int usloss_rc = USLOSS_DeviceInput(USLOSS_CLOCK_DEV, 0, &retval);
    assert(usloss_rc == USLOSS_DEV_OK);

    return retval;
}

/**
 * Returns the amount of time the current process has been on the CPU.
 */
int elapsedTime() {
  return currentTime() - startTime;
}

/* 
 * Author: Russell Lewis
 * This is the interrupt handler for the CLOCK device
 */
void clockHandler(int dev,void *arg) {
  if(0) {
    USLOSS_Console("clockHandler(): PSR = %d\n", USLOSS_PsrGet());
    USLOSS_Console("clockHandler(): currentTime = %d\n", currentTime());
  }
  /* make sure to call this first, before timeSlice(), since we want to do
   * the Phase 2 related work even if process(es) are chewing up lots of
   * CPU.
   */
  phase2_clockHandler();
  // call the dispatcher if the time slice has expired
  timeSlice();
  /* when we return from the handler, USLOSS automatically re-enables
   * interrupts and disables kernel mode (unless we were previously in
   * kernel code).  Or I think so.  I haven’t double-checked yet.  TODO
   */
  //interOn();
}

/*-------------------------- Special Processes ------------------------------*/

/*
 * Begins bootstrap, folled by creating two child processes sentinel and
 * testcase_main. Upon creation of both, context switches to testcase main.
 * Prints Completion message if testcase main returns. 
 */
void init() {
  checkMode();
  phase2_start_service_processes();
  phase3_start_service_processes();
  phase4_start_service_processes();
  phase5_start_service_processes();
  fork1("sentinel", sentinel, NULL, USLOSS_MIN_STACK, 7);  
  fork1("testcase_main", tc_main_wrapper, NULL, USLOSS_MIN_STACK, 3);
  int * status = &PDT[getSlot(getpid())].status;
  while(join(status) != -2) {
    ; 
  }
   USLOSS_Console("Error no children left\n");
  USLOSS_Halt(1);
}

/**
 * Wrapper function for init, used to create context.
 */
void init_wrapper() {
  interOn();
  init();
  restorePSR();
}

/**
 * Used to detect deadlock. Has last priority, will only run if all else has
 * stopped.
 */
int sentinel() {
  checkMode();
  while (1) {
    if (phase2_check_io() == 0){
     USLOSS_Console("DEADLOCK DETECTED!  All of the processes have blocked, but I/O is not ongoing.\n");
      USLOSS_Halt(1);
    } 
    USLOSS_WaitInt();
  }
  return -1; 
}

/*--------------------------- Helper functions ------------------------------*/


/**
 *  Called by join after the status has been collected from a child process
 *  that has quit.
 *
 *  @param child, An address to the child PCB.
 */
void clean_slot(PCB *child) {
  // Connect prev & next adjacent sibling structs to each other
  if(child -> prev != NULL) child -> prev -> next = child -> next;
  if(child -> next != NULL) child -> next -> prev = child -> prev;
  if(child -> pid == child -> parent -> child -> pid) {
    child -> parent -> child = child -> parent -> child -> next;
  }
  memset(&PDT[child -> slot], 0, sizeof(PCB));
  open_slots++; // increment slots to let fork know there is space in PDT.
}

/*
 * @return currPID, pid of the current process in CPU.
 */
int getpid() {
  return currPID;
} 

/**
 *  Helper function for join. Recursively searches a list of sibilings
 *  to see if a process has quit.
 *
 *  @param child, a child process
 *
 *  @return child process that has quit.
 */
PCB* get_quit_proc(PCB* child) {
  if(child == NULL) return NULL;
  if(child -> quit == 1) return child;
  return get_quit_proc(child -> next);
}


/**
 * A trampoline function used to create a context for testcase_main().
 *
 * @param null, NULL pointer for fork function.
 */
int tc_main_wrapper(char * null) {
  interOn();
  int status = testcase_main();
  USLOSS_Halt(status);
  return 0;
}

/**
 * Adds child to parent process struct.
 *
 * @param newChild, pointer to PCB getting appended to child list of parent.
 */
  void add_child(PCB * newChild) {
    newChild -> parent -> child_count++;

    // If process has no children
    if (newChild -> parent -> child == NULL) {
      newChild -> parent -> child = newChild;
      return;
    }

    // Appends pcb to front of child list.
    newChild -> next = newChild -> parent -> child;
    newChild -> parent -> child = newChild;
  }

/**
 *  Helper function for adding a child to a parent process.
 *  Recurses through child process until one has an next pointer.
 *
 *  @param child, pcb with pointers to sibilngs.
 *  @param newChild, pcb that will be pointed to by a next or prev pointer.
 */
void add_sibling(PCB *child, PCB *newChild) {
  if(child -> next == NULL) { // Checks for empty pointer
    child -> next = newChild; // Sets pointer.
    return;
  }
  add_sibling(child -> next, newChild); // Recursive call to next sibling
}

/**
 * Wrapper function to help add context to PDT in fork1 function.
 */
void fork1_wrapper() {
  interOn(); // Enable interrupts 
  PCB *proc = &PDT[getSlot(getpid())];
  int (*main)(char*) = proc -> main;
  char* arg = PDT[getSlot(getpid())].arg;
  int status = main(arg);
  restorePSR();
  quit(status); // If returns calls quit on behalf
}

/**
 * Creates a pid for process.
 * Uses a count variable and modulos it by MAXPROC
 * to check if it is an available index spot on the
 * PDT.
 *
 * @return pid, a process identifaction number that can be moduloed
 * to find the location of the process on the table.
 */
int pidGen() {
  if(open_slots < 1) return -1; 
  int pid = pid_count++;
  int slot = pid % MAXPROC; // Finds slot
  if (PDT[slot].slot_closed == 0) { // Slot closed is False
    open_slots--; 
    return pid;
  }
  return pidGen();
}

/**
 * Uses pid to get slot on PDT table.
 *
 * @param pid, process identification.
 *
 * @return slot, index on PDT.
 */
int getSlot(int pid) {
  return pid % MAXPROC;
}


/** ----------------- PSR Functions -------------------*/

/**
 * Check if USLOSS is in kernal or user mode.
 * Calls USLOSS to halt program if in user mode.
 *
 * @param func, the function name, used for printing error message.
 */
void checkMode(char* func) {
  unsigned int mode = USLOSS_PsrGet();
  if ((mode & USLOSS_PSR_CURRENT_MODE) == 0 ) {
    USLOSS_Console("ERROR: Someone attempted to call %s while in user mode!\n", func);
    USLOSS_Halt(1);
  }
}

/**
 * Enable Kernal mode using mask and '&' operator.
 */
void kernalOn() {
  unsigned int psr = USLOSS_PsrGet(); 
  if((psr & ~USLOSS_PSR_CURRENT_MODE) == 0) { // Checks if off.
    psr = USLOSS_PSR_CURRENT_MODE | psr; 	
    int code = USLOSS_PsrSet(psr); // Sets to Kernal mode.
    if(code == USLOSS_ERR_INVALID_PSR) { // Looks for Error.
       USLOSS_Console("ERROR: Invalid PSR\n");  
       USLOSS_Halt(1);
      }
  }    
}

/**
 *  Disable interrupts using mask and '&' operator.
 */
void interOff() {
  unsigned int psr = USLOSS_PsrGet();
  if((psr & USLOSS_PSR_CURRENT_INT) == 2) { // Checks if interrupts are on.
    psr = ~USLOSS_PSR_CURRENT_INT & psr; 	
    int code = USLOSS_PsrSet( psr); // Turns interrupts on.
    if(code == USLOSS_ERR_INVALID_PSR) { // Checks for error.
       USLOSS_Console("ERROR: Invalid PSR\n");  
       USLOSS_Halt(1);
      }
  }    
}

/**
 *  Enable interrputs using mask and '|' operator.
 */
void interOn() {
  int unsigned psr = USLOSS_PsrGet();
  if((psr & USLOSS_PSR_CURRENT_INT) != 2) { // Checks if interrupts are off.
    psr = USLOSS_PSR_CURRENT_INT | psr; 	
    int code = USLOSS_PsrSet( psr); 
    if(code == USLOSS_ERR_INVALID_PSR) { // Checks for error message.
       USLOSS_Console("ERROR: Invalid PSR\n");  
       USLOSS_Halt(1);
      }
  }    
}

/**
 * Restores the PSR to what is was last.
 */
void restorePSR() {
  int unsigned psr = USLOSS_PsrGet();
  int unsigned prev = USLOSS_PSR_PREV_MASK >> 2;
  psr = prev | psr;                                         
  int code = USLOSS_PsrSet( psr); // Restores PSR to previous state.           
  if(code == USLOSS_ERR_INVALID_PSR) { // Checks for error message.           
     USLOSS_Console("ERROR: Invalid PSR\n");                                  
     USLOSS_Halt(1);                                                          
  }               
}

/** ------------------- Queue ------------------- **/


/**
 * initialize fields in Queue.
 *
 * @param queue, pointer to q.
 */
void initializeQueue(Queue *queue) {
  queue->front = -1;
  queue->tail = -1;
}

/**
 * Checks if Queue is full.
 *
 * @param queue, pointer to q.
 */
int isFull(Queue *queue) {
  return ((queue->tail + 1) % MAXPROC == queue->front);
}

/**
 * Adds num to front of Reader Queue.
 *
 * @param queue, pointer ot a q.
 * @param num, id of a num
 */
void enqueue(Queue *queue, int num) {
  if (isFull(queue)) return;

  if (queue->front == -1) {
      queue->front = 0;
  }

  queue->tail = (queue->tail + 1) % MAXPROC;
  queue->array[queue->tail] = num;
}

/**
 * Returns num of reader in front of q.
 *
 * @param queue, pointer to a q.
 */
int dequeue(Queue *queue) {
  int num;
  if (isEmpty(queue)) return -1;

  num = queue->array[queue->front];
  if (queue->front == queue->tail) {
      queue->front = queue->tail = -1;
  } else {
      queue->front = (queue->front + 1) % MAXPROC;
  }
  return num;
}

/**
 * Removes the last element in a Queue. Not ideal for a Queue, but used
 * to pass test 18 without readme for ordering.
 */
int pop(Queue *queue) {
  if (isEmpty(queue)) return -1;

  int num = queue->array[queue->tail];
  if(queue->front == queue->tail) {
      queue->front = queue->tail = -1;
  } else {
      queue->tail = (queue->tail - 1 + MAXPROC) % MAXPROC;
  }
  return num;
}

/**
 * Returns int at top of q.
 *
 * @param queue, pointer to a q.
 */
int peek(Queue *queue) {
  int num;
  if (isEmpty(queue)) return -1;
  num = queue->array[queue->front];
  return num;
}

/**
 * Checks if Queue is empty.
 *
 * @param queue, pointer to a q.
 */
int isEmpty(Queue *queue) {
  return (queue->front == -1);
}

