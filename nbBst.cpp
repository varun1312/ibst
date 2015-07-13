#include <iostream>
#include <climits>
#include <vector>
#include <thread>

#define GETADDR(node)  ((Node *)((uintptr_t)node & (~0x03)))
#define GETDATA(node) *(int *)((uintptr_t)(GETADDR(node)->dataPtr) & (~0x03))
#define ISNULL(node) (((status_t)((uintptr_t)node & 0x03) == UNQNULL) || (GETADDR(node) == NULL))
#define ISUNQNULL(node) ((status_t)((uintptr_t)node & 0x03) == UNQNULL) 
#define MARKNODE(node, status) (Node *)(((uintptr_t)GETADDR(node)) | status)
#define CAS(ptr, source, sourceState, target, targetState) \
		__sync_bool_compare_and_swap(\
			ptr, \
			MARKNODE(source, sourceState), \
			MARKNODE(target, targetState))
#define STATUS(ptr) (status_t)((uintptr_t)ptr & 0x03)
#define ISMARKED(node) ((STATUS((GETADDR(node))->dataPtr) == MARKED) || (STATUS((GETADDR(node))->child[LEFT])== MARKED) || (STATUS((GETADDR(node))->child[RIGHT]) == MARKED))

enum status_t {
	NORMAL=0x00,
	MARKED=0x01,
	PROMOTE=0x02,
	UNQNULL=0x03
};

enum {
	LEFT=0,
	RIGHT=1
};

enum markRightStatus_t {
	CALL_REMOVE_PROMOTE,
	RIGHTMARKED
};

enum markStatus_t {
	HELP_REMOVE_SUCC_NODE,
	HELP_REPLACE,
	ABORT_REMOVE,
	REMOVE_PROMOTE,
	REMOVE_0C,
	REMOVE_1C,
	REMOVE_2C
};

class nbBst {
private:
	class Node {
	public:
		int *dataPtr;
		Node *child[2];
		Node(int data) {
			this->dataPtr = new int(data);
			child[LEFT] = child[RIGHT] = MARKNODE(NULL, UNQNULL);
			if ((child[0] == (Node *)0x103) || (child[RIGHT] == (Node *)0x103))	 {
				std::cout<<"Some Problem in assigning addresses..."<<std::endl;
				exit(0);
			}
		}
	};

	Node *root;
public:	
	nbBst() {
		root = new Node(INT_MAX);
		std::cout<<"Initialized root of the tree"<<std::endl;
	}

	markStatus_t mark(Node *node, int data) {
		Node *rp = node->child[RIGHT];
		while (ISUNQNULL(rp)) {
			if (GETADDR(rp) == root) {
				/* Check data. If data is Marked, HELP REMOVE.
				If data is not marked and not my data, return.
				If data is not marked, and my data, remove the node */
				// Break for now;
				break;
			}
			switch(STATUS(rp)) {
				case UNQNULL:
					if (CAS(&node->child[RIGHT], rp, UNQNULL, NULL, MARKED)) {
						/* CAS succeeded means mark left irrespective of its status */
						Node *lp = node->child[LEFT];
						while(STATUS(lp) != MARKED) {	
							if (STATUS(lp) == MARKED) {
								break;
							}
							else if (STATUS(lp) == UNQNULL) {
								if (CAS(&node->child[LEFT], lp, UNQNULL, NULL, MARKED)) {
									break;
								}
							}
							else if (STATUS(lp) == NORMAL) {
								if (CAS(&node->child[LEFT], lp, NORMAL, lp, MARKED)) {
									break;
								}
							}
							lp = node->child[LEFT];
						} 
					}
					break;
				case PROMOTE:
					/* Help by swapping data */
					break;
				case MARKED:
					/* Mark left and remove the node */
					Node *lp = node->child[LEFT];
					while(STATUS(lp) != MARKED) {	
						if (STATUS(lp) == MARKED) {
							break;
						}
						else if (STATUS(lp) == UNQNULL) {
							if (CAS(&node->child[LEFT], lp, UNQNULL, NULL, MARKED)) {
								break;
							}
						}
						else if (STATUS(lp) == NORMAL) {
							if (CAS(&node->child[LEFT], lp, NORMAL, lp, MARKED)) {
								break;
							}
						}
						lp = node->child[LEFT];
					}
				}
			rp = node->child[RIGHT];			
		}
		Node *lp = node->child[LEFT];
		while(ISUNQNULL(lp)) {	
			switch(STATUS(lp)) {
				case UNQNULL:
					if (CAS(&node->child[LEFT], lp, UNQNULL, NULL, MARKED)) {
						// Check and Mark right. The code is a bit tricky here.
						do {
							rp = node->child[RIGHT];
							if (STATUS(rp) == UNQNULL) {
								if (CAS(&node->child[RIGHT], rp, UNQNULL, NULL, MARKED)) {
									break;
								}
							}
							else if (STATUS(rp) == NORMAL) {
								if (CAS(&node->child[RIGHT], rp, NORMAL, rp, MARKED)) {
									break;
								}
							}
							else if (STATUS(rp) == MARKED) {
								break;
							}
							else if (STATUS(rp) == PROMOTE) {
								// Left Marked and right is PROMOTE. Spl Case. Handle this efficiently
							}
						}while(STATUS(rp) != MARKED);	
					}
					break;	
				case MARKED:
						// Check and Mark right. The code is a bit tricky here.
						do {
							rp = node->child[RIGHT];
							if (STATUS(rp) == UNQNULL) {
								if (CAS(&node->child[RIGHT], rp, UNQNULL, NULL, MARKED)) {
									break;
								}
							}
							else if (STATUS(rp) == NORMAL) {
								if (CAS(&node->child[RIGHT], rp, NORMAL, rp, MARKED)) {
									break;
								}
							}
							else if (STATUS(rp) == MARKED) {
								break;
							}
							else if (STATUS(rp) == PROMOTE) {
								// Left Marked and right is PROMOTE. Spl Case. Handle this efficiently
							}
						}while(STATUS(rp) != MARKED);
					break;
			}
			lp = node->child[LEFT];
		}
		// Here we need to return a proper value
		if ((GETADDR(rp) == NULL) && (GETADDR(lp) == NULL))
			return REMOVE_0C;
		else if (((GETADDR(rp) == NULL) && (GETADDR(lp) == (Node *)NORMAL)) || ((GETADDR(rp) == (Node *)NORMAL) && (GETADDR(lp) == NULL)))
			return REMOVE_1C;
		return REMOVE_2C;
	}

