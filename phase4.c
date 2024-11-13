 /*                                                                          
 * Author: Angel Benavides                                        
 * File: phase4.c                                                                  
 * Assignment: phase 4                                                             
 * Purpose: Implement syscalls for disk, terminal, & clock devices. Implements
 * deamon processes to maintain disks, terminals, & clock device. 
 * Key to communication between daemon & processes is done by using mailboxes.
 * For clock a mailbox is used to block a process until the deaomon process 
 * wakes it up.
 * Queues are used as a from of mutex in Disk and Terminal Devices. Each
 * process must enter a Queue to get access to the device use it for requests
 * or taking data that was saved from it. Only the process at the front of the 
 * Queue is able to do this.
 * Terminals also utilize mailboxes for reading and writing by sending
 * characters and buffer pointers to and from the Deamon.
 * Disk Queues have a special feature that sort the processes mailboxes in
 * order of track number. This is done by creating a serial number like so
 * "(track * CONVERT) + mailboxId" then adding it to the Queue were it is
 * sorted by dividing by convert and comparing the numbers. The lower numer is 
 * moved to the front. CONVERT is in the h file and is set to 10000.
 */

#include "myphase4.h"

int clockInterruptCount = 0;

Queue readerQ0;
Queue readerQ1;
Queue readerQ2;
Queue readerQ3;

Queue writerQ0;
Queue writerQ1;
Queue writerQ2;
Queue writerQ3;


Queue diskQueues[2][2];

int diskWakeUp[2];
int currTracks[2] = {-1, -1};


BufferStorage t0Buffers;
BufferStorage t1Buffers;
BufferStorage t2Buffers;
BufferStorage t3Buffers;

AlarmList sleepers;

USLOSS_DeviceRequest diskReq[2];
int diskTracks[2] = {-1, -1};

/**
 * Create any data structures or initialize and variables needed for program
 * functionality.
 */
void phase4_init(void){
    memset(&sleepers, 0, sizeof(sleepers));
    systemCallVec[SYS_SLEEP] = sys_sleep;
    systemCallVec[SYS_TERMREAD] = sys_termread; 
    systemCallVec[SYS_TERMWRITE] = sys_termwrite; 
    systemCallVec[SYS_DISKSIZE] = sys_disksize; 
    systemCallVec[SYS_DISKREAD] = sys_diskread;
    systemCallVec[SYS_DISKWRITE] = sys_diskwrite;
    int val = 0x2;
    val |= 0x4;
    void * cr_val = (void*)(long) val;
    int code0 = USLOSS_DeviceOutput(USLOSS_TERM_DEV, 0, cr_val);
    int code1 = USLOSS_DeviceOutput(USLOSS_TERM_DEV, 1, cr_val);
    int code2 = USLOSS_DeviceOutput(USLOSS_TERM_DEV, 2, cr_val);
    int code3 = USLOSS_DeviceOutput(USLOSS_TERM_DEV, 3, cr_val);
    if(code0 != 0 || code1 != 0 || code2 != 0 || code3 != 0) print("Error\n"); 
}

/**
 * Creates any process needed for program functionalityi.
 * All daemon processes used to maintain terminal and clock devices.
 */
void phase4_start_service_processes(void) {
  fork1("sleep_handler", sleep_handler, NULL, USLOSS_MIN_STACK, 1);
  fork1("terminalZero", terminalZero, NULL, USLOSS_MIN_STACK, 1); 
  fork1("terminalOne", terminalOne, NULL, USLOSS_MIN_STACK, 1); 
  fork1("terminalTwo", terminalTwo, NULL, USLOSS_MIN_STACK, 1);
  fork1("terminalThree", terminalThree, NULL, USLOSS_MIN_STACK, 1);
  fork1("diskZero", disk, "0", USLOSS_MIN_STACK, 1);
  fork1("diskOne", disk, "1", USLOSS_MIN_STACK, 1);
}

/*--------------------------- syscalls --------------------------------------*/

/**
 * System call for disk read. Extracts args from user code to implement a
 * request to Disk Device for a Read. Creates mailbox for syscall to be woken
 * up. Checks if the Disk device is busy. If the Disk device is not busy it 
 * will make a request immediately, then put its mailbox into a queue that the
 * deamon uses to wake processes up. If the device is busy the syscall syscall
 * adds its self to the queue that the daemon uses to wake processes. In either
 * case the function waits to recieve a message from the daemon before it makes
 * a request or finishes.
 *
 * @param args, parameters for Device request
 */
