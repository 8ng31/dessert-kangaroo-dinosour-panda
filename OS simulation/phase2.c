/*
 * Author: Angel Benavides                                     
 * File: phase2.c                                                                                                                        
 * Assingnment: phase 2                                                            
 * Purpose: Implement Mmailbox for OS in order to understand the critical          
 * asspects of for handling interrupts in a simulation. MailBox is a mechanism
 * that allows you to send and receive messages or discreate packages of data
 * ranging in bytes from 0 to MAX_MESSAGE bytes long(inclusive). MailBox can       
 * have any number of processes send and receive messages from it. We call a       
 * process sending a message a Producer and a Consumer if it is recieving          
 * messages. Both types of processes will block depending on MailBox               
 * availability. Producers will block when MailBox is full, and be unblocked       
 * when they are able to deliver their own msg. Consumers block when their is   
 * no messages in the MailBox and are unblocked when the MailBox gets a message 
 * the consumer can take.                                                          
 */

#include "myphase2.h"

void (*systemCallVec[MAXSYSCALLS])(USLOSS_Sysargs *args);
struct sPCB SPDT[MAXPROC];
struct MailBox mailboxes[MAXMBOX];
struct Slot slots[MAXSLOTS];
int open_sPCB_slots;
int curr_sPCB;
int mb_open_slots;
int curr_mb;
int open_slots;
int curr_slot;
int wd_lock;
int start_time;
int time;

/*
 * Initialize data structures. Cannot use fork here.
 */
void phase2_init(void){
  isInKernalMode("phase2_init");
  //interOff();
  memset(SPDT, 0, sizeof(SPDT));
  memset(mailboxes, 0, sizeof(mailboxes));
  memset(slots, 0, sizeof(slots));
  mb_open_slots = MAXMBOX;
  curr_mb = 0;
  open_slots = MAXSLOTS;
  curr_slot = 0;
  open_sPCB_slots = MAXPROC;
  curr_sPCB = 0;
  wd_lock = 0; // Wait device flag
  start_time = 0;
  genMBS();
  for (int i = 0; i < MAXSYSCALLS; i++) {
    systemCallVec[i] = nullsys; 
  }
  USLOSS_IntVec[USLOSS_SYSCALL_INT] = syscallHandler;  
  USLOSS_IntVec[USLOSS_DISK_INT] = diskHandler;  
  USLOSS_IntVec[USLOSS_TERM_INT] = termHandler;
}

/*
 * Creates new mailbox
 *
 * @param numSlots, the maximum number of slots that may be used to queue up 
 * messages from this mailbox.
 * @param slotSize, he largest allowable message that can be sent through this 
 * mailbox.
 *
 * @return id, identification number of new mailbox. -1 if negative numSlots or
 * slotSize is negative or larger than allowed. -1 if no mailboxes avialable.
 */
int MboxCreate(int numSlots, int slot_size) {
  isInKernalMode("MboxCreate");
  interOff();
  if(mb_open_slots < 1) return -1;
  if(numSlots > open_slots) return -1;
  if(numSlots < 0 || slot_size < 0) return -1;
  if(slot_size > MAX_MESSAGE) return -1;
  int id = id_gen();
  Queue qp = {SPCB, 0, NULL, NULL, NULL, NULL};
  Queue qc = {SPCB, 0, NULL, NULL, NULL, NULL};
  Queue q = {SLOT, 0, NULL, NULL, NULL, NULL};
  MailBox mb = {1, id, numSlots, 0, slot_size, qp, qc, q};
  mailboxes[getIndex(id)] = mb;
  restorePSR();
  return id;
}

/*
 * Destroys a mailbox.
 * Frees slots consumed by mailbox.
 * Unblocks all Producers and Consumers that use mailBox.
 * Wakes up processes and context switches. Does not finish immediately. 
 * Changes pid to -3.
 *
 * @param mailBoxID, the ID of the MailBox to destroy.
 *
 * @return int, 0 if the release was successful, -1 if the ID is not a mailbox
 * currently in use.
 */
