// CSc 422, Spring 2024
// Program 2 code for sequential myMalloc-helper

#include <stdlib.h>
#include <stdio.h>
#include "myMalloc-helper.h"

// list routines: implements a circular, doubly-linked list with a dummy header node

// create the list; just allocate a dummy header node, and both next and prev
// point to the header node
chunk *createList() {
  chunk *dummy = (chunk *) malloc(sizeof(chunk));
  dummy->allocSize = -1;  // this is the header node, not a real list node
  dummy->next = dummy;
  dummy->prev = dummy;
  return dummy;
}

// insert curItem into the list after item
void insertAfter(chunk *item, chunk *curItem) {
  curItem->next = item->next;
  curItem->prev = item;
  curItem->prev->next = curItem;
  curItem->next->prev = curItem;
}

// insert curItem into the list before item
void insertBefore(chunk *item, chunk *curItem) {
  insertAfter(item->prev, curItem);
}

// insert chunk at end of list
void listAppend(chunk *list, chunk *item) {
  insertAfter(list->prev, item);
}

// insert chunk at beginning of list
void listPush(chunk *list, chunk *item) {
  insertBefore(list->next, item);
}

// remove item from the list
void unlinkItem(chunk *item) {
  item->prev->next = item->next;
  item->next->prev = item->prev;
}

// return the first list element
chunk *listFront(chunk *list) {
  return list->next;
}

// end of list routines

// take a block of memory and logically divide it into chunks
// note that blocksize is here because it's necessary when you add
// large blocks
void setUpChunks(chunk *list, void *mem, int num, int blockSize) {
  int i;
  int chunkSize = sizeof(chunk) + blockSize;

  for (i = 0; i < num; i++) {
    // note below mem is cast to a (char *), so we are adding i * chunksize bytes 
    chunk *currentChunk = (chunk *) ((char *) mem + i * chunkSize);  // start position of this chunk
    currentChunk->allocSize = blockSize;	
    listAppend(list, currentChunk);  // put this chunk at the end of the list
  }
}

// used to move a block from free list to allocated list or vice versa
void moveBetweenLists(chunk *toMove, chunk *fromList, chunk *toList) {
  // remove toMove from the source list
  unlinkItem(toMove);
  // now push it on to the destination list	
  listPush(toList, toMove);
}

// get a chunk---grab first available chunk, put on allocated list, and return pointer to the chunk
chunk *getChunk(chunk *freeList, chunk *allocList) {
  chunk *toAlloc = listFront(freeList);
  moveBetweenLists(toAlloc, freeList, allocList);
  return toAlloc;
}

// return a chunk from the allocated list to the free list
void returnChunk(chunk *freeList, chunk *allocList, chunk *toFree) {
  moveBetweenLists(toFree, allocList, freeList);
}

// returns 1 if chunk list is empty
int isEmpty(chunk *list) {
  if (list->next == list) return 1;
  return 0;
}