void sys_diskread(USLOSS_Sysargs *args){
  char *buff = (char*) args->arg1; //buffer pointer
  int sectors = (int)(long)args->arg2; //number of sectors/blocks to read
  int track = (int)(long)args->arg3; //starting track number
  int block = (int)(long)args->arg4; //starting block number
  int unit = (int)(long)args->arg5; //which disk to access = unit

  int transferStatus = 0;
  args->arg1 = (void*)(long) transferStatus; //0 if transfer success; otherwise->=disk status
  args->arg4 = (void*)(long) transferStatus; //-1 if illegal values given as input; otherwise->0
  if(unit > 1 || unit < 0 || block > USLOSS_DISK_TRACK_SIZE || block < 0 || track > 1700 || track < 0) {
    args->arg1 = (void*)(long) USLOSS_DEV_ERROR;
    args -> arg4 = (void *)(long) -1;
    return;
  }
  int mailbox = MboxCreate(1, 0);  
  if(isEmpty(&diskQueues[unit][0])) {
    readReq:
    if (track != currTracks[unit]){
      diskSeek(unit, track); //seek to correct track
    }
    //seeking might take time and might cause timing lag in future tests
    for (int i = block; i < block + sectors; i++){
      int currentBlock = i % 16;
      diskReq[unit].opr = USLOSS_DISK_READ;
      diskReq[unit].reg1 = (void*)(long) currentBlock;// * 512;
      diskReq[unit].reg2 = buff + (512 * (i - block));
      USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &diskReq[unit]);
      enqueueDisk(unit, (track * CONVERT) + mailbox);
      MboxRecv(mailbox, NULL, 0);
    }
  } else {
    enqueueDisk(unit, (track * CONVERT) + mailbox);
    MboxRecv(mailbox, NULL, 0);
    goto readReq;
  }
}

/**
 * System call for disk write. Extracts args from user code to implement a
 * request to Disk Device for a Write. Creates mailbox for syscall to be woken
 * up. Checks if the Disk device is busy. If the Disk device is not busy it 
 * will make a request immediately, then put its mailbox into a queue that the
 * deamon uses to wake processes up. If the device is busy the syscall syscall
 * adds its self to the queue that the daemon uses to wake processes. In either
 * case the function waits to recieve a message from the daemon before it makes
 * a request or finishes.
 *
 * @param args, parameters for Device request
 */
void sys_diskwrite(USLOSS_Sysargs *args){
  char *buff = (char*) args->arg1; //buffer pointer
  int sectors = (int)(long)args->arg2; //number of sectors/blocks to read
  int track = (int)(long)args->arg3; //starting track number
  int block = (int)(long)args->arg4; //starting block number
  int unit = (int)(long)args->arg5; //which disk to access = unit
  
  int transferStatus = 0;
  args->arg1 = (void*)(long) transferStatus; //0 if transfer success; otherwise->disk status
  args->arg4 = (void*)(long) transferStatus; //-1 if illegal values given as input; otherwise->0
  
  if(unit > 1 || unit < 0 || block > USLOSS_DISK_TRACK_SIZE || block < 0 || track < 0) {
    args -> arg4 = (void *)(long) -1;
    return;
  }
  if (track > 1700){ //track seems plausable but is too high!
    args->arg1 = (void*)(long) USLOSS_DEV_ERROR;
    return;
  }
 
  int mailbox = MboxCreate(1, 0);

  if(isEmpty(&diskQueues[unit][0])) {
    writeReq:
    if (track != currTracks[unit]) diskSeek(unit, track); //seek to correct track

    for (int i = block; i < block + sectors; i++){
      int currentBlock = i % 16;
      
      diskReq[unit].opr = USLOSS_DISK_WRITE;
      diskReq[unit].reg1 = (void*)(long) currentBlock;// * 512;
      diskReq[unit].reg2 = buff + (512 * (i - block));
      USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &diskReq[unit]);

      enqueueDisk(unit, (track * CONVERT) + mailbox);
      MboxRecv(mailbox, NULL, 0);
    } 
  } else {
    enqueueDisk(unit, (track * CONVERT) + mailbox);
    MboxRecv(mailbox, NULL, 0);
    goto writeReq;
  }  
}

