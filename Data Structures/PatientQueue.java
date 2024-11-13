/* 
 * AUTHOR: Angel Benavides
 * FILE: PatientQueue.Java
 * ASSIGNMENT: PA9 - PatientQueue
 * COURSE: CSc 210; Spring 2022
 * PURPOSE: Create a data structure "PriorityQueue" that keeps 
 * track of patients in an array and sorts them in order of higher 
 * priority using Binary minimum heap data structure. Has 11 methods including 
 * Constructor. Uses Helper functions to compare and swap patients
 * within heap. Has methods for bubbling up and down through array. 
 */

public class PatientQueue {

    /*-----Fields---- 
     * Uses three fields to create 
     * ADT of PriorityQueue.
     * size to keep track of size. 
     * An array of patient class and 
     * a default size for the array. 
     */
    private static final int DEFUALT = 10;
    private Patient[] array;
    private int size;

    /*----Constructor----
     * Assigns variables to fields upon 
     * Instantiation. 
     */
    public PatientQueue() {
        array = new Patient[DEFUALT];
        size = 0;
    }

    /*-----enqueue----
     * Creates a an instance of a Patient class
     * then adds it to the PriorityQueue using 
     * the other "enqueue" method.
     * 
     * @param name, A string required to create a patient class.
     * @param priority, and int required to create a patient class.
     */
    public void enqueue(String name, int priority) {
        Patient newP = new Patient(name, priority);
        enqueue(newP);
    }

    /*------enqueue------
     * Adds a patient to the array that represents 
     * the PriorityQueue. Increments size then 
     * places the Patient at the index of size.
     * Will then bubble the patient up the array 
     * using the method 'bubbleUp'.
     * 
     * @param patient, The object of the patient class input 
     * to the array. 
     */
    public void enqueue(Patient patient) {
        size += 1;
        if (size == array.length) {
            growArray();
        }
        array[size] = patient;
        bubbleUp(size);
    }

    /*-----dequeue----  
     * Removes the first patient object from the array and 
     * returns the name. Adjusts heap to full fill the properties
     * required. Throws Exception for empty PriorityQueue.
     * 
     * @return patient.name, the name of the first patient in the queue. 
     */
    public String dequeue() {
        if (size <= 0) {
            throw new IllegalStateException(
                    "Cannot dequeue empty PriorityQueue");
        }

        Patient patient = array[1];
        array[1] = array[size];
        array[size] = null;
        size--;
        bubbleDown(1);
        return patient.name;

    }

    /*----peek----- 
     * Gets the name of the first patient in the 
     * priority queue. 
     * 
     * @return array[1].name, name of patient with highest priority. 
     */
    public String peek() {
        if (size <= 0) {
            throw new IllegalStateException(
                    "Cannot peek into empty PriorityQueue");
        }
        return array[1].name;
    }

    /*----peekPriority---- 
     * Gets the highest priority number in the 
     * priority queue. 
     * 
     * return array[1].priority, highest priority in queue. 
     */
    public int peekPriority() {
        if (size <= 0) {
            throw new IllegalStateException(
                    "Cannot peek into empty PriorityQueue");
        }
        return array[1].priority;
    }

    /*------changePriority------ 
     * Used to search array for patient with unique name 
     * and change the priority associated with patient. Bubbles
     * up or down depending on weather the new priority is 
     * less or greater than the previous. 
     * 
     * @param name, A string the for loop is searching for in 
     * each patient, 
     * @param newPriority, An integer to be switched if name 
     * of object matches the found one. c
     */
    public void changePriority(String name, int newPriority) {
        for (int i = 1; i <= size; i++) {
            if (array[i].name.equals(name)) {
                int temp = array[i].priority;
                array[i].priority = newPriority;
                if (temp > newPriority) {
                    bubbleUp(i);
                }
                if (temp < newPriority) {
                    bubbleDown(i);
                }
            }
        }
    }

    /*------isEmpty----
     * Checks if the Priority queue is empty 
     * by verifying size is less than 1.  
     * 
     * @return boolean, true if size is zero. 
     */
    public boolean isEmpty() {
        return size <= 0;
    }

    /*-----size-----
     * @return size, the size of the array. 
     */
    public int size() {
        return size;
    }

    /*-----clear-----
     * Resets the array size to zero.
     * Represents clearing the patient queue. 
     */
    public void clear() {
        size = 0;
    }

