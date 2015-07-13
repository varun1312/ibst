#include <iostream>
#include <climits>
#include <vector>
#include <thread>

#define GETADDR(node) ((Node *)((uintptr_t)node & ~0x03))
#define STATUS(node) ((uintptr_t)node & 0x03)
#define ISNULL(node) ((GETADDR(node) == NULL) || (STATUS(node) == UNQNULL))
#define ISUNQNULL(node) (STATUS(node) == UNQNULL)
#define GETDATA(node) *(int *)((uintptr_t)(GETADDR(node)->dataPtr) & ~0x03)
#define MARKNODE(node, status) (Node *)(((uintptr_t)node & ~0x03) | status)
#define CAS(ptr, source, sourceState, target, targetState) \
		__sync_bool_compare_and_swap(\
		ptr, \
		MARKNODE(source, sourceState), \
		MARKNODE(target, targetState))
#define ISMARKED(node) ((STATUS(GETADDR(node)->child[RIGHT]) == MARKED) || (STATUS(GETADDR(node)->child[LEFT]) == MARKED) || (STATUS(GETADDR(node)->dataPtr) == MARKED))
	


const int LEFT = 0, RIGHT = 1;
const int NORMAL = 0, MARKED = 1, PROMOTE = 2, UNQNULL = 3;

enum markStatus_t {
	REMOVE_0C,
	REMOVE_1C,
	REMOVE_2C,
	ABORT_REMOVE
};

class nbBst {
private:
	class Node {
	public:
		int *dataPtr;
		Node *child[2];
		Node(int data) {
			dataPtr = new int(data);
			child[LEFT] = child[RIGHT] = (Node *)UNQNULL;
		}
	};
	Node *root;
public:
	nbBst() {
		root = new Node(INT_MAX);
	}

	markStatus_t markNode(Node *node, int* dataPtr, int data) {
		Node *rp = GETADDR(node)->child[RIGHT];
		while(STATUS(rp) != NORMAL) {
			if (GETADDR(rp) == root) {
				// Based on data status, go and help or remove the node
			}
			if (STATUS(rp) == MARKED) {
				// Mark Left irrespective of any reason
				Node *lp = GETADDR(node)->child[LEFT];
				while(STATUS(lp) != MARKED) {
					if (STATUS(lp) == NORMAL) {
						if (CAS(&GETADDR(node)->child[LEFT], lp, NORMAL, lp, MARKED)) {
							break;
						}
					}
					else if (STATUS(lp) == UNQNULL) {
						if (CAS(&GETADDR(node)->child[LEFT], lp, UNQNULL, NULL, MARKED)) {
							break;
						}
					}
					lp = GETADDR(node)->child[LEFT];
				}
				// Break the loop.
				break;
			}
			else if (STATUS(rp) == PROMOTE) {
				// go and help remove. This is a bit tricky. It must be dealt with care
			}
			else if (STATUS(rp) == UNQNULL) {
				if (CAS(&GETADDR(node)->child[RIGHT], rp, UNQNULL, NULL, MARKED)) {
					// Mark Left irrespective of any reason
					Node *lp = GETADDR(node)->child[LEFT];
					while(STATUS(lp) != MARKED) {
						if (STATUS(lp) == NORMAL) {
							if (CAS(&GETADDR(node)->child[LEFT], lp, NORMAL, lp, MARKED)) {
								break;
							}
						}
						else if (STATUS(lp) == UNQNULL) {
							if (CAS(&GETADDR(node)->child[LEFT], lp, UNQNULL, NULL, MARKED)) {
								break;
							}
						}
						lp = GETADDR(node)->child[LEFT];
					}
					// Break the loop.
					break;
				}
			}				
			rp = GETADDR(node)->child[RIGHT];
		}
		Node *lp = GETADDR(node)->child[LEFT];
		while(STATUS(lp) != NORMAL) {
			if (STATUS(lp) == MARKED) {
				// Check and mark right. This is a very tricky code.
				Node *rp = GETADDR(node)->child[RIGHT];
				while(STATUS(rp) != MARKED) {
					if (STATUS(rp) == MARKED) {
						break;
					}
					else if (STATUS(rp) == NORMAL) {
						if (CAS(&GETADDR(node)->child[RIGHT], rp, NORMAL, rp, MARKED)) {
							break;
						}
					}
					else if (STATUS(rp) == UNQNULL) {
						if (CAS(&GETADDR(node)->child[RIGHT], rp, UNQNULL, NULL, MARKED)) {
							break;
						}
					}
					else if (STATUS(rp) == PROMOTE) {
						// This is a very tricky code. Here we must help remove
					}
					rp = GETADDR(node)->child[RIGHT];
				}	
				// Break the loop;
				break;
			}
			else if (STATUS(lp) == UNQNULL) {
				if (CAS(&GETADDR(node)->child[LEFT], lp, UNQNULL, NULL, MARKED)) {
					// Check and mark right. This is a very tricky code.
					while(STATUS(rp) != MARKED) {
						if (STATUS(rp) == MARKED) {
							break;
						}
						else if (STATUS(rp) == NORMAL) {
							if (CAS(&GETADDR(node)->child[RIGHT], rp, NORMAL, rp, MARKED)) {
								break;
							}
						}
						else if (STATUS(rp) == UNQNULL) {
							if (CAS(&GETADDR(node)->child[RIGHT], rp, UNQNULL, NULL, MARKED)) {
								break;
							}
						}
						else if (STATUS(rp) == PROMOTE) {
							// This is a very tricky code. Here we must help remove
						}
						rp = GETADDR(node)->child[RIGHT];
					}
					// Break the loop;
					break;
				}
			}
		}
		// Here we check the status of the node and then return appropriate value
		Node *rP = GETADDR(node)->child[RIGHT];
		Node *lP = GETADDR(node)->child[LEFT];
		if ((GETADDR(rP) == NULL) && (GETADDR(lP) == NULL))
			return REMOVE_0C;
		else if ((GETADDR(rP) == NULL) || (GETADDR(lP) == NULL))
			return REMOVE_1C;
		if (CAS(&GETADDR(node)->dataPtr, dataPtr, NORMAL, dataPtr, MARKED))
			return REMOVE_2C;
		if (GETDATA(node) != data)
			return ABORT_REMOVE;	
		return REMOVE_2C;
	}

