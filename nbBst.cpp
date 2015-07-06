#include <iostream>
#include <climits>
#include <vector>
#include <thread>

#define MARKNODE(pointer, status) (((uintptr_t)pointer & ~0x03) | status)
#define ISNULL(pred, curr) \
	( (((uintptr_t)curr & 0x03) == NULLPTR) && \
		(GETDATA(pred) == (int)(uintptr_t)curr) )
#define GETDATA(ptr) *(int *)((uintptr_t)(((Node *)((uintptr_t)ptr & ~0x03))->dataPtr) & ~0x03)
#define GETADDR(ptr) (Node *)((uintptr_t)ptr & ~0x03)
#define CAS(ptr, source, sourceStatus, target, targetStatus) \
	__sync_bool_compare_and_swap(\
		ptr, \
		MARKNODE(source, sourceStatus), \
		MARKNODE(target, targetStatus))

#define ISMARKED(node) \
		((((uintptr_t)(((Node *)((uintptr_t)node & ~0x03))->dataPtr) & 0x03) == MARKED)  || \
		 (((uintptr_t)node->child[RIGHT] & 0x03) == MARKED) || \
		 (((uintptr_t)node->child[LEFT] & 0x03) == MARKED))

#define STATUS(ptr) \
	(status_t)((uintptr_t)ptr & 0x03)

enum status_t {
	NORMAL,
	MARKED,
	PROMOTE,
	NULLPTR
};

enum {
	LEFT,
	RIGHT
};

enum markStatus_t {
	CALLREPLACE,
	REMOVE_2C,
	REMOVE_1C,
	REMOVE_0C,
	REMOVE_PROMOTE,
	LRNULL,
	LRNORMAL,
	UNINITIALIZED,
	REMOVESUCC
	// Here the return status has to be updated.
};

class nbBst {
private:
	class Node {
	public:
		int *dataPtr;
		Node *child[2];
		Node(int data) {
			this->dataPtr = new int(data);
			child[LEFT] = child[RIGHT] = (Node *)((uintptr_t)data | NULLPTR);
		}
	};

	Node *root;
public:	
	nbBst() {
		root = new Node(INT_MAX);
	}

	markStatus_t markForSure(Node *node, int data, int lr) {
		while(true) {
			int *dP = node->dataPtr;
			Node *ptr = node->child[lr];	
			if ((STATUS(dP) == MARKED) && (*(int *)GETADDR(ptr) == INT_MAX))
				return CALLREPLACE;
			if (STATUS(ptr) == NORMAL) {
				if (CAS(&node->child[lr], ptr, NORMAL, ptr, MARKED)) 
					return LRNORMAL;
				continue;
			}
			if (STATUS(ptr) == MARKED) {
				if ((*(int*)GETADDR(ptr) == GETDATA(node)) || (*(int*)GETADDR(ptr) == INT_MAX))
				return LRNULL;
			}
			if (STATUS(ptr) == PROMOTE) {
				// here we must call helping replace node removal
				printf("Enterring Promote Mode...\n");
				return REMOVE_PROMOTE;
			}
			if (STATUS(ptr) == NULLPTR) {
				if (CAS(&node->child[lr], ptr, NULLPTR, ptr, MARKED))
 					return LRNULL;
				continue;
			}
		}
	}

	markStatus_t mark(Node *node, int data, int *dataPtr) {
		Node *rP = node->child[RIGHT];
		Node *lP = node->child[LEFT];
		int *dP = node->dataPtr;
		status_t rS = STATUS(rP);
		markStatus_t rN = UNINITIALIZED, lN = UNINITIALIZED;
		if ((STATUS(dP) == MARKED) && (*(int *)GETADDR(rP) == INT_MAX))
			return CALLREPLACE;
		if (rS == NULLPTR) {
			if (CAS(&node->child[RIGHT], rP, NULLPTR, rP, MARKED)) {
				lN = markForSure(node, data, LEFT);
				if (lN == LRNULL)
					return REMOVE_0C;
				return REMOVE_1C;
			}
			else {
				// Here we nned to take proper action	
				while(true) {
					Node *rP = node->child[RIGHT];
					status_t rS = STATUS(rP);
					if (rS == NORMAL)
						break;
					if (rS == MARKED) {
						if ((*(int*)GETADDR(rP) == GETDATA(node)) || (*(int*)GETADDR(rP) == INT_MAX)) {
							rN = LRNULL;
							lN = markForSure(node, data, LEFT);
						}
						break;
					}	
					if (rS == PROMOTE)
						return REMOVE_PROMOTE;
					if (rS == NULLPTR) {
						if (CAS(&node->child[RIGHT], rP, NULLPTR, rP, MARKED)) {
							lN = markForSure(node, data, LEFT);
							if (lN == LRNULL)
								return REMOVE_0C;
							return REMOVE_1C;
						}
					}
				}
			}
		}
		status_t lS = STATUS(lP);
		if (lS == NULLPTR) {
			if (CAS(&node->child[LEFT], lP, NULLPTR, lP, MARKED)) {
				rN = markForSure(node, data, RIGHT);
				if (rN == REMOVE_PROMOTE)
					return CALLREPLACE;
				if (rN == LRNULL)
					return REMOVE_0C;
				return REMOVE_1C;
			}
			else {
				while(true) {
					Node *lP = node->child[LEFT];
					status_t lS = STATUS(lP);
					if (lS == NORMAL)
						break;
					if (lS == MARKED) {
						if (*(int*)GETADDR(lP) == GETDATA(node)){
							lN = LRNULL;
							rN = markForSure(node, data, RIGHT);
							if (rN == REMOVE_PROMOTE)
								return CALLREPLACE;
							if (rN == LRNULL)
								return REMOVE_0C;
							return REMOVE_1C;
						}
					}	
					if (lS == NULLPTR) {
						if (CAS(&node->child[LEFT], lP, NULLPTR, lP, MARKED)) {
							rN = markForSure(node, data, LEFT);
							if (rN == REMOVE_PROMOTE)
								return CALLREPLACE;
							if (rN == LRNULL)
								return REMOVE_0C;
							// WHat if right returns promote remove...
							// Also add that code.
							return REMOVE_1C;
						}
					}
				}
			}
		}
		if ((rN == LRNULL) && (lN == LRNULL))
			return REMOVE_0C;
		else if ((rN == LRNULL) || (lN == LRNULL))
			return REMOVE_1C;
		// 2C Node
		if (CAS(&node->dataPtr, dataPtr, NORMAL, dataPtr, MARKED))
			return REMOVE_2C;
		// CAS failed means that the data pointer has been changed. Therefore, remove the successor node.
		return REMOVESUCC;
	}
	
