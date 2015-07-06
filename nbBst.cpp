#include <iostream>
#include <climits>
#include <vector>
#include <thread>

#define GETADDR(node)  (Node *)((uintptr_t)node & ~0x03)
#define GETDATA(node) *(int *)((uintptr_t)((GETADDR(node))->dataPtr) & ~0x03)
#define ISNULL(node) (((status_t)((uintptr_t)node & 0x03) == UNQNULL) || (GETADDR(node) == NULL))
#define MARKNODE(node, status) (Node *)(((uintptr_t) node & ~0x03) | status)
#define CAS(ptr, source, sourceState, target, targetState) \
		__sync_bool_compare_and_swap(\
			ptr, \
			MARKNODE(source, sourceState), \
			MARKNODE(target, targetState))
#define STATUS(ptr) (status_t)((uintptr_t)ptr & 0x03)
#define ISMARKED(node) ((STATUS((GETADDR(node))->dataPtr) == MARKED) || (STATUS((GETADDR(node))->child[LEFT])== MARKED) || (STATUS((GETADDR(node))->child[RIGHT]) == MARKED))

enum status_t {
	NORMAL,
	MARKED,
	PROMOTE,
	UNQNULL
};

enum {
	LEFT,
	RIGHT
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
			child[LEFT] = child[RIGHT] = (Node *)UNQNULL;	
		}
	};

	Node *root;