/**
 * Deamon used for waking up processes that use the Disk device. The Deamon
 * uses two queues to implement the C-Scan algorithm. The Queues are in a two
 * dimensional array that have 1 array of 2 queues for each Disk device, called
 * diskQueues. The first index for accessing a devices queus can be the unit
 * number of the device. The deamon always chooses the queue at index 0 of each
 * device. If the queue becomes empty and the queue at index 1 has content, a
 * helper function will switch out the Queues. When the Disk device has
 * completed a request the deamon dequeues a mailbox of the process that made
 * the request and sends a message to wake it up.
 *
 * @param diskUnit, unit of device. 
 */
int disk(char *diskUnit) {
  int unit = atoi(diskUnit);

  int diskStatus; 
  initializeQueue(&diskQueues[unit][0]);
  initializeQueue(&diskQueues[unit][1]);
  while(1) {
    waitDevice(USLOSS_DISK_INT, unit, &diskStatus);
    cScan:
    while(!isEmpty(&diskQueues[unit][0])) {
      int mailbox = dequeue(&diskQueues[unit][0]);
      MboxCondSend(mailbox % CONVERT, NULL, 0);
    }
    if(isEmpty(&diskQueues[unit][0]) && !isEmpty(&diskQueues[unit][1])) {
      swapQueues(unit);
      goto cScan;  
    }
  }
}

/**
 * Swaps two Queue structs of a device. A helper function to switch out an
 * empty q in use to another. Used to implment C-Scan algorithm.
 *
 * @param unit, the device unit.
 */
void swapQueues(int unit) {
  Queue temp;
  memcpy(&temp, &diskQueues[unit][0], sizeof(Queue));
  memcpy(&diskQueues[unit][0], &diskQueues[unit][1], sizeof(Queue));
  memcpy(&diskQueues[unit][1], &temp, sizeof(Queue));
}

/**
 * Helper funciton used to completed a Seek request to Disk device.
 * Creates a mailbox that is used to block itself and wait for the disk Deamon
 * to wake it up.
 *
 * @param unit, the device unit.
 * @param track, the track to seek.
 */
void diskSeek(int unit, int track){                                             
  diskReq[unit].opr = USLOSS_DISK_SEEK;                                       
  diskReq[unit].reg1 = (void*)(long) track;                                                 

  int mailbox = MboxCreate(1, 0);                                                  
  USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &diskReq[unit]);        
                                                                                   
  enqueueDisk(unit,(track * CONVERT) + mailbox);                                                 
  MboxRecv(mailbox, NULL, 0);                                                      
  diskTracks[unit] = track;
}

/**
 * Creates a request to Disk device unit for the number of tracks it currently
 * has. Checks if device is busy by checking if the queue assigned to the disk
 * is currently empty. If it is empty then it makes a request and adds its
 * mailbox to the queue and waits for a message to be recieved. The deamon will
 * wake up the process once the request is completed. Function saves the result
 * in a table incase the the user wants to ask for size again. Eliminates the
 * need for another request to Disk device.
 *
 * @param args, struct with parameters for request.
 */
void sys_disksize(USLOSS_Sysargs *args) {                                        
  int unit = (int)(long)args->arg1; //the disk to query                         
                                                                                
  //if illegal values given as input                                            
  if(unit > 1 || unit < 0) {                                                    
    args -> arg4 = (void *)(long) -1;                                           
    return;                                                                     
  }
  // Prevents unneccesary size requests to disk.  
  if(diskTracks[unit] != -1) goto trackSize;
  int mailbox = MboxCreate(1, 0);
  
  if(isEmpty(&diskQueues[unit][0])) { // Checks if device is busy.
    sizeReq:
    diskReq[unit].opr = USLOSS_DISK_TRACKS;
    diskReq[unit].reg1 =  &diskTracks[unit];
    USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &diskReq[unit]);
    enqueueDisk(unit, (1776 * CONVERT) + mailbox);
    MboxRecv(mailbox, NULL, 0);
  } else { // Waits for device to be free.
    enqueueDisk(unit, (1776 * CONVERT) + mailbox);
    MboxRecv(mailbox, NULL, 0);
    goto sizeReq;
  }  
  int diskSize;                                                                 
  int blockSize = USLOSS_DISK_SECTOR_SIZE;                                      
  int numBlocks = USLOSS_DISK_TRACK_SIZE;
  trackSize:  
  diskSize = diskTracks[unit];                                       
  args->arg1 = (void*)(long) blockSize; //size of block in bytes 
  args->arg2 = (void*)(long) numBlocks; //number of blocks in track
  args->arg3 = (void*)(long) diskSize; //number of tracks in disk
  args -> arg4 = (void *)(long) 0;                                     
}



