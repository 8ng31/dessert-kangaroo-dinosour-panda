#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "dht.h"
#include "dht-helper.h"

static int myStorageId, childRank, myRank, numProcesses;
static int parentRank, parentId, size;

int *keyVal;

// merge two key val databases.
void merge(int source) {
  int *parentKV, parentSize, rank;

  // First call for the size
  MPI_Recv(&parentSize, 1, MPI_INT, source, MERGE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  parentKV = (int *) malloc(parentSize * sizeof(int));
  // Second call for the array.
  MPI_Recv(parentKV, parentSize, MPI_INT, source, MERGE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);   

  keyVal = mergeArrays(keyVal, size, parentKV, parentSize); 
  size = size + parentSize; // Point here for debugging remove distribution

  rank = childRank;
  if(myRank == 0) rank = parentRank; 
  MPI_Send(&rank, 1, MPI_INT, rank, ACK, MPI_COMM_WORLD);
}

// Removes storage node from cycle
void remov(int source) {
  int id, rank;
  MPI_Recv(&id, 1, MPI_INT, source, REMOVE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  
  int argsOut[2];
  if(myStorageId == id) {
    // Update child status with parent (Change parents pointer to child)
    argsOut[0] = childRank;
    argsOut[1] = CHILD;
    MPI_Send(argsOut, 2, MPI_INT, parentRank, UPDATE, MPI_COMM_WORLD);

    // Update parent status with child
    argsOut[0] = parentRank;
    argsOut[1] = PRANK;
    MPI_Send(argsOut, 2, MPI_INT, childRank, UPDATE, MPI_COMM_WORLD);
   

    // Update parent ID with child
    argsOut[0] = parentId;
    argsOut[1] = PID;
    MPI_Send(argsOut, 2, MPI_INT, childRank, UPDATE, MPI_COMM_WORLD);
    
    // Redestribute here
    MPI_Send(&size, 1, MPI_INT, childRank, MERGE, MPI_COMM_WORLD);
    MPI_Send(keyVal, size, MPI_INT, childRank, MERGE, MPI_COMM_WORLD);
    free(keyVal); 
    
  } else {
    MPI_Send(&id, 1, MPI_INT, childRank, REMOVE, MPI_COMM_WORLD);
  }
}


// Updates pointers of node relationships. Child, parent, etc.
void update(int source, int count) {
  int args[count];
  int num, cmd; 
  MPI_Recv(&args, count, MPI_INT, source, UPDATE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  num = args[0];
  cmd = args[1];

  if(cmd == PRANK && count == 2) {
    parentRank = num;
  } else if(cmd == CHILD && count == 2) {
    childRank = num;
  } else if(cmd == PID && count == 2) {
    parentId = num;
  } else  {
    
  }
}


// Sends message to idle node for activation.
void activate(int source) {
  int *temp, tempSize;
  int argsAdd[3];
  int argOut[2];
  MPI_Recv(argsAdd, 3, MPI_INT, source, ACT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  parentRank = argsAdd[PRANK];
  parentId = argsAdd[PID];
  myStorageId = argsAdd[ID];
  childRank = source;
  size = myStorageId - parentId;
  keyVal = (int *) malloc(size * sizeof(int)); 
  temp = (int *) malloc(tempSize * sizeof(int));
  MPI_Recv(&tempSize, 1, MPI_INT, source, ACT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(temp, tempSize, MPI_INT, source, ACT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  memcpy(keyVal, temp, size * sizeof(int));
  argOut[0] = myRank; // Updating parents child
  argOut[1] = CHILD;           
  free(temp); 
  MPI_Send(argOut, 2, MPI_INT, parentRank, UPDATE, MPI_COMM_WORLD);
}

// Checks if current node is greater than requested id. Sends activation
// message to node if it is.
void add(int source) {
  int recv[2];
  int rank, id, cut, *temp;
  int *argsAdd = (int *) malloc (3 * sizeof(int));
  MPI_Recv(recv, 2, MPI_INT, source, ADD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  id = recv[1];
  if(myStorageId > id) { // Gather data for new node activation.
    rank = recv[0];
    argsAdd[PID] = parentId;
    argsAdd[PRANK] = parentRank; 
    argsAdd[ID] = id;
    cut = id - parentId;
    parentRank = rank;
    parentId = id;
    MPI_Send(argsAdd, 3, MPI_INT, rank, ACT, MPI_COMM_WORLD);
    MPI_Send(&size, 1, MPI_INT, rank, ACT, MPI_COMM_WORLD);
    MPI_Send(keyVal, size, MPI_INT, rank, ACT, MPI_COMM_WORLD);
    size = myStorageId - parentId;
    temp = (int*) malloc(size * sizeof(int));
    memcpy(temp, keyVal + cut, size * sizeof(int));  
    free(keyVal);
    keyVal = temp;
    MPI_Send(&size, 1, MPI_INT, childRank, ACK, MPI_COMM_WORLD); 
  } else {
    MPI_Send(recv, 2, MPI_INT, childRank, ADD, MPI_COMM_WORLD);
  }

}

// Sends data from one node to another uses count to make data retval flexable
void gain(int source, int count) {                                       
  int retval[count];                                                               
  int rank;
  MPI_Recv(&retval, count, MPI_INT, source, RETVAL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  rank = childRank;
  if(myRank == HEAD) rank = parentRank;
  MPI_Send(&retval, count, MPI_INT, rank, RETVAL, MPI_COMM_WORLD);                     
}

// on an END message, the head node is to contact all storage nodes and tell them
void headEnd() {
  int i, dummy;

  // the head node knows there is an END message waiting to be received because
  // we just called MPI_Probe to peek at the message.  Now we just receive it.
  MPI_Recv(&dummy, 1, MPI_INT, numProcesses-1, END, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  // tell all the storage nodes to END
  // the data sent is unimportant here, so just send a dummy value
  for (i = 1; i < numProcesses-1; i++) {
    MPI_Send(&dummy, 1, MPI_INT, i, END, MPI_COMM_WORLD);
  }

  MPI_Finalize();
  exit(0);
}

// on an END message, a storage node just calls MPI_Finalize and exits
void storageEnd() {
  int dummy;  // the data is unimportant for an END
  MPI_Recv(&dummy, 1, MPI_INT, 0, END, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
 
  MPI_Finalize();
  exit(0);
}

void getKeyVal(int source) {
  int *argsAdd;
  int key, value, rank, retval;

  // receive the GET message
  // note that at this point, we've only called MPI_Probe, which only peeks at the message
  // we are receiving the key from whoever sent us the message 
  MPI_Recv(&key, 1, MPI_INT, source, GET, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  if (key <= myStorageId) {  // I own this key/value pair
    // allocate two integers: the first will be the value, the second will be this storage id
    argsAdd = (int *) malloc (2 * sizeof(int));
    
    retval = key - (parentId + 1);

    // find the associated value (called "value") using whatever data structure you use
    // you must add this code to find it (omitted here)
    value = keyVal[retval];

    argsAdd[0] = value;
    argsAdd[1] = myStorageId;
    // send this value around the ring
    rank = childRank;
    if(myRank == HEAD) rank = parentRank;
    MPI_Send(argsAdd, 2, MPI_INT, rank, RETVAL, MPI_COMM_WORLD);
    free(argsAdd);
  }
  else {  // I do NOT own this key/value pair; just forward request to next hop
    MPI_Send(&key, 1, MPI_INT, childRank, GET, MPI_COMM_WORLD);
  }
}

// Adds key val pair to a node database.
void put(int source) {

  int *argsAdd;
  int key, val, rank, dummy, place;
  int kvPair[2];
  
  // receive the PUT message
  MPI_Recv(kvPair, 2, MPI_INT, source, PUT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  key = kvPair[0];
  val = kvPair[1];
  if (key <= myStorageId) {  // I own this key/value pair
    place = key - (parentId + 1); // Offset position on mem
    keyVal[place] = val;
    rank = childRank;
    if(myRank == HEAD) rank = parentRank;
    MPI_Send(&dummy, 1, MPI_INT, rank, ACK, MPI_COMM_WORLD);
  }
  else {  // I do NOT own this key/value pair; just forward request to next hop
    MPI_Send(&kvPair, 2, MPI_INT, childRank, PUT, MPI_COMM_WORLD);
  }
 

}

// Recieve and send acknowledgement
void ack(int source) {
  int dummy, rank;
  rank = childRank;
  if (myRank == HEAD) rank = parentRank;
  MPI_Recv(&dummy, 1, MPI_INT, source, ACK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Send(&dummy, 1, MPI_INT, rank, ACK, MPI_COMM_WORLD);

}

// handleMessages repeatedly gets messages and performs the appropriate action
void handleMessages() {
  MPI_Status status;
  int count, source, tag, dummy;

  while (1) {
    // peek at the message
    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_INT, &count);
    source = status.MPI_SOURCE;
    tag = status.MPI_TAG;
    // now take the appropriate action
    // code for END and most of GET is given; others require your code
    switch(tag) {
      case END:
        if (myRank == 0) {
          headEnd();
        }
        else {
          storageEnd();
        }
        break;
      case ADD:
        add(source);
        break;
      case REMOVE:
        remov(source);
        break;
      case PUT:
        put(source);
        break;
      case GET:
        getKeyVal(source);
        break;
      case ACK:
        ack(source);
        break;
      case ACT:
        activate(source);
        break;
      case UPDATE:
        update(source, count);
        break;
      case RETVAL:
        gain(source, count);
        break;
      case MERGE:
        merge(source);
        break;
      // NOTE: you probably will want to add more cases here, e.g., to handle data redistribution
      default:
        // should never be reached---if it is, something is wrong with your code; just bail out
        printf("ERROR, my id is %d, source is %d, tag is %d, count is %d\n", myRank, source, tag, count);
        exit(1);
    }  
  }
}

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  // get my rank and the total number of processes
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);
  
  // set up the head node and the last storage node
  if (myRank == 0) {
    myStorageId = 0;
    childRank = numProcesses-2;
    parentRank = numProcesses-1;
  }
  else if (myRank == numProcesses-2) {
    myStorageId = MAX;
    childRank = parentRank = parentId = 0; 
    keyVal = (int *) malloc(MAX * sizeof(int)); // allocate storage
    size = MAX;
  }

  // the command node is handled separately
  if (myRank < numProcesses-1) {
    handleMessages();
  }
  else {
    commandNode(); 
  }
  return 0;
}