int MboxRelease(int mailBoxID) {
  isInKernalMode("MboxRelease");
	interOff();
  MailBox *mb = &mailboxes[getIndex(mailBoxID)]; // Get mailbox.
  // Change pid so mailbox cannot be used.
  mb->id = -3;
  mb->slot_closed = -3;
  // Release slots.
  Slot *s = slotDequeue(&mb->messages);
  while(s != NULL) {
    releaseSlot(s);
    s = slotDequeue(&mb->messages); 
  }
  // Wake up processes.
  sPCB *p = dequeue(&mb->producers);
  while(p != NULL) {
    unblockProc(p->pid);
    p = dequeue(&mb->producers);
  }
  sPCB *c = dequeue(&mb->consumers);
  while(c != NULL) {
    unblockProc(c->pid);
    c = dequeue(&mb->consumers);
  }
  memset(mb, 0, sizeof(MailBox)); 
  mb_open_slots++; 
  restorePSR();
  return 0;
}

/*
 * Sends a message through a MailBox. If there are no consumers queued and no
 * space available to queue a message, then this process will block until the
 * message can be delivered - either to a consumer, or into a mail slot. Uses
 * helper function with condition flag to implement function.
 *
 * @param mbox_id, the ID of the mailbox.
 * @param msg_ptr, a pointer to a buffer.
 * @param msg_size, the size of the string in the buffer.
 *
 * @return int: 
 *  0 - Success
 *  -1 - Illegal values given as arguments (including invalid mailbox ID)
 *  -2 - The system has run out of global mailbox slots, so the message could
 *  not be queued
 *  -3 - The mailbox was released(but is not completely invalid yet)
 */
int MboxSend(int mbox_id, void *msg_ptr, int msg_size){
	isInKernalMode("MboxSend");
  interOff();
  int result = mbsHelper(mbox_id, msg_ptr, msg_size, NON_COND);
  restorePSR();
  return result;
}

/*
 * Waits to recieve a message through a mailbox. If there is a message already
 * queued in a mail slot, it may read it directly and return. Otherwise it will
 * block until a message is available.
 *
 * @param mbox_id, the ID of the mailbox
 * @param msg_ptr, pointer to a buffer containing a message.
 * @param msg_max_size, the size of the buffer. Can recieve message up to this
 * size.
 *
 * @return int:
 *    -3: The mailbox was released (but is not completely invalid yet).
 *    -1: Illegal values given as arguements(Invalid MB ID or msg_max_size
 *    exceeded).
 *    >= 0: The size of the message received.
 */
int MboxRecv(int mbox_id, void *msg_ptr, int msg_max_size){
	isInKernalMode("MboxRecv");
  interOff();
  int result = mbrHelper(mbox_id, msg_ptr, msg_max_size, NON_COND);
  restorePSR();
  return result;
}

/**
 * Checks to see if a mailbox has allocated enough slots to unblock a producer
 * process. Resests slot fields.
 *
 * @param mb, mailbox.
 * @param s, slot
 */
void updateMB(MailBox *mb) {
 if(peek(&mb->producers) != NULL) {
    sPCB *p = dequeue(&mb->producers);
    // get an empty slot
    int index = getSlot();// Write this function to get an empty slot.  
    // Add the message to a slot and enqueue it to the Mailbox.
    Slot *s = &slots[index];
    s->msg_len = p->msg_size;
    if(p->msg_ptr != NULL) memcpy(s->message, p->msg_ptr, s->msg_len);
    slotEnqueue(&mb->messages, s);
    mb->slots_in_use++;
    unblockProc(p->pid);
  } 
}


/**
 * Releases slot of fields and becomes available for further use.
 *
 * @param s, a slot to be released.
 */
void releaseSlot(Slot *s) {
  memset(s, 0, sizeof(Slot));
 open_slots++;
 } 

/*
 * Sends a message to a mailbox. Will never block, instead return -2.
 *
 * @param mbox_id, the ID of the mailbox.
 * @param msg_ptr, a pointer to a buffer.
 * @param msg_size, the size of the string in the buffer.
 *
 * @return int: 
 *  0 - Success
 *  -1 - Illegal values given as arguments (including invalid mailbox ID)
 *  -2 - The system has run out of global mailbox slots, so the message could
 *  not be queued
 *  -3 - The mailbox was released(but is not completely invalid yet)
 */
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size){
	isInKernalMode("MboxCondSend");
  interOff();
  int result = mbsHelper(mbox_id, msg_ptr, msg_size, COND);
  restorePSR();
  return result;
}