/**
 * Syscall from user code.
 * Reads a line of characters from a terminal.
 * Creates a mailbox that is added to a Q of the specific termial. The function
 * asks for a message from the created mailbox. The daemon for the terminal
 * will send back a buffer when it get allocated. 
 *
 * @param args, the arguements from user code.
 */
void sys_termread(USLOSS_Sysargs *args) {
  char *buff = (char *) args -> arg1;
  int length = (int) (long) args -> arg2;
  int unit = (int) (long) args -> arg3; 
  if(length < 1 || length > MAXLINE || unit > 3 || unit < 0) {
    args -> arg4 = (void *)(long) -1;
    return;
  }
  BufferStorage *storage;
  int mb = MboxCreate(1, MAXLINE + 1);
  char msg[MAXLINE + 1] = ""; 
  Queue *q;
  // Pick the terminal q to enter mailbox.
  if(unit == 0) {
    storage = &t0Buffers;
    q = &readerQ0;
  }
  if(unit == 1) {
    storage = &t1Buffers;
    q = &readerQ1;
  }
  if(unit == 2) {
    storage = &t2Buffers;
    q = &readerQ2;
  }
  if(unit == 3) {
    storage = &t3Buffers;
    q = &readerQ3;
  }
  if(isEmpty(q) && isReadyToShip(storage)) {
    getBuffer(storage, msg);  
  } else {
    enqueue(q, mb); // Add to q
    MboxRecv(mb, &msg, MAXLINE);
    MboxRelease(mb); // Destroy mailbox.
  }
  int i = 0;
  while(i < length && msg[i] != '\0') { // Copy buffer into functions buffer.
    buff[i] = msg[i];
    i++;
  }
  args -> arg2 = (void *) (long) i;
  args -> arg4 = 0;
}

/**
 * Flushes a given buffer to a terminal device.
 * Creates a mailbox that is used to send characters to daemon process.
 * The mailbox is inserted into a Q for the terminal and once selected the
 * termminal device will recieve messages from it. The function will send
 * messages until it sends every character from the buffer.
 *
 * @param args, parameters for device.
 */
void sys_termwrite(USLOSS_Sysargs *args) {
  char *buff = (char *) args -> arg1;
  int length = (int) (long) args -> arg2;
  int unit = (int) (long) args -> arg3;
  if(length < 1 || length > MAXLINE + 1 || unit > 3 || unit < 0) {
    args -> arg4 = (void *)(long) -1;
    return;
  }
  int mb = MboxCreate(1, MAXLINE + 1);
  Queue *q;

  // Select q to add mailbox too.
  if(unit == 0) q = &writerQ0;
  if(unit == 1) q = &writerQ1;
  if(unit == 2) q = &writerQ2;
  if(unit == 3) q = &writerQ3;

  enqueue(q, mb); // Add to q
  int i = 0;
  while(i < length) {   
    MboxSend(mb, &buff[i], 1);
    i++;
  }
  char newline = '\n';
  MboxSend(mb, &newline, 1);
  dequeue(q); // Remove from q to avoid errors with daemon.
  MboxRelease(mb);
  
  args -> arg2 = (void *) (long) i;
  args -> arg4 = 0;
}

/**
 * Puts a process to sleep for a certain amount of secondes. Converts seconds
 * into clock interrupts that will pass. Uses mailbox to go to sleep. Sleep
 * Deamon wakes up process after the number of clock interrupts have occured.
 * This is done by using a queue that takes a struct with the clock interrupt
 * time and the mailbox id.
 *
 * @param args, parameters for request. 
 */
void sys_sleep(USLOSS_Sysargs *args){
    int seconds = (int)(long) args -> arg1;
    int clockInterrupts = seconds * (1000/100);
    int wakeupTime = clockInterruptCount + clockInterrupts;
    if (clockInterrupts > 0){
        int mb = MboxCreate(1, 0);
        Alarm a = {wakeupTime, mb, NULL}; 
        addAlarm(&a);
        MboxRecv(mb, NULL, 0);
        MboxRelease(mb);
        args -> arg4 = (void *)(long) 0;
    }
    else{
        args -> arg4 = (void *)(long) -1;
    }
}