    /*
     * ----toString----
     * Will convert the array elements that
     * represent the PriorityQueue into a string representation.
     * Overrides object toString method.
     * 
     * @return string, string that has elements of stack
     * in curly braces.
     */
    public String toString() {
        String string = "{";
        if (isEmpty()) {
            return string += "}";
        }
        for (int i = 1; i < size; i++) {
            string += array[i].toString() + ", ";
        }
        string += array[size].toString() + "}";
        return string;
    }

    /*----comparePatients-----
     * Helper function used to compare one patient to another to see if 
     * the patient has higher priority. The method will return true if 
     * patient1 has a higher priority that patient2. Uses a helper 
     * function to compare names within the patient instances in the case 
     * that the priorities are the same. 
     * 
     * @param patient1, A patient instance that is used for comparison. This
     * patient is getting compared to another to see if it has a higher 
     * priority.
     * @param patient2, A patient instance that is getting compared by 
     * patient1. 
     * 
     * @return boolean, Will return true if patient1 has a higher priority 
     * than patient2.
     */
    private boolean compare(Patient patient1, Patient patient2) {
        int p1 = patient1.priority;
        int p2 = patient2.priority;

        if (p1 < p2) {
            return true;
        }
        if (p1 == p2) {
            if (compareNames(patient1.name, patient2.name)) {
                return true;
            }
        }
        return false;
    }

    /*------compareNames------
     * Compares one string to another to see if it comes 
     * before the other alphabetically. Will return true if 
     * string comes before alphabetically. Ex. 'Amy' will 
     * become before 'Ben. 
     * 
     * @param name1, A string used to check if it comes before 
     * another alphabetically. 
     * @paream name2, A string used to get compared too.
     * 
     * @return boolean, Returns true if name1 comes before name2.
     */
    private boolean compareNames(String name1, String name2) {

        for (int i = 0; i < name1.length(); i++) {
            if (i >= name2.length()) {
                return false;
            }
            int c1 = name1.charAt(i);
            int c2 = name2.charAt(i);
            if (c1 < c2) {
                return true;
            }
        }
        return true;
    }

    /*-------swap------
     * Used to swap to objects at the input indexes. 
     * 
     * @param index, index to switch with other
     * @param other, index to switch.  
     */
    private void swap(int index, int other) {
        Patient temp = array[other];
        array[other] = array[index];
        array[index] = temp;
    }

    /*-----bubbleUp----
     * Helper function used to implement 
     * the Bubble up concept in binary minimum heap. 
     * 
     * @param child, index of array that contains patient instance
     * that needs to be bubbled up. 
     */
    private void bubbleUp(int child) {
        int parent = child / 2;

        while (parent > 0) {
            if (compare(array[child], array[parent])) {
                swap(child, parent);
                child = parent;
                parent = child / 2;
            } else {
                parent = 0;
            }
        }
    }

    /*------bubbleDown-----
     * Helper function used to implement the 
     * Bubble down concept in binary minimum heap. 
     * 
     * @param parent, index that contains patient instance that 
     * needs to be bubbled down. 
     */
    private void bubbleDown(int parent) {
        int child1 = parent * 2;
        while (child1 <= size) { // Ends if there is no children.
            int child2 = child1 + 1;
            if (child2 > size) { // Used if there is no second child.
                if (compare(array[child1], array[parent])) {
                    swap(child1, parent);
                    parent = child1;
                } else {
                    break;
                }
            } else { // Used for two children
                if (compare(array[child1], array[child2])) {
                    if (compare(array[child1], array[parent])) {
                        swap(parent, child1);
                        parent = child1;
                    } else {
                        break;
                    }
                } else {
                    if (compare(array[child2], array[parent])) {
                        swap(child2, parent);
                        parent = child2;
                    } else {
                        break;
                    }
                }
            }
            child1 = parent * 2;
        }
    }

    /*----growArray----
     * Will create a new array double the size 
     * of the current in the class. Used when the
     * current array is has no more space for new 
     * elements. Created by Tyler Conklin. 
     */
    private void growArray() {
        Patient[] newArray = new Patient[2 * array.length];
        for (int i = 1; i < size; i++) {
            newArray[i] = array[i];
        }
        array = newArray;
    }

}