/*
 * Waits to recieve a message through a mailbox. If there is a message already
 * queued in a mail slot, it may read it directly and return. Will never block
 * instead will return -2.
 *
 * @param mbox_id, the ID of the mailbox
 * @param msg_ptr, pointer to a buffer containing a message.
 * @param msg_max_size, the size of the buffer. Can recieve message up to this
 * size.
 *
 * @return int:
 *    -3: The mailbox was released (but is not completely invalid yet).
 *    -1: Illegal values given as arguements(Invalid MB ID or msg_max_size
 *    exceeded).
 *    >= 0: The size of the message received.
 */
int MboxCondRecv(int mbox_id, void *msg_ptr, int msg_max_size){
	isInKernalMode("MboxCondRecv");
  interOff();
  int result = mbrHelper(mbox_id, msg_ptr, msg_max_size, COND);
  restorePSR();
  return result;
}

/*
 * Waits for an inturrupt to fire on a given device. Will recieve from the
 * proper mailbox for device. When the message arrives, it will store the
 * status into the out parameter and then return.
 *
 * @param type, the device type: clock, disk or terminal(In usloss.h)
 * @param unit, "unit" of given type that is being accessed. NULL pointer is
 * allowed.
 * @param status, out parameter, which will be used ot deliver the device
 * status once an interrupt arrives.
 */
void waitDevice(int type, int unit, int *status) {
  isInKernalMode("waitDevice");
  wd_lock = -5;
  
  if(USLOSS_CLOCK_DEV == type) {
    if(unit != 0) return;
    wd_lock = MboxRecv(0, status, sizeof(int));
  }

  if(USLOSS_DISK_DEV == type) {
    if(unit != DISK_1 && unit != DISK_2) return; 
    wd_lock = MboxRecv(unit + 1, status, sizeof(int));
  }

  if(USLOSS_TERM_DEV == type) {
    if(unit != TERM_1 && unit != TERM_2 && unit != TERM_3 && unit != TERM_4) return;
    wd_lock = MboxRecv(unit + 3, status, sizeof(int));
  } 
  
  for(int i = 0; i < 7; i++) {
    MailBox *mb = &mailboxes[i];
    if(peek(&mb->consumers) != NULL) wd_lock = -5;
  }
}

/*-------------------- INTERRUPT HANDLERS -------------------------------*/

/*
 * Runs code each time clock interrupt occurs.
 * NEED TO DETERMINE USE FOR.
 */
void phase2_clockHandler() { 
  isInKernalMode("phase2_clockHandler");
  time = currentTime();
  if(start_time == 0) start_time = time; 
  int elapsed_time = time - start_time;
  if(wd_lock == -5 && elapsed_time >= 100000) {
    int status = currentTime();
    MboxCondSend(USLOSS_CLOCK_DEV, &status, sizeof(int)); 
    start_time = 0;
  }
}

/*
 * Checks if any process is currently blocked inside waitDevice().
 *
 * @return int:
 *    nonzero: if any process is currently blocked.
 *    0: if no process is blocked.
 */
int phase2_check_io(void){
  isInKernalMode("phase2_check_io");
  if(wd_lock == -5)	return 1;
  return 0;
}

/*
 * Called by phase 1, Use to add any processes that implemenatation may need.
 * NEED TO DETERMINE IF USEFUL FOR IMPLEMENTATION.
 */
void phase2_start_service_processes(){
  isInKernalMode("phase2_start_service_processes");
}


/*
 * Handles a system call. Picks the appropriate system call in
 * the System Call Vector. All are going to be nullsys.
 *
 * @param
 */
void syscallHandler(int type, USLOSS_Sysargs *args) {
  kernalOn();
  interOff();
  
  int index = args-> number;
  
  if(index >= MAXSYSCALLS || index < 0) {
    USLOSS_Console("syscallHandler(): Invalid syscall number %d\n", index);
    USLOSS_Halt(1);
  }
  systemCallVec[index](args);
  userOn();
  restorePSR();
}

/**
 * Disk interrupt handler
 */
void diskHandler(int type, void* arg) {
  int unitNo = (int)(long)arg;
  int status;
  int usloss_rc = USLOSS_DeviceInput(USLOSS_DISK_INT, unitNo, &status); 
  assert(usloss_rc == USLOSS_DEV_OK);
  MboxCondSend(unitNo + 1, &status, sizeof(int)); 
}