/*---------------------------- DEAMONS --------------------------------------*/

/*
 * A deamon used for completing maintence on the clock device. Wakes up
 * processes that have been blocked for a certain amount of requested clock
 * interrupts. 
 */
int sleep_handler(char *null){
    int clockStatus;
    while(1){
        waitDevice(USLOSS_CLOCK_INT, 0, &clockStatus);
        clockInterruptCount++;
        Alarm* alarm = sleepers.front;
        while(alarm != NULL && clockInterruptCount >= alarm -> wakeupTime) {
          alarm = sleepers.front;
          removeAlarm(sleepers.front); 
          MboxCondSend(alarm->mailbox, NULL, 0);     
          alarm = sleepers.front;
        }
    }
}

/** TERMINAL processes description
 * Terminal[Zero - Three] are all the same functions with different global Q's
 * inserted into them to choose between different mailboxes for char writing
 * and reading. Each one has two q's assigned to them readerQ* & writerQ*. The
 * processes send and receive characters to the mailboxes in the q's. Each
 * mailbox is to a specific process. It creates mutex so the processes cannot
 * use the terminals at the same time. Specifically more than one process
 * cannot write at the same time to the terminal or read from it.
 *
 * Each processes has a struct BufferStorage for storing characters read. The
 * struct holds a maximum of 10 buffers of MAXLINE characters. Assiting
 * functions help flush buffers from the storage. 
 */

// Refer to TERMINAL processes description on line 180.
int terminalZero(char *null) {
  initializeQueue(&readerQ0);
  initializeQueue(&writerQ0);
  memset(&t0Buffers, 0, sizeof(t0Buffers));
  while(1) {
    int status;
    waitDevice(USLOSS_TERM_DEV, 0, &status);
    int xmit = USLOSS_TERM_STAT_XMIT(status);
    int recv = USLOSS_TERM_STAT_RECV(status);
    if(recv == USLOSS_DEV_BUSY) {
      char c = (char) USLOSS_TERM_STAT_CHAR(status); 
      addToBuffer(&t0Buffers, c);
    }
    if(!isEmpty(&readerQ0) && isReadyToShip(&t0Buffers)) {
      shipOut(&t0Buffers, dequeue(&readerQ0));
    }
    if(xmit == USLOSS_DEV_READY && !isEmpty(&writerQ0)) { 
      int mb = peek(&writerQ0);
      char c;
      int code = MboxCondRecv(mb, &c, 1);
      if(code ==  -2) {
        dequeue(&writerQ0);
      } else if(c != '\0') {
        int cr_val = 0x7;
        cr_val |= (c << 8);
        int s = USLOSS_DeviceOutput(USLOSS_TERM_DEV, 0, (void*)(long) cr_val);
        if(s) print("Error\n");
      }

    }
  }
}

// Refer to TERMINAL processes description on line 180.
int terminalOne(char *null) {
  initializeQueue(&readerQ1);
  initializeQueue(&writerQ1);
  memset(&t1Buffers, 0, sizeof(t1Buffers));
  while(1) {
    int status;
    waitDevice(USLOSS_TERM_DEV, 1, &status);
    int xmit = USLOSS_TERM_STAT_XMIT(status);
    int recv = USLOSS_TERM_STAT_RECV(status);
    if(recv == USLOSS_DEV_BUSY) {
      char c = (char) USLOSS_TERM_STAT_CHAR(status); 
      addToBuffer(&t1Buffers, c);
    }
    if(!isEmpty(&readerQ1) && isReadyToShip(&t1Buffers)) {
      shipOut(&t1Buffers, dequeue(&readerQ1));
    }
    if(xmit == USLOSS_DEV_READY && !isEmpty(&writerQ1)) {
      int mb = peek(&writerQ1);
      char c;
      int code = MboxCondRecv(mb, &c, 1);
      if(code ==  -2) {
        dequeue(&writerQ1);
      } else {
        int cr_val = 0x7;
        cr_val |= (c << 8);
        int s = USLOSS_DeviceOutput(USLOSS_TERM_DEV, 1, (void*)(long) cr_val);
        if(s) print("Error\n");
      }
    }
  }
}

