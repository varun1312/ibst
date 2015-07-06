#include <iostream>
#include <climits>
#include <vector>
#include <thread>

#define GETDATA(node) *(int *)((uintptr_t)node->dataPtr & ~0x03)
#define GETADDR(node, lr)  (Node *)((uintptr_t)node->child[lr] & ~0x03)
#define ISNULL(node) (node == NULL)
#define UNQNULL(node) (((uintptr_t)node & 0x03) == UNQNULL)
#define MARKNODE(node, status) (((uintptr_t) node & ~0x03) | status)
#define CAS(ptr, source, sourceState, target, targetState) \
		__sync_bool_compare_and_swap(\
			ptr, \
			MARKNODE(source, sourceState), \
			MARKNODE(target, targetState))

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

class nbBst {
private:
	class Node {
	public:
		int *dataPtr;
		Node *child[2];
		Node(int data) {
			this->dataPtr = new int(data);
			child[LEFT] = child[RIGHT] = NULL;	
		}
	};

	Node *root;
public:	
	nbBst() {
		root = new Node(INT_MAX);
		std::cout<<"Initialized root of the tree"<<std::endl;
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
				return insert_data(pred, curr, NORMAL, data);	
			}
			if (UNQNULL(curr)) {
				int ancNodeDataCurr = GETDATA(ancNode);
				if ((ancNodeDataSeen != ancNodeDataCurr) && (data > ancNodeDataCurr))
					return insert_tree(ancNode, data);
				return insert_data(pred, curr, UNQNULL, data);
			}
			int currData = GETDATA(curr);
			if (data > currData) {
				pred = curr;
				curr = GETADDR(curr, RIGHT);
			}
			else if (data < currData) {
				ancNode = pred;
			    ancNodeDataSeen = GETDATA(ancNode);
				pred = curr;
				curr = GETADDR(curr, LEFT);
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
		if (ISNULL(node) || UNQNULL(node))
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
