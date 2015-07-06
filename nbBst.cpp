#include <iostream>
#include <climits>
#include <vector>
#include <thread>

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
		}
	};

	Node *root;
public:	
	nbBst() {
		root = new Node(INT_MAX);
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
