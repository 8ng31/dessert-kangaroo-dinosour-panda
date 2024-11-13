// CSc 422, Spring 2024
// Program 2 code for sequential myMalloc

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "myMalloc-helper.h"

// total amount of memory to allocate, and size of each small chunk
#define SIZE_TOTAL 276672
#define SIZE_SMALL 64
#define SIZE_LARGE 1024

pthread_key_t key;
pthread_mutex_t idLock;
pthread_mutex_t mmLock;
int newId = 1;
int idArray[9]; 
int seenId[9] = {0,0,0,0,0,0,0,0,0}; 
int fine;

// maintain lists of free blocks and allocated blocks
typedef struct memoryManager {
  chunk *freeList;
  chunk *allocList;
} memManager;

memManager *mmSmall[9];
memManager *mmLarge[9];

void* memArray[9];


// note that flag is not used here because this is only the sequential version
int myInit(int numCores, int flag) {
  fine = (2 == flag);
  int numSmall, numLarge;
  pthread_mutex_init(&idLock, NULL); 
  pthread_mutex_init(&mmLock, NULL); 
  pthread_key_create(&key, NULL);

  // allocate SIZE_TOTAL of memory that will be split into small chunks below
  // Takes into account corse grain or fine grain allocation using loop
  int stop = 0;
  if (flag == 2) stop = numCores;
  for (int i = 0; i <= stop; i++) { 
    memArray[i] = malloc(SIZE_TOTAL); 
    if (memArray[i] == NULL) { // make sure malloc succeeded
      return -1;
    } 
  }

  // Create head nodes for threads
  for (int i = 0; i <= numCores; i++) { 
    // set up memory manager for small list
    mmSmall[i] = (memManager *) malloc(sizeof(memManager));
    mmSmall[i]->freeList = createList();
    mmSmall[i]->allocList = createList();

    mmLarge[i] = (memManager *) malloc(sizeof(memManager));
    mmLarge[i]->freeList = createList();
    mmLarge[i]->allocList = createList();
  }

  for (int i = 0; i <= stop; i++) { 
    numSmall = (SIZE_TOTAL/2) / (SIZE_SMALL + sizeof(chunk));
    numLarge = (SIZE_TOTAL/2) / (SIZE_LARGE + sizeof(chunk));
    void *memLarge = (void *) ((char *) memArray[i] + (SIZE_TOTAL/2));  
    // set up free list chunks
    setUpChunks(mmSmall[i]->freeList, memArray[i], numSmall, SIZE_SMALL);
    setUpChunks(mmLarge[i]->freeList, memLarge, numLarge, SIZE_LARGE);
  }
  return 0;
}

void touchFile(int id) {
  char str[64];
  if(fine && !seenId[id]) {
    if(id) {
    sprintf(str, "touch Id-%d\n", id);    
    } else {
    sprintf(str, "touch Overflow\n");    
    }
    system(str);
    seenId[id] = 1;
  }
 
}

// myMalloc just needs to get the next chunk and return a pointer to its data
// note the pointer arithmetic that makes sure to skip over our metadata and
// return the user a pointer to the data
void *myMalloc(int size) {
  int danger = 0; // Flag used for overlow list
  int *my_id = (int *) pthread_getspecific(key);
  
  // Create ID for thread
  if (my_id == NULL) { 
    pthread_mutex_lock(&idLock);
    my_id = malloc(sizeof(int));
    *my_id = newId++;
    idArray[*my_id] = *my_id;
    pthread_setspecific(key, (const void *)(&idArray[*my_id]));
    pthread_mutex_unlock(&idLock);
  }
  
  touchFile(*my_id);

  // get a chunk
  chunk *toAlloc;
  memManager *mMan = mmSmall[*my_id];
  if(size > 64) mMan = mmLarge[*my_id];
  
  // Check if overflow list is needed
  if(isEmpty(mMan->freeList)) {
    danger = 1;
    mMan = mmSmall[0];
    if(size > 64) mMan = mmLarge[0];
    touchFile(0);
  }

  // Incase Overflow or sequential mem is used.
  if(danger) pthread_mutex_lock(&mmLock);
  toAlloc = getChunk(mMan->freeList, mMan->allocList);     
  if(danger) pthread_mutex_unlock(&mmLock);

  return ((void *) ((char *) toAlloc) + sizeof(chunk));
}

// myFree just needs to put the block back on the free list
// note that this involves taking the pointer that is passed in by the user and
// getting the pointer to the beginning of the chunk (so moving backwards chunk bytes)
void myFree(void *ptr) {
  int *my_id = (int *) pthread_getspecific(key);
  int danger = 0;
  // find the front of the chunk
  chunk *toFree = (chunk *) ((char *) ptr - sizeof(chunk));
  int size = toFree->allocSize;
  memManager *mMan = mmSmall[*my_id];
  if(size > 64) mMan = mmLarge[*my_id];

  // Check if overflow list is needed
  if(isEmpty(mMan->allocList)) {
    danger = 1;
    mMan = mmSmall[0];
    if(size > 64) mMan = mmLarge[0];
    touchFile(0);
  }
 
  if(danger) pthread_mutex_lock(&mmLock);
  returnChunk(mMan->freeList, mMan->allocList, toFree);
  if(danger) pthread_mutex_unlock(&mmLock);
}