/**
 * Terminal interrupt handler
 */
void termHandler(int type, void* arg) {
  int unitNo = (int)(long)arg;
  int status;

  int usloss_rc = USLOSS_DeviceInput(USLOSS_TERM_INT, unitNo, &status); 
  assert(usloss_rc == USLOSS_DEV_OK);
  MboxCondSend(unitNo + 3, &status, sizeof(int)); 
}

/*
 * Prints error message and Halts simulation.
 */
void nullsys(USLOSS_Sysargs *args) {
	USLOSS_Console("nullsys(): Program called an unimplemented syscall.  syscall no: %d   PSR: 0x%02x\n", 
      args->number, USLOSS_PsrGet());
  USLOSS_Halt(1);
}

/* ------------------------ Helper functions --------------------------------*/

/*
 * Helper function for MBoxSend.
 * Sends a message through a MailBox. If there are no consumers queued and no
 * space available to queue a message, then this process will block until the
 * message can be delivered - either to a consumer, or into a mail slot. Will
 * not block if conditional flag is on.
 *
 * @param mbox_id, the ID of the mailbox.
 * @param msg_ptr, a pointer to a buffer.
 * @param msg_size, the size of the string in the buffer.
 * @param c, flag to determine if the Send is conditional.
 *
 * @return int: 
 *  0 - Success
 *  -1 - Illegal values given as arguments (including invalid mailbox ID)
 *  -2 - The system has run out of global mailbox slots, so the message could
 *  not be queued
 *  -3 - The mailbox was released(but is not completely invalid yet)
 */
int mbsHelper(int mbox_id, void *msg_ptr, int msg_size, int c) { 
  MailBox *mb = &mailboxes[getIndex(mbox_id)]; // Get mailbox.
  if(msg_size > mb-> slot_size || mb-> slot_closed == 0) return - 1; 
  if(mb-> id != mbox_id || mb-> id == -3) return -1;
  if(peek(&mb-> consumers) == NULL && mb-> slots_in_use >= mb-> numSlots &&
      c == COND) return -2; 
  if(peek(&mb->consumers) != NULL) { // If consumer is waiting.
    sPCB *c = dequeue(&mb->consumers);// dequeue pc
    int delivered = 0; 
    if(msg_ptr != NULL && c->max_size >= msg_size) {
      memcpy(c->msg_ptr, msg_ptr, msg_size); // give it the msg
      delivered++;
    }
    if(!delivered && msg_size != 0) c->msg_size = -1;
    unblockProc(c->pid);
    goto finish;
  }
  if(mb-> slots_in_use >= mb-> numSlots || mb-> numSlots == 0) {   
    // Add producers to q and block them.
    int index = getSPCB_slot();  
    sPCB p = {1, getpid(), NULL, msg_ptr, msg_size, 0};
    SPDT[index] = p;
    enqueue(&mb->producers, &SPDT[index]);
    blockMe(11);
    goto finish;
  }
  // get an empty slot
  int index = getSlot();// Write this function to get an empty slot.  
  if(index == -2) {
    return -2;
  }
  // Add the message to a slot and enqueue it to the Mailbox.
  Slot *s = &slots[index];
  s->msg_len = msg_size;
  if(msg_ptr != NULL) memcpy(s->message, msg_ptr, msg_size); // give it the msg
  slotEnqueue(&mb->messages, s);
  mb->slots_in_use++; 
finish:
 if(mb-> slot_closed == 0 || mb->id == -3) return -3;
 return 0;
}

/*
 * Waits to recieve a message through a mailbox. If there is a message already
 * queued in a mail slot, it may read it directly and return. Otherwise it will
 * block until a message is available if a Non conditional flag is entered,
 *
 * @param mbox_id, the ID of the mailbox
 * @param msg_ptr, pointer to a buffer containing a message.
 * @param msg_max_size, the size of the buffer. Can recieve message up to this
 * size.
 *
 * @return int:
 *    -3: The mailbox was released (but is not completely invalid yet).
 *    -1: Illegal values given as arguements(Invalid MB ID or msg_max_size
 *    exceeded).
 *    >= 0: The size of the message received.
 */
