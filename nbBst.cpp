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

	bool insert_tree(Node *startNode, int data) {
		Node *pred = startNode;
		Node *curr = startNode;
		Node *ancNode = startNode;
		int currData;
		int ancNodeDataPrev, ancNodeDataCurr;
		while(true) {
			if (ISNULL(curr)) {
				// Here we will insert data.
				int predData = GETDATA(pred);
				Node *node = new Node(data);
				if (data > predData) {
					if (CAS(&pred->child[RIGHT], curr, NULLPTR, node, NORMAL))
						return true;
					else 
						return insert_tree(root, data);
				}
				else {
					if (CAS(&pred->child[LEFT], curr, NULLPTR, node, NORMAL))
						return true;
					else
						return insert_tree(root, data);
				}
			}
			currData = GETDATA(curr);
			if (data > currData) {
				pred = GETADDR(curr), curr = curr->child[RIGHT]; 
			}
			else if (data < currData) {
				ancNode = pred, pred = GETADDR(curr), curr = curr->child[LEFT];
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

int main() {
	testbenchSequential();
	return 0;
}
