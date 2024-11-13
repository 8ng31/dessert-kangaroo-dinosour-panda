// eventually put code in here, e.g., list operations

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "dht.h"
#include "dht-helper.h"

// Merge arrays and return new one. Free memory,
int* mergeArrays(int *child, int childSize, int *parent, int parentSize) {
  
  int size = childSize + parentSize;
  int *mergedArray = (int*) malloc(size * sizeof(int));

  // Copy lower id's first coming from parent node.
  memcpy(mergedArray, parent, parentSize * sizeof(int));

  // Copy higher id's from child.
  memcpy(mergedArray + parentSize, child, childSize * sizeof(int));
  
  free(child);
  free(parent);
  return mergedArray;

}