	bool remove_tree(Node *startNode, int data) {
		Node *pred = startNode;
		Node *curr = startNode;
		while(true) {
			if (ISNULL(curr))
				return false;
			int currData = GETDATA(curr);
			if (data > currData) {
				pred = GETADDR(curr);
				curr = GETADDR(curr)->child[RIGHT];
			}
			else if (data < currData) {
				pred = GETADDR(curr);
				curr = GETADDR(curr)->child[LEFT];
			}
			else if (data == currData) {
				// We must first mark the node.
				// Call the mark method.
				markStatus_t stat = mark(curr, data);
				return true;
			}
		}
	}

	bool remove(int data) {
		return remove_tree(root, data);
	}

	bool insert_data(Node *pred, Node *curr, status_t status, int data) {
		int predData = GETDATA(pred);	
		Node *node = new Node(data);
		if (data > predData) {
			if (CAS(&pred->child[RIGHT], curr, UNQNULL, node, NORMAL))
				return true;
			else
				// Here we are retrying from start. But, we shoult backtrack
				return insert_tree(root, data);
		}
		else {
			if (CAS(&pred->child[LEFT], curr, UNQNULL, node, NORMAL))
				return true;
			else
				// Here we are retrying from start. But, we shoult backtrack
				return insert_tree(root, data);
		}
	}

	bool insert_tree(Node *startNode, int data) {
		Node *pred = startNode;
		Node *curr = startNode;
		Node *ancNode = startNode;
		int ancNodeDataSeen, ancNodeDataCurr;
		while(true) {
			if (ISNULL(curr)) {
				int ancNodeDataCurr = GETDATA(ancNode);
				if ((ancNodeDataSeen != ancNodeDataCurr) && (data > ancNodeDataCurr))
					return insert_tree(ancNode, data);
				return insert_data(pred, curr, UNQNULL, data);	
			}
			int currData = GETDATA(curr);
			if (data > currData) {
				pred = GETADDR(curr);
				curr = GETADDR(curr)->child[RIGHT]; 
			}
			else if (data < currData) {
				ancNode = pred;
			    ancNodeDataSeen = GETDATA(ancNode);
				pred = GETADDR(curr);
				curr = GETADDR(curr)->child[LEFT];
			}
			else if (data == currData) {
				// Check is MAarked or not. Else, return
				if (ISMARKED(curr))
					return insert_tree(root, data);
				return false;
			}
		}
	}
	
	bool insert(int data) {
		return insert_tree(root, data);
	}
	
	void print_tree(Node *node) {
		if (ISNULL(node))
			return;
		print_tree(((Node *)((uintptr_t)node & ~0x03))->child[LEFT]);
	//	if (!ISMARKED(node))	
			std::cout<<GETDATA(node)<<std::endl;
		print_tree(((Node *)((uintptr_t)node & ~0x03))->child[RIGHT]);
	}

	void print() {
		std::cout<<"Calliing print tree method"<<std::endl;
		print_tree(root->child[LEFT]);
	}
};

void testbenchSequential() {
	nbBst myTree;
	srand(time(NULL));
	for (int i = 0; i < 10; i++)
		myTree.insert(rand());
	myTree.print();
}

void testbenchParallel() {
	std::cout<<"In Parallel Testbench"<<std::endl;
	nbBst myTree;
	srand(time(NULL));
	const int numThreads = 10;
	int arr[numThreads];
	int removeElement;
	std::vector<std::thread> addT(numThreads);
	std::vector<std::thread> removeT(numThreads);
	for (int i = 0; i < numThreads; i++)
		arr[i] = rand();
	for (int i = 0; i < numThreads; i++)
		addT[i] = std::thread(&nbBst::insert, &myTree, arr[i]);
	for (int i = 0; i < numThreads; i++)
		addT[i].join();
	myTree.print();
/*	for (int i = 0; i < numThreads; i++)
		myTree.remove(arr[i]); */
	for (int i = 0; i < numThreads; i++)
		removeT[i] = std::thread(&nbBst::remove , &myTree, arr[(i+5)%numThreads]);
	for (int i = 0; i < numThreads; i++)
		removeT[i].join();
	myTree.print();
/*	while(removeElement != -1) {
		std::cout<<"Enter an element to Remove : ";
		std::cin>>removeElement;
		myTree.remove(removeElement);
		myTree.print();
	}
*/
} 

int main() {
//	testbenchSequential();
	testbenchParallel();
	return 0;
}
