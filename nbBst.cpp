#include <iostream>
#include <climits>
#include <vector>
#include <thread>

#define GETDATA(node) *(int *)(((uintptr_t)(((Node *)((uintptr_t)node & ~0x03))->dataPtr)) & ~0x03)
#define GETADDR(node)  (Node *)((uintptr_t)node & ~0x03)
#define ISNULL(node) ((status_t)((uintptr_t)node & 0x03) == UNQNULL)
#define MARKNODE(node, status) (((uintptr_t) node & ~0x03) | status)
#define CAS(ptr, source, sourceState, target, targetState) \
		__sync_bool_compare_and_swap(\
			ptr, \
			MARKNODE(source, sourceState), \
			MARKNODE(target, targetState))
#define STATUS(ptr) (status_t)((uintptr_t)ptr & 0x03)


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

/*	void markLeft(Node *node) {
		while(true) {
			Node *leftPtr = ((Node *)((uintptr_t)node & ~0x03))->child[LEFT];
			Node *n = ((Node *)((uintptr_t)node & ~0x03));
			if (CAS(&n->child[LEFT], leftPtr, NORMAL, leftPtr, MARKED)) {
				return;
			}
			if (STATUS(ptr) == MARKED)
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
			if ((status == NORMAL) || (status == UNQNULL))
				if (CAS(&n->child[RIGHT], rightPtr, status, rightPtr, MARKED))
					return RIGHTMARKED;
			if (status == MARKED)
				return RIGHTMARKED;	
		}
	}
	void markNode(Node *node, int data) {
		int *dP = ((Node *)((uintptr_t)node & ~0x03))->dataPtr;
		Node *rP = ((Node *)((uintptr_t)node & ~0x03))->child[RIGHT];
		Node *lP = ((Node *)((uintptr_t)node & ~0x03))->child[LEFT];
		if (ISNULL(rP)) {
			if (CAS(&node->child[RIGHT], NULL, NORMAL, NULL, MARKED)) {
				// Mark left for Sure
				markLeft(node);
			}
			else {
				/* CAS Failed. This can be because of the following
				reasons:
				1. rP changed from NULL to NON NULL.
				2. Status changed from NORMAL to MARKED
				3. Status changed from NORMAL to PROMOTE
				4. Status changed from NORMAL to UNQNULL 
				while(true) {
					Node *rP = ((Node *)((uintptr_t)node & ~0x03))->child[RIGHT];
					if (!(ISNULL(rP) || (ISUNQNULL(rP)))) {	
						break;
					}
					if (STATUS(rP) == PROMOTE)
						return CALL_REMOVE_PROMOTE;
					else if (STATUS(rP) == MARKED) {
							// Mark left for Sure
						markLeft(node);
					}
					else if (STATUS(rP) == UNQNULL) {
						int *dP = ((Node *)((uintptr_t)node & ~0x03))->dataPtr;
						if ((status_t)((uintptr_t)dP & 0x03) == MAKRED)
							return HELPREMOVE;
						return ABORTREMOVE;
					}
				}
			}
		}
		else if (ISUNQNULL(rP)) {
			int *dP = ((Node *)((uintptr_t)node & ~0x03))->dataPtr;
			if ((status_t)((uintptr_t)dP & 0x03) == MAKRED)
				return HELPREMOVE;
			return ABORTREMOVE;
		}
		if (ISNULL(lP)) {
			if (CAS(&node->child[LEFT], NULL, NORMAL, NULL, MARKED)) {
				// Mark RIGHT For Sure if it is NORMAL (or) UNQNULL
				// If it is PROMOTE, return CALL_REMOVE_PROMOTE
				markRightStatus_t rightStatus = markRight(node);
			}
			else {
				 CAS failed. CAS can fail because of the following
				reasons:
				1. lP changed from NULL to a regular pointer
				2. Status of LP changed from NORMAL to MARKED 
				while(true) {
					Node *lP = ((Node *)((uintptr_t)node & ~0x03))->child[LEFT];
					if (!ISNULL(lP) && !ISUNQNULL(lP))
						break;
					if (STATUS(lP) == MARKED) {
						// Mark RIGHT For Sure if it is NORMAL (or) UNQNULL
						// If it is PROMOTE, return CALL_REMOVE_PROMOTE
						markRightStatus_t rightStatus = markRight(node);
					}
				}
			}
		}
	}
*/	
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
				// Here we mark the node;
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
			if (CAS(&pred->child[RIGHT], curr, status, node, NORMAL))
				return true;
			else
				// Here we are retrying from start. But, we shoult backtrack
				return insert_tree(root, data);
		}
		else {
			if (CAS(&pred->child[LEFT], curr, status, node, NORMAL))
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
				curr = pred->child[RIGHT]; 
			}
			else if (data < currData) {
				ancNode = pred;
			    ancNodeDataSeen = GETDATA(ancNode);
				pred = GETADDR(curr);
				curr = pred->child[LEFT];
			}
			else if (data == currData) {
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
		std::cout<<GETDATA(node)<<std::endl;
		print_tree(((Node *)((uintptr_t)node & ~0x03))->child[RIGHT]);
	}

	void print() {
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
	const int numThreads = 100;
	std::vector<std::thread> addT(numThreads);
	for (int i = 0; i < numThreads; i++)
		addT[i] = std::thread(&nbBst::insert, &myTree, rand());
	for (int i = 0; i < numThreads; i++)
		addT[i].join();
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
