#include <iostream>
#include <climits>

#define MARK_NODE(pointer, status) (((uintptr_t)pointer & ~0x03) | status)
#define ISNULL(pointer) (((uintptr_t)pointer & 0x03) == NULLPTR) 
#define GET_DATA_PTR(pointer) (((Node *)((uintptr_t)pointer & ~0x03))->data)
#define GET_DATA(pointer) *(int *)((uintptr_t)pointer & ~0x03)
#define GET_ADDR(node) (Node *)((uintptr_t)node & ~0x03)
#define CAS(ptr, source, sourceStatus, target, targetStatus) \
	__sync_bool_compare_and_swap( \
		ptr,\
		MARK_NODE(source, sourceStatus),\
		MARK_NODE(target, targetStatus) )


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
				// Before inserting, we also need to check the validity of insertion.
				int *predDataPointer = pred->data;
				int predData = GET_DATA(predDataPointer);
				Node *node = new Node(data);
				if (data > predData) {
					if (CAS(&pred->child[RIGHT], curr, NULLPTR, node, NORMAL))
						return true;
					else
						/* This is the retrial point. Here we are restrating for now, but, we need to back track
						in future */
						return insert_tree(root, data);
				}
				else {
					if (CAS(&pred->child[LEFT], curr, NULLPTR, node, NORMAL))
						return true;
					else
						/* This is the retrial point. Here we are restrating for now, but, we need to back track
						in future */
						return insert_tree(root, data);
				}
			}
			int *dataPointer = GET_DATA_PTR(curr);
			int currData = GET_DATA(dataPointer);
			if (currData == data) {
				// Here we need to check if the node is marked or not.
				// If it is marked, then we need to add the node again.
				// Returning false for now.
				return false;
			}
			else if (data > currData) {
				pred = GET_ADDR(curr), curr = curr->child[RIGHT];
			}
			else if (data < currData) {
				ancNode = pred, pred = GET_ADDR(curr), curr = curr->child[LEFT]; 
			}
		}		
	}
	
	bool insert(int data) {
		return insert_tree(root, data);
	}

	void print_tree(Node *node) {
		if (ISNULL(node))
			return;
		print_tree(node->child[LEFT]);
		std::cout<<GET_DATA(GET_DATA_PTR(node))<<std::endl;
		print_tree(node->child[RIGHT]);
	}

	void print() {
		return print_tree(GET_ADDR(root->child[LEFT]));
	}
};

void testbenchSequential() {
	nbBst myTree;
	myTree.insert(10);
	myTree.insert(20);
	myTree.insert(30);
	myTree.insert(40);
	myTree.insert(50);
	myTree.print();
}

int main(void) {
	testbenchSequential();
	return 0;	
}