public:	
	nbBst() {
		root = new Node(INT_MAX);
		std::cout<<"Initialized root of the tree"<<std::endl;
	}

	void markLeft(Node *node) {
		while(true) {
			Node *leftPtr = ((Node *)((uintptr_t)node & ~0x03))->child[LEFT];
			Node *n = ((Node *)((uintptr_t)node & ~0x03));
			status_t leftStatus = STATUS(leftPtr);
			if (leftStatus == UNQNULL) {
				if (CAS(&((GETADDR(node))->child[LEFT]), leftPtr, UNQNULL , NULL, MARKED)) {
					return;
				}
			}
			else if (leftStatus == NORMAL) {
				if (CAS(&((GETADDR(node))->child[LEFT]), leftPtr, NORMAL, leftPtr, MARKED)) {
					return;
				}
			}
			else if (leftStatus == MARKED)
				return;
		}
	}

	markRightStatus_t markRight(Node *node) {
		while(true) {
			Node *rightPtr = ((Node *)((uintptr_t)node & ~0x03))->child[RIGHT];
			Node *n = ((Node *)((uintptr_t)node & ~0x03));
			status_t status = STATUS(rightPtr);
			if (status == PROMOTE)
				return CALL_REMOVE_PROMOTE;
			if (status == NORMAL) 
				if (CAS(&n->child[RIGHT], rightPtr, NORMAL, rightPtr, MARKED))
					return RIGHTMARKED;
			if (status == UNQNULL)
				if (CAS(&n->child[RIGHT], rightPtr, UNQNULL, NULL, MARKED))
					return RIGHTMARKED;
			if (status == MARKED)
				return RIGHTMARKED;	
		}
	}
	
	markStatus_t markNode(Node *node, int data) {
		int *dP = ((Node *)((uintptr_t)node & ~0x03))->dataPtr;
		Node *rP = ((Node *)((uintptr_t)node & ~0x03))->child[RIGHT];
		Node *lP = ((Node *)((uintptr_t)node & ~0x03))->child[LEFT];
		if (data != *(int *)((uintptr_t)dP & ~0x03)) {
			// Data pointer changed. This means the node is removed from the tree.
			// Help remove Succ Node.
			return ABORT_REMOVE;
		}
		if (ISNULL(rP)) {
			if (GETADDR(rP) == root && STATUS(dP) == MARKED)
				return HELP_REPLACE;
			if (CAS(&((GETADDR(node))->child[RIGHT]), rP, UNQNULL, NULL, MARKED)) {
				// Mark left for Sure
				markLeft(node);
			}
			else {
				/* CAS Failed. This can be because of the following
				reasons:
					1. rP changed from NULL to NON NULL(i.e UNQNULL to NORMAL status change).
					2. Status changed from UNQNULL to MARKED
					3. Status changed from UNQNULL to PROMOTE */
				while(true) {
					Node *rP = ((Node *)((uintptr_t)node & ~0x03))->child[RIGHT];
					int *dP = ((Node *)((uintptr_t)node & ~0x03))->dataPtr;
					if (data != *(int *)((uintptr_t)dP & ~0x03)) {
						return ABORT_REMOVE;
					}
					if (GETADDR(rP) == root && STATUS(dP) == MARKED)
						return HELP_REPLACE;
					status_t rightStatus = STATUS(rP);
					if (rightStatus == NORMAL)
						break;
					else if (rightStatus == MARKED) {
						markLeft(node);
						break;
					}
					else if (rightStatus == PROMOTE) {
						// Here we will first help to CAS the data,
						// then we remove the data
						// and then call remove_tree(ancNode, data);
					}
				}
			}
		}
		if (ISNULL(lP)) {
			if (CAS(&(GETADDR(node))->child[LEFT], lP, UNQNULL, NULL, MARKED)) {
				// Here we mark right irrespective of its status.
				markRightStatus_t stat = markRight(node);
				if (stat == CALL_REMOVE_PROMOTE)
					return REMOVE_PROMOTE;
			}
			else {
				/* CAS failed. This can be because of the following
				reasons:
					1. lP changed from UNQNULL to NORMAL
					2. UNQNULL changed from UNQNULL to MARKED*/
				while(true) {
					Node *lP = ((Node *)((uintptr_t)node & ~0x03))->child[LEFT];
					int *dP = ((Node *)((uintptr_t)node & ~0x03))->dataPtr;
					if (data != *(int *)((uintptr_t)dP & ~0x03)) {
						return ABORT_REMOVE;
					}
					status_t leftStatus = STATUS(lP);
					if (leftStatus == NORMAL)
						break;
					if (leftStatus == MARKED) {
						// Here we mark right irrespective of its status.
						markRightStatus_t stat = markRight(node);
						if (stat == CALL_REMOVE_PROMOTE)
							return REMOVE_PROMOTE;
						break;
					}
				}
			}
		}
		if (((GETADDR((GETADDR(node))->child[RIGHT])) == NULL) && ((GETADDR((GETADDR(node))->child[LEFT])) == NULL))
			return REMOVE_0C;
		if (((GETADDR((GETADDR(node))->child[RIGHT])) == NULL) || ((GETADDR((GETADDR(node))->child[LEFT])) == NULL))
			return REMOVE_1C;
		if (&node->dataPtr, dP, NORMAL, dP, MARKED)
			return REMOVE_2C;
		if (STATUS(dP) == MARKED)
			return REMOVE_2C;
		return ABORT_REMOVE;
	}
	
	bool remove_tree(Node *startNode, int data)	 {
		Node *pred = startNode;
		Node *curr = startNode;
		Node *ancNode = startNode;
		int ancNodeDataSeen, ancNodeDataCurr;
		while(true) {
			if (ISNULL(curr)) {
				ancNodeDataCurr = GETDATA(ancNode);
				if ((ancNodeDataSeen != ancNodeDataCurr) && (data > ancNodeDataCurr))
					return remove_tree(ancNode, data);
				return false;
			}
			int currData = GETDATA(curr);
			if (data > currData) {
				pred = GETADDR(curr);
				curr = curr->child[RIGHT]; 
			}
			else if (data < currData) {
				ancNode = pred;
				ancNodeDataSeen = GETDATA(ancNode);
				pred = GETADDR(curr);
				curr = curr->child[LEFT]; 
			}
			else if (data == currData) {
				markStatus_t markStat = markNode(GETADDR(curr), data);
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
				curr = curr->child[RIGHT]; 
			}
			else if (data < currData) {
				ancNode = pred;
			    ancNodeDataSeen = GETDATA(ancNode);
				pred = GETADDR(curr);
				curr = curr->child[LEFT];
			}
			else if (data == currData) {
				// Check is MAarked or not. Else, return
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
		if (!ISMARKED(node))	
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
	for (int i = 0; i < numThreads; i++)
		removeT[i] = std::thread(&nbBst::remove , &myTree, arr[i]);
	for (int i = 0; i < numThreads; i++)
		removeT[i].join();
	myTree.print();
}
/*	int removeElement;
	std::cout<<"Enter an element to Remove : ";
	while(removeElement != -1) {
		std::cin>>removeElement;
		myTree.remove(removeElement);
		myTree.print();
	}
} */

int main() {
//	testbenchSequential();
	testbenchParallel();
	return 0;
}
