#include <iostream>
#include <climits>
#include <vector>
#include <thread>

#define MARKNODE(pointer, status) (((uintptr_t)pointer & ~0x03) | status)
#define ISNULL(ptr) (((uintptr_t)ptr & 0x03) == NULLPTR)
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
	REMOVE_1C,
	REMOVE_0C
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
	
	bool remove_tree(Node *node, int data) {
		Node *pred = node;
		Node *curr = node;
		Node *ancNode = node;
		int currData;
		while(true) {
			if (ISNULL(curr))
				return false;
			int *dP = (((Node *)((uintptr_t)curr & ~0x03))->dataPtr);
			Node *rP = (((Node *)((uintptr_t)curr & ~0x03))->child[RIGHT]);
			Node *lP = (((Node *)((uintptr_t)curr & ~0x03))->child[LEFT]);
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
			if (ISNULL(curr)) {
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

	void print_tree(Node *node) {
		if (ISNULL(node))
			return;
		else
			print_tree(node->child[LEFT]);
			if (!ISMARKED(node))
				std::cout<<GETDATA(node)<<std::endl;
			print_tree(node->child[RIGHT]);
	}
	
	void print() {
		return print_tree(root->child[LEFT]);
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
}

int main() {
	//testbenchSequential();
	testbenchParallel();
	return 0;
}
