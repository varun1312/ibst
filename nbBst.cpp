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

	markStatus_t markNode(Node *node, int data) {
		Node *rP = ((Node *)((uintptr_t)node & ~0x03))->child[RIGHT];
		Node *lP = ((Node *)((uintptr_t)node & ~0x03))->child[LEFT];
		int *dP = ((Node *)((uintptr_t)node & ~0x03))->dataPtr;
		if (data != *(int *)((uintptr_t)dP & ~0x03)) {
			// Data pointer changed. This means the node is removed from the tree.
			return ABORT_REMOVE;
		}
		if (STATUS(rP) == PROMOTE)
			return REMOVE_PROMOTE;
		while(STATUS(rP) == UNQNULL) {
			if (data != *(int *)((uintptr_t)dP & ~0x03)) {
				// Data pointer changed. This means the node is removed from the tree.
				return ABORT_REMOVE;
			}
			if ((GETADDR(rP) == root) && (STATUS(dP) == MARKED))
				return HELP_REPLACE;
			if (CAS(&node->child[RIGHT], rP, UNQNULL, NULL, MARKED)) {
				lP = ((Node *)((uintptr_t)node & ~0x03))->child[LEFT];
				while(STATUS(lP) != MARKED) {
					lP = ((Node *)((uintptr_t)node & ~0x03))->child[LEFT];
					if (STATUS(lP) == UNQNULL) {
						if (CAS(&node->child[LEFT], lP, UNQNULL, NULL, MARKED))
							break;
					}
					else if (STATUS(lP) == NORMAL){
						if (CAS(&node->child[LEFT], lP, NORMAL, lP, MARKED))
							break;
					}
				}	
			}
			rP = ((Node *)((uintptr_t)node & ~0x03))->child[RIGHT];
			dP = ((Node *)((uintptr_t)node & ~0x03))->dataPtr;
		}
		if (STATUS(rP) == PROMOTE)
			return REMOVE_PROMOTE;
		while(STATUS(lP) == UNQNULL) {
			if (CAS(&node->child[LEFT], lP, UNQNULL, NULL, MARKED)) {
				rP = ((Node *)((uintptr_t)node & ~0x03))->child[RIGHT];
				while(STATUS(rP) != MARKED)	 {
					rP = ((Node *)((uintptr_t)node & ~0x03))->child[RIGHT];
					if (STATUS(rP) == UNQNULL) {
						if (CAS(&node->child[RIGHT], rP, UNQNULL, NULL, MARKED))
							break;
					}
					else if (STATUS(rP) == NORMAL) {
						if (CAS(&node->child[RIGHT], rP, NORMAL, rP, MARKED))
							break;
					}
					else if (STATUS(rP) == PROMOTE) {
						return REMOVE_PROMOTE; 
					}						
				}
			}
			lP = ((Node *)((uintptr_t)node & ~0x03))->child[LEFT];
		}
		if ((STATUS(rP) == MARKED) && (STATUS(lP) == MARKED)) {
			if ((GETADDR(rP) == NULL) && (GETADDR(lP) == NULL))
				return REMOVE_0C;
			else if ((GETADDR(rP) == NULL) || (GETADDR(lP) == NULL)) 
				return REMOVE_1C;
		}
		if ((STATUS(rP) == NORMAL) && (STATUS(lP) == NORMAL)) {
			while ((STATUS(dP) != MARKED) && (data == *(int *)((uintptr_t)dP & ~0x03))) {
				if (CAS(&node->dataPtr, dP, NORMAL, dP, MARKED))
					return REMOVE_2C;
				dP = ((Node *)((uintptr_t)node & ~0x03))->dataPtr;
			}
		}
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
				curr = (GETADDR(curr))->child[RIGHT]; 
			}
			else if (data < currData) {
				ancNode = GETADDR(pred);
				ancNodeDataSeen = GETDATA(ancNode);
				pred = GETADDR(curr);
				curr = (GETADDR(curr))->child[LEFT]; 
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
	const int numThreads = 100;
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