int mbrHelper(int mbox_id, void *msg_ptr, int msg_max_size, int c) {
  MailBox *mb = &mailboxes[getIndex(mbox_id)];
  if(mb-> slot_closed == 0 || mb-> id == -3) return -1; 
  if(mb-> id != mbox_id || mb-> id == -3|| mb-> slot_closed == 0) return -3;
  if(slotPeek(&mb->messages) != NULL) {
    Slot *s = slotPeek(&mb->messages);
    if(s->msg_len > msg_max_size) return -1; 
    s = slotDequeue(&mb->messages); 
    if(msg_ptr != NULL) memcpy(msg_ptr, s->message, s->msg_len);
    int msg_len = s->msg_len;
    // Check to see if a producer is blocked.
    releaseSlot(s);
    mb->slots_in_use--;
    updateMB(mb);
    return msg_len;
  }

  if(peek(&mb->producers) != NULL) {
    sPCB *p = dequeue(&mb->producers);
    unblockProc(p->pid);
    goto finishR; 
  }

  // Return if conditional
  if(c == COND) return -2;
  
  int index = getSPCB_slot();
  sPCB p = {1, getpid(), NULL, msg_ptr, 0, msg_max_size}; //Add the fields to the SPDT.
  SPDT[index] = p;  
  enqueue(&mb->consumers, &SPDT[index]);  //Add sPCB to consumer q 
  blockMe(11); // Calls dispatch
  if(SPDT[index].msg_size == -1) return -1;
finishR:
  if(msg_ptr != NULL && (mb-> id != 0 && mb-> id != -3)) { 
    return strlen(msg_ptr) + 1;
  }
  if(mb-> slot_closed == 0 || mb->id == -3) return -3;
  return 0;
}

/**
 * Create the mailboxes for interrupts.
 * Uses hard coding due to instead of mbcreate becuase of manipulation of 
 * PSR.
 */
void genMBS() {
  for(int i = 0; i < 7; i++) {
    int s = 1; // numSlots
    int ss = sizeof(int); // Slot size
    int id = id_gen();
    Queue qp = {SPCB, 0, NULL, NULL, NULL, NULL};
    Queue qc = {SPCB, 0, NULL, NULL, NULL, NULL};
    Queue q = {SLOT, 0, NULL, NULL, NULL, NULL};
    MailBox mb = {1, id, s, 0, ss, qp, qc, q};
    mailboxes[getIndex(id)] = mb;
  }
}

/**
 * Searches for an empty slot on the slots table. 
 * If this function gets called a slot must exist. Protected in parent function
 * from endless recursion.
 *
 * @return int, index of empty slot. 
 */
int getSlot() {
  if(open_slots < 1) return -2;
  int slot = curr_slot % MAXSLOTS;
  if(slots[slot].slot_closed == 0) {
    slots[slot].slot_closed++;

    open_slots--;
    return slot;
  }
  curr_slot++;
  return getSlot();
}

/**
 * Searches for an empty sPCB slot.
 *
 * @return index, empty pcb slot. -1 if none are left.
 */
int getSPCB_slot() {
  if(open_sPCB_slots < 1) return -1;
  int slot = curr_sPCB % MAXPROC;
  if(SPDT[curr_sPCB].slot_closed == 0) {
    open_sPCB_slots--;
    return slot;
  }
  curr_sPCB++;
  return getSPCB_slot();
}


/**                                                                             
 * Uses id to get index on mailboxes table.                                           
 *                                                                              
 * @param id, identification number for mailbox.                                          
 *                                                                              
 * @return slot, index in mailboxes table.                     
 */                                                                             
  int getIndex(int id) {                                                     
    return id % MAXMBOX;                                                       
  }

/**                                                                                 
 * Creates a id for MailBox.                                                       
 * Uses a count variable and modulos it by MAXMBOX                                  
 * to check if it is an available index spot on the                                 
 * mailboxes array.                 
 *                                                                                  
 * @return id, a identifaction number that can be moduloed                 
 * to find the location of a mailbox.                                
 */                                                                                 
  int id_gen() {                                                                   
    if(mb_open_slots < 1) return -1;                                                      
    int id = curr_mb++;                                                              
    if(curr_mb > MAXMBOX) curr_mb = 0;
    int index = id % MAXMBOX; // Finds slot                                         
    if (mailboxes[index].slot_closed == 0) { // Slot closed is False                       
      mb_open_slots--;                                                                 
      return id;                                                                   
    }                                                                               
    return id_gen();                                                               
  }

