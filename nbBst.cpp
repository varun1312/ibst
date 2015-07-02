#include <iostream>
#include <climits>

#define MARK_NODE(pointer, status) (((uintptr_t)pointer & ~0x03) | status)
#define ISNULL(pointer) (((uintptr_t)pointer & 0x03) == NULLPTR)
#define GET_DATA(pointer) *(int *)((uintptr_t)pointer & ~0x03)
#define GET_ADDR(node, lr) (Node *)((uintptr_t)node->child[lr] & ~0x03)

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
		int *data;
		Node *child[2];
		Node(int data) {
			this->data = new int(data);
			this->child[LEFT] = this->child[RIGHT] = (Node *)MARK_NODE(this, NULLPTR);
		}
	};
	Node *root;
public:
	nbBst() {
		std::cout<<"I am in BST constructor"<<std::endl;
		root = new Node(INT_MAX);
	}

	bool insert_tree(Node *startNode, int data) {
		Node *pred = startNode;
		Node *curr = startNode;
		Node *ancNode = startNode;
		while(true) {
			if (ISNULL(curr)) {
				// Here actual insertion happens.
			}
			int *dataPointer = curr->data;
			int currData = GET_DATA(dataPointer);
			if (currData == data) {
				// Here we need to check if the node is marked or not.
				// If it is marked, then we need to add the node again.
				// Returning false for now.
				return false;
			}
			else if (currData > data) {
				pred = curr, curr = GET_ADDR(curr, RIGHT);
			}
			else if (currData < data) {
				ancNode = pred, pred = curr, curr = GET_ADDR(curr, LEFT);
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

int main(void) {
	testbenchSequential();
	return 0;	
}
