/*
 * AUTHOR: Angel Benavides
 * FILE: Tree.java
 * COURSE: Cs 345, Fall 2022
 * ASSIGNMENT: Project 5
 * PURPOSE: Implement a RLRB tree.
 */
import java.lang.Math.*;

public class Tree <K extends Comparable<K>, V> {
	
	private static final boolean BLACK = true;
	private static final boolean RED = false; 	
	private boolean inc;
	private Node root;
    	

	public Tree() {
		this.root = null;
		this.inc = true;
	}

	/* --- put --- 
	 * Insert Key, val into the tree or update val if key is already in the
	 * tree.
	 *
	 * @param key, key value used for comparison. 
	 * @param val, value associated with key. 
	 */
	public void put(K key, V val) {
		if(root == null) {
			root = new Node(key, val, 1, BLACK);
		} else {
			root = insert(key, val,root);
		}
		
		this.inc = true;
		
	}

	/* --- insert ---
	 * Helper method used to put key into tree. 
	 */
	private Node insert(K key, V val, Node n) {
		if (n == null) {
			return new Node(key, val, 1, RED);
		}
		if(n.key.compareTo(key) == 0) {
			n.val = val;
			this.inc = false;
			return n;
		} else if (n.key.compareTo(key)> 0) {
			n.left = insert(key, val, n.left);
		} else {
			n.right = insert(key, val, n.right);
		}
		if(inc) {
			n.N++; // Increment size for added node.
		}
		n = maintenance(n);

		return n;
	}

	/* ------ maintenance -----
	 * Checks the properties of the node and 
	 * its children. Performs movements of tree
	 * if properties are not met.
	 *
	 * @param n, Node that is getting checked
	 *
	 * @return n, Node possibily a different one. 
	 */
	private Node maintenance(Node n) { 
		// Case 1. n.left is red & n.right is black or null.
		if(isRed(n.left) && !isRed(n.right)) {
			// rotate right
			n = rotateRight(n);
		}

		// Case 2. n.left is red & n.right is red
		if (isRed(n.left) && isRed(n.right)) {
			flipColors(n);
		}	

		// Case 3. n.right is red and n.right.right is red
		if (isRed(n.right) && isRed(n.right.right)) {
			n = rotateLeft(n);
			flipColors(n);
		}	

		return n;
	}


	/* --- get ---
	 * Get the value associated with key in the tree or return null if the key
	 * does not exist. 
	 */
	public V get(K key) {
		Node n = get(key, root);
		if (n == null) {
			return null;
		}
		return n.val;
	}

	/* --- get ----
	 * Helper method used to find node with key and return value. 
	 */
	public Node get(K key, Node n) {
		if (n == null) {
			return null;
		}
		if (n.key.compareTo(key) == 0) {
			return n;
		} else if (n.key.compareTo(key) > 0) {
			return get(key, n.left); 
		} else {
			return get(key, n.right);
		}
	}

	/* --- isEmpty --- 
	 * Return true if the tree is empty or false otherwise.
	 */
	public boolean isEmpty() {
		return root == null;
	}

	/* --- size ---
	 * Return the number of nodes in the 
	 * tree. 
	 */
	public int size() {
		if (root == null) {
			return 0;
		}
		return size(root.key);
	}

	/* --- size --- 
	 * Returns the size of the subtree for
	 * the given Key.
	 */
	public int size (K key) {
		Node subtree = get(key, root);
		if (subtree != null) {
			return subtree.N;
		}
		return -1;
	}
	
	/* --- size ----
	 * Returns size of node.
	 */
	private int size(Node n) {
		if(n == null) {
			return 0;
		}
		return n.N;
	}

	/* --- height --- 
	 * Return the height of the tree.
	 */
	public int height() {
		if (root == null) {
			return -1;
		}
		return height(root.key); 
	}

	/* --- height --- 
	 * Return the height of the subtree whose root node contains the given key.
	 * If the key does not exist in the tree, return -1. Worst case should be
	 * LogN.
	 */
	public int height(K key) {
		Node subtree = get(key, root);
		if(subtree != null) {
			if(isRed(subtree)) {
				int h = (int) (2 * (Math.log(subtree.N-2) / Math.log(3)));
				return h;
			}
			return (int) (Math.log(subtree.N) / Math.log(2));
		}
		return -1;
	}

	/* --- contains ---
	 * Return true if the key is in the tree and false otherwise. Worst LogN
	 */ 
	public boolean contains(K key) {
		return get(key) != null;
	}

	
	/* ---- rotateLeft ----
	 * Rotates the node to the Left.
	 *
	 * @param n, the node to be rotated.
	 */
	private Node rotateLeft(Node n) {
		Node temp = n.right; 
		n.right = temp.left;
		temp.left = n;
		temp.color = n.color;
		n.color = RED;
		temp.N = n.N;
		n.N = 1 + size(n.left) + size(n.right);
		return temp;
	}

	/* ----- rotateRight -----
	 * Rotates the Node to the Right.
	 *
	 * @param n, the node to be rotated.
	 */ 
	private Node rotateRight(Node n) {
		Node temp = n.left;
		n.left = temp.right; 
		temp.right = n; 
		temp.color = n.color; 
		n.color = RED;
		temp.N = n.N;
		n.N = 1 + size(n.left) + size(n.right);
		return temp;
	}
	
	/* ------ flipColors -----
	 * Flips the colors of the given node and 
	 * its children.
	 *
	 * @param n, node
	 */
	private void flipColors(Node n) {
		n.color = RED;
		n.left.color = BLACK;
		n.right.color = BLACK;
	}

	/* checks if node is red */
	private boolean isRed(Node n) {
		
		if (n == null) {
			return false;
		}
		return n.color == RED;  
	}

	/* Node Class */
	private class Node{

		Node left, right;
		boolean color;
		int height; 
		int N; 
		K key; 
		V val; 
		
		/* ---- Constructor ----*/
		public Node(K key, V val, int N, boolean color) {
			this.color = color; 
			this.right = null; 
			this.left = null;
			this.height = 0;
			this.key = key;
			this.val = val;
			this.N = N;	
		}
	}
}
