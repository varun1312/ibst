#include <iostream>
#include <climits>

#define MARK_NODE(pointer, status) (((uintptr_t)pointer & ~0x03) | status)

enum status_t {
	NORMAL,
	MARKED,
	PROMOTE,
	NULLPTR
};

class nbBst {
	class Node {
	public:
		int *data;
		Node *left;
		Node *right;
		Node(int data) {
			this->data = new int(data);
			left = right = (Node *)MARK_NODE(this, NULLPTR);
		}
	};
	Node *root;
public:
	nbBst() {
		std::cout<<"I am in BST constructor"<<std::endl;
		root = new Node(INT_MAX);
	}
};

void testbenchSequential() {
	nbBst myTree;
}

int main(void) {
	testbenchSequential();
	return 0;	
}