	bool remove_tree(Node *startNode, int data) {
		Node *ancNode= startNode;
		Node *pred = startNode;
		Node *curr = startNode;
		int ancNodePrevData, ancNodeCurrData;
		while(true) {
			if (ISNULL(curr)) {
				ancNodeCurrData = GETDATA(ancNode);
				if (ancNodePrevData != ancNodeCurrData)
					return remove_tree(ancNode, data);
				return false;
			}
			int currData = GETDATA(curr);
			if (data > currData) {
				pred = GETADDR(curr);
				curr = GETADDR(curr)->child[RIGHT];
			}
			else if (data < currData) {
				ancNode = pred;
				ancNodePrevData = GETDATA(ancNode);
				pred = GETADDR(curr);
				curr = GETADDR(curr)->child[LEFT];
			}
			else if (data == currData) {
				// Here we mark the data;
				markStatus_t stat = markNode(curr, GETADDR(curr)->dataPtr, data);
				//std::cout<<data<<" : "<<stat<<std::endl;
				return true;
			}
		}
	}

	bool remove(int data) {
		return remove_tree(root, data);
	}

	bool insert_data(Node *pred, Node *curr, int status, int data) {
		int predData = GETDATA(pred);
		Node *myNode = new Node(data);
		if (data > predData) {
			if (CAS(&pred->child[RIGHT], curr, status, myNode, NORMAL)) 	
				return true;
			else 
				// Here we are restarting but we should backtrack from pred
				return insert_tree(root, data);	
		}
		else {
			if (CAS(&pred->child[LEFT], curr, status, myNode, NORMAL)) 
				return true;
			else
				// Here we are restarting but we should backtrack from pred
				return insert_tree(root, data);
		}
	}

	bool insert_tree(Node *startNode, int data) {
		Node *ancNode = startNode;
		Node *pred = startNode;
		Node *curr = startNode;	
		int ancNodePrevData, ancNodeCurrData;
		while(true) {
			if (ISUNQNULL(curr)) {
				ancNodeCurrData = GETDATA(ancNode);	
				if (ancNodePrevData != ancNodeCurrData)
					return insert_tree(ancNode, data);
				return insert_data(pred, curr, UNQNULL, data);
			}
			else if ((GETADDR(curr) == NULL) && (STATUS(curr) == MARKED)) {
				ancNodeCurrData = GETDATA(ancNode);	
				if (ancNodePrevData != ancNodeCurrData)
					return insert_tree(ancNode, data);
				// Here we help the corresponding delete by removing it from the tree.
				// This requires traversing the predecessor parent. Something like below
				// return insert_data(pred->parent, pred, NORMAL, data);
				// return false for now;
				return false;
			}
			int currData = GETDATA(curr);
			if (data == currData) {
				/* Check if the node is marked or not. 
				If it is, then traverse right and then insert the node again*/
				return false;
			}
			else if (data > currData) {
				pred = GETADDR(curr);
				curr = GETADDR(curr)->child[RIGHT];
			}
			else if (data < currData) {
				ancNode = pred;
				ancNodePrevData = GETDATA(ancNode);
				pred = GETADDR(curr);
				curr = GETADDR(curr)->child[LEFT]; 
			}
		}
	}

	bool insert(int data) {
		return insert_tree(root, data);
	}

	void print_tree(Node *node) {
		if (ISNULL(node))
			return;
		print_tree(GETADDR(node)->child[LEFT]);
		if (!ISMARKED(node))
			std::cout<<GETDATA(node)<<std::endl;
		print_tree(GETADDR(node)->child[RIGHT]);
	}

	void print() {
		print_tree(root->child[LEFT]);
	}
};

void testbenchSequential() {
	nbBst myTree;
	srand(time(NULL));
	for (int i = 0; i < 100; i++)
		myTree.insert(rand());
	myTree.print();
}

void testbenchParallel() {
	nbBst myTree;
	srand(time(NULL));
	const int numThreads = 100;
	std::vector<std::thread> addT(numThreads);
	std::vector<std::thread> removeT(numThreads);
	int arr[numThreads];
	for (int i = 0; i < numThreads; i++)
		arr[i] = rand();
	for (int i = 0; i < numThreads; i++)
		addT[i] = std::thread(&nbBst::insert, &myTree, arr[i]);
	for (int i = 0; i < numThreads; i++)
		addT[i].join();
	myTree.print();
	std::cout<<"Removing Elements"<<std::endl;
	for (int i = 0; i < numThreads; i++)
		removeT[i] = std::thread(&nbBst::remove, &myTree, arr[(i+5)%numThreads]);
	for (int i = 0; i < numThreads; i++)
		removeT[i].join();
/*	int removeElement;
	while(removeElement != -1) {
		std::cout<<"Enter an element to remove : ";
		std::cin>>removeElement;
		myTree.remove(removeElement);
	}
*/
	myTree.print();
}

int main(void) {
	//testbenchSequential();
	testbenchParallel();
	return 0;
}
