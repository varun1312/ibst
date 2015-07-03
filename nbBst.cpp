#include <iostream>
#include <climits>
#include <vector>
#include <thread>

#define ISNULL(ptr) (((uintptr_t)ptr & 0x03) == NULLPTR)
#define ISSPLPTR(ptr) (ptr == NULL)
#define GETDATA(ptr) *(int *)((uintptr_t)(((Node *)((uintptr_t)ptr & ~0x03))->dataPtr) & ~0x03)
#define GETADDR(ptr) (Node *)((uintptr_t)ptr & ~0x03)

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
			if (ISNULL(curr) || ISSPLPTR(curr)) {
				// Here we will insert data.
				return true;
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
};

void testbenchSequential() {
	nbBst myTree;
}

int main() {
	testbenchSequential();
	return 0;
}