// Refer to TERMINAL processes description on line 180.
int terminalTwo(char *null) {
  initializeQueue(&readerQ2);
  initializeQueue(&writerQ2);
  memset(&t2Buffers, 0, sizeof(t2Buffers));
  while(1) {
    int status;
    waitDevice(USLOSS_TERM_DEV, 2, &status);
    int xmit = USLOSS_TERM_STAT_XMIT(status);
    int recv = USLOSS_TERM_STAT_RECV(status);
    if(recv == USLOSS_DEV_BUSY) {
      char c = (char) USLOSS_TERM_STAT_CHAR(status); 
      addToBuffer(&t2Buffers, c);
    }
    if(!isEmpty(&readerQ2) && isReadyToShip(&t2Buffers)) {
      shipOut(&t2Buffers, dequeue(&readerQ2));
    }
    if(xmit == USLOSS_DEV_READY && !isEmpty(&writerQ2)) { 
      int mb = peek(&writerQ2);
      char c;
      int code = MboxCondRecv(mb, &c, 1);
      if(code ==  -2) {
        dequeue(&writerQ2);
      } else {
        int cr_val = 0x7;
        cr_val |= (c << 8);
        int s = USLOSS_DeviceOutput(USLOSS_TERM_DEV, 2, (void*)(long) cr_val);
        if(s) print("Error\n");
      }
    }
  }
}

// Refer to TERMINAL processes description on line 180.
int terminalThree(char *null) {
  initializeQueue(&readerQ3);
  initializeQueue(&writerQ3);
  memset(&t3Buffers, 0, sizeof(t3Buffers));
  while(1) {
    int status;
    waitDevice(USLOSS_TERM_DEV, 3, &status);
    int xmit = USLOSS_TERM_STAT_XMIT(status);
    int recv = USLOSS_TERM_STAT_RECV(status);
    if(recv == USLOSS_DEV_BUSY) {
      char c = (char) USLOSS_TERM_STAT_CHAR(status); 
      addToBuffer(&t3Buffers, c);
    }
    if(!isEmpty(&readerQ3) && isReadyToShip(&t3Buffers)) {
      shipOut(&t3Buffers, dequeue(&readerQ3));
    }
    if(xmit == USLOSS_DEV_READY && !isEmpty(&writerQ3)) { 
      int mb = peek(&writerQ3);
      char c;
      int code = MboxCondRecv(mb, &c, 1);
      if(code ==  -2) {
        dequeue(&writerQ3);
      } else {
        int cr_val = 0x7;
        cr_val |= (c << 8);
        int s = USLOSS_DeviceOutput(USLOSS_TERM_DEV, 3, (void*)(long) cr_val);
        if(s) print("Error\n");
      }
    }
  }
}

/* ------------------------ Linked List Functions --------------------------*/

/**
 * Adds alarm to list.
 * Starts at the front of linked list and moves back if alarm has greater time
 * than each alarm node in the list.
 *
 * @param alarm, struct to add to linked list.
 */
void addAlarm(Alarm *alarm) {
  if(sleepers.front == NULL) {
    sleepers.front = alarm;
    sleepers.size++;
    return;
  }
  if(alarm->wakeupTime <= sleepers.front->wakeupTime) {
    alarm -> next = sleepers.front;
    sleepers.front = alarm;
    sleepers.size++;
    return;
  }

  Alarm *current = sleepers.front;
  while (current->next != NULL && alarm->wakeupTime >= current->next->wakeupTime) {
        current = current->next;
    }

  alarm->next =current->next;
  current->next = alarm;
  sleepers.size++;
}


/**
 * Removes struct at the front of a linked list.
 *
 * @param head, front of a linked list.
 */
void removeAlarm(Alarm *head) {
  if(sleepers.front == NULL) return;
  sleepers.front = sleepers.front->next;
  sleepers.size--;
}

/* ------------------------- Queue functions -------------------------*/

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
 * Adds mailbox to front of Reader Queue.
 *
 * @param queue, pointer ot a q.
 * @param mailbox, id of a mailbox
 */
void enqueue(Queue *queue, int mailbox) {
  if (isFull(queue)) return;

  if (queue->front == -1) {
      queue->front = 0;
  }

  queue->tail = (queue->tail + 1) % MAXPROC;
  queue->array[queue->tail] = mailbox;
}

/**
 * Function specifically used for adding mailbox id's to the Disk Queue. It
 * makes a conversion on the mailbox id's to check which queue to add too. The
 * queue to add to will depend on if the mailbox priority has passed or not.
 * The priority is the track number on the disk. The mailbox is then enqueued
 * onto the determined queue and then a helper function called swim is called
 * to move the mailbox up in the queue to be in order of priority.
 */