/** ----------------- PSR Functions -------------------*/

/**                                                                                 
 * Check if USLOSS is in kernal mode.                                       
 * Calls USLOSS to halt program if in user mode.                                    
 *                                                                                  
 * @param func, the function name, used for printing error message.                 
 */                                                                                 
void isInKernalMode(char* func) {                       
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
 * Enables user mode.
 */
void userOn() {
  int result;                                                                    
                                                                                 
  result = USLOSS_PsrSet( USLOSS_PsrGet() & (~ USLOSS_PSR_CURRENT_MODE) );       
  if ( result != USLOSS_DEV_OK ) {                                               
      USLOSS_Console("ERROR: Invalid PSR\n");                                            
      USLOSS_Halt(1);                                                            
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

/*----------------------------- Queue functions -----------------------------*/

/*                                                                               
 * Enqueues a Slot in a queue.                                                  
 *                                                                              
 * @param q, the Slot to enqueue the process.                                   
 * @param slot, the Slot added to the q.                                  
 *                                                                              
 */ 
void slotEnqueue(Queue* q, Slot* slot){                                
  //set up first pcb                                                            
  if (q->slot_back == NULL){                                                         
    q->slot_front = slot;                                                         
    q->slot_back = slot;                                                          
  }                                                                             
  else{                                                                         
    //extend queue                                                              
    q->slot_back->qNext = slot;                                                  
    q->slot_back = slot;                                                          
  }                                                                           
  //incremement queue size                                                      
  q->size++;                                                                    
                                                                                
}              

/*                                                           
 * Dequeues the next Slot in a queue                                       
 *                                                                              
 * @param q, address to a queue.                                                   
 *                                                                              
 * @return slot, the slot that got removed from the q.                    
 */
Slot* slotDequeue(Queue* q){                                
  
  //empty queue check                                                           
  if (q->size == 0){                                                            
    //printf("Queue is empty\n");                                               
    return NULL;                                                                
  }                                                                             
  //get next up pcb                                                             
  struct Slot* slot = q->slot_front;                            
  //update front pointer                                                        
  q->slot_front = slot->qNext;                                                   
  //if empty then update front/back                                             
  if (q->slot_front == NULL){                                                        
    q->slot_back = NULL;                                                             
  }
  slot -> qNext = NULL;  
  //decrement size                                                              
  q->size--;                                                                    
                                                                                
  return slot;                                                               
}     


/*                                                                               
 * Enqueues a process in a queue.                                                  
 *                                                                              
 * @param q, the queueue to enqueue the process.                                   
 * @param process, the process added to the q.                                  
 *                                                                              
 */
void enqueue(Queue* q, sPCB* process){                                
  //set up first pcb                                                            
  if (q->back == NULL){                                                         
    q->front = process;                                                         
    q->back = process;                                                          
  }                                                                             
  else{                                                                         
    //extend queue                                                              
    q->back->qNext = process;                                                  
    q->back = process;                                                          
  }                                                                             
  //incremement queue size                                                      
  q->size++;                                                                    
                                                                                
}              

/*                                                           
 * Dequeues a the next up process in a queue                                       
 *                                                                              
 * @param q, address to a queue.                                                   
 *                                                                              
 * @return process, the process that got removed from the q.                    
 */
sPCB* dequeue(Queue* q){                                
  
  //empty queue check                                                           
  if (q->size == 0){                                                            
    return NULL;                                                                
  }                                                                             
  //get next up pcb                                                             
  sPCB* process = q->front;                                                      
  //update front pointer                                                        
  q->front = process->qNext;                                                   
  //if empty then update front/back                                             
  if (q->front == NULL){                                                        
    q->back = NULL;                                                             
  }
  process -> qNext = NULL;  
  //decrement size                                                              
  q->size--;                                                                    
                                                                                
  return process;                                                               
}      

/**                                                                   
 * Returns the front Slot of the q but does not remove it.                    
 *                                                                                  
 * @param q, the q to peek at.                                             
 */                                                                                 
Slot* slotPeek(Queue * q) {
  return q->slot_front;                                                                 
}


/**                                
 * Returns the front sPCB of the q but does not remove it.                    
 *                                                                                  
 * @param q, the q to peek at.                                             
 */                                                                                 
sPCB* peek(Queue * q) {
  return q->front;                                                                 
}