	bool remove_tree(Node *node, int data) {
		Node *pred = node;
		Node *curr = node;
		Node *ancNode = node;
		int currData;
		while(true) {
			if (ISNULL(pred, curr))
				return false;
			int *dP = (((Node *)((uintptr_t)curr & ~0x03))->dataPtr);
			currData = *(int *)((uintptr_t)dP & ~0x03) ;
			if (data > currData) {
				pred = GETADDR(curr), curr = curr->child[RIGHT];
			}
			else if (data < currData) {
				ancNode = pred, pred = GETADDR(curr), curr = curr->child[LEFT];
			}
			else if (data == currData) {
				// Here we call MARK method and then physically remove the data
				// returning "true" for now;
				markStatus_t stat = mark(curr, data, dP);
				return true;
			}
		}
	}

	bool remove(int data) {
		return remove_tree(root, data);
	}

	bool insert_tree(Node *startNode, int data) {
		Node *pred = startNode;
		Node *curr = startNode;
		Node *ancNode = startNode;
		int currData;
		int ancNodeDataPrev, ancNodeDataCurr;
		while(true) {
			if (ISNULL(pred, curr)) {
				// Here we will insert data.
				ancNodeDataCurr = GETDATA(ancNode);
				if (ancNodeDataCurr != ancNodeDataPrev)
					// We need to back track instead of restart 
					return insert_tree(root, data);
				int predData = GETDATA(pred);
				Node *node = new Node(data);
				if (data > predData) {
					if (CAS(&pred->child[RIGHT], curr, NULLPTR, node, NORMAL))
						return true;
					else
						// We need to back track instead of restart 
						return insert_tree(root, data);
				}
				else {
					if (CAS(&pred->child[LEFT], curr, NULLPTR, node, NORMAL))
						return true;
					else
						// We need to back track instead of restart 
						return insert_tree(root, data);
				}
			}
			currData = GETDATA(curr);
			if (data > currData) {
				pred = GETADDR(curr), curr = curr->child[RIGHT]; 
			}
			else if (data < currData) {
				ancNode = pred, pred = GETADDR(curr), curr = curr->child[LEFT];
				ancNodeDataPrev = GETDATA(ancNode);
			}
			else if (data == currData) {
				// Here check the validity of data;	
				return false;
			}
		}
	}
	
	bool insert(int data) {
		return insert_tree(root, data);
	}

	void print_tree(Node *pred, Node *curr) {
		if (ISNULL(pred, curr))
			return;
		print_tree(curr, curr->child[LEFT]);
		if (!ISMARKED(curr))
			std::cout<<GETDATA(curr)<<std::endl;
		print_tree(curr, curr->child[RIGHT]);
	}
	
	void print() {
		return print_tree(root, root->child[LEFT]);
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
	std::vector<std::thread> addT(numThreads);
	for (int i = 0; i < numThreads; i++)
		addT[i] = std::thread(&nbBst::insert, &myTree, rand());
	for (int i = 0; i < numThreads; i++)
		addT[i].join();
	myTree.print();	
	int removeElement;
	std::cout<<"Enter an element to Remove : ";
	while(removeElement != -1) {
		std::cin>>removeElement;
		myTree.remove(removeElement);
		myTree.print();
	}
}

int main() {
	testbenchSequential();
//	testbenchParallel();
	return 0;
}