void enqueueDisk(int unit, int mbId) {
  Queue *q;
  int front = peek(&diskQueues[unit][0]);
  if((mbId / CONVERT) >= (front / CONVERT) && !isFull(&diskQueues[unit][0])) {
    q = &diskQueues[unit][0]; 
  } else {
    q = &diskQueues[unit][1];
  }
  enqueue(q, mbId);
  swim(q);  
}

/**
 * Starts at the last element in the q and moves it up until it is in order of
 * priority in the queue.
 */
void swim(Queue *q) {
  int *array = q->array;
  int front = q->front;
  int curr = q->tail;
  int other;
  while(curr != front) {
    other = (curr - 1) % MAXPROC;
    if(array[curr] <= array[other]) {
      swap(array, curr, other);
      curr = (curr - 1) % MAXPROC;
    } else {
      break;
    }
  }
}

/**
 * Switches two elements in an array.
 *
 * @param arr, array
 * @param i, index in array
 * @param j, index in array
 */
void swap(int *arr, int i, int j) {
  int temp = arr[i];
  arr[i] = arr[j];
  arr[j] = temp;
}

/**
 * Returns mailbox of reader in front of q.
 *
 * @param queue, pointer to a q.
 */
int dequeue(Queue *queue) {
  int mailbox;
  if (isEmpty(queue)) return -1;

  mailbox = queue->array[queue->front];
  if (queue->front == queue->tail) {
      queue->front = queue->tail = -1;
  } else {
      queue->front = (queue->front + 1) % MAXPROC;
  }
  return mailbox;
}

/**
 * Returns int at top of q.
 *
 * @param queue, pointer to a q.
 */
int peek(Queue *queue) {
  int mailbox;
  if (isEmpty(queue)) return -1;
  mailbox = queue->array[queue->front];
  return mailbox;
}

/**
 * Checks if Queue is empty.
 *
 * @param queue, pointer to a q.
 */
int isEmpty(Queue *queue) {
  return (queue->front == -1);
}

/* ------------------------- Buffer Functions -------------------------------*/

/**
 * Retreves a buffer from a BufferStorage and sends it to a mailbox,
 *
 * @param storage, buffer storage
 * @param mailbox, id of mailbox to send to.
 */
void shipOut(BufferStorage *storage, int mailbox) {
  Buffer *buffer = &storage->buffers[storage->shipOut];
  MboxSend(mailbox, buffer->buff, strlen(buffer->buff));
  memset(buffer, 0, sizeof(*buffer));

  // Choose next buffer to send
  storage->shipOut = (storage->shipOut + 1) % MAXBUFFS;
  storage->full = 0;
}

/**
 * Grabs a buffer from a storage struct. The buffer that has been sitting in
 * the storage strcut the longest.
 *
 * @param storage, a struct containing buffers
 * @param msg, the buffer address to copy the buffer into.
 */
void getBuffer(BufferStorage *storage, char *msg) {
  Buffer *buffer = &storage->buffers[storage->shipOut];
  //print("Sending msg: %s\n", buffer->buff);
  strcpy(msg, buffer->buff);
  memset(buffer, 0, sizeof(*buffer));

  // Choose next buffer to send
  storage->shipOut = (storage->shipOut + 1) % MAXBUFFS;
  storage->full = 0;
}

/**
 * Checks if a BufferStorage has a buffer to send.
 *
 * @param storage, struct with buffers.
 */
int isReadyToShip(BufferStorage *storage) {
  Buffer *buffer = &storage->buffers[storage->shipOut];
  return (buffer->ready == 1);
}

/**
 * Adds a character to a buffer storage. The function looks for space in the 10
 * buffers within the struct to place it.
 * 
 * @param storage, struct with buffers
 * @param c, character to add
 */
void addToBuffer(BufferStorage *storage, char c) {
  int flag = 0;
  if(storage->full) return;
  Buffer *buffer = &storage->buffers[storage->selected];
  if(buffer->ready) return;
  buffer->buff[buffer->index++] = c;
  if(c == '\n') flag++;
  if(buffer->index == MAXLINE) {
    buffer->buff[buffer->index] = '\0';
    flag++;
  }

  if(flag) {
    buffer->ready++;
    storage->selected = (storage->selected + 1) % MAXBUFFS;
    if(storage->selected == storage->shipOut) storage->full++; 
  }
}
