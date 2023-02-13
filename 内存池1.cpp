#include "iostream"
using namespace std;

template<int ObjectSize, int NumofObjects = 20>
class MemPool {
private:
	//空闲节点结构体
	struct FreeNode {
		FreeNode* pNext;
		char data[ObjectSize];
	};

	//内存块结构体
	struct MemBlock {
		MemBlock* pNext;
		FreeNode data[NumofObjects];
	};

	FreeNode* freeNodeHeader;
	MemBlock* memBlockHeader;

public:
	MemPool() {
		freeNodeHeader = NULL;
		memBlockHeader = NULL;
	}

	~MemPool() {
		MemBlock* ptr;
		while (memBlockHeader)
		{
			ptr = memBlockHeader->pNext;
			delete memBlockHeader;
			memBlockHeader = ptr;
		}
	}
	void* malloc();
	void free(void*);
};

// 分配空闲的结点。
template<int ObjectSize, int NumofObjects>
void* MemPool<ObjectSize, NumofObjects>::malloc() {
	//无空闲节点，申请新内存块
	if (freeNodeHeader == NULL) {
		MemBlock* newBlock = new MemBlock;
		newBlock->pNext = NULL;

		freeNodeHeader = &newBlock->data[0];	 //设置内存块的第一个节点为空闲节点链表的首节点
												 //将内存块的其它节点串起来
		for (int i = 1; i < NumofObjects; ++i) {
			newBlock->data[i - 1].pNext = &newBlock->data[i];
		}
		newBlock->data[NumofObjects - 1].pNext = NULL;

		// 首次申请内存块
		if (memBlockHeader == NULL) {
			memBlockHeader = newBlock;
		}
		else {
			// 将新内存块加入到内存块链表。
			newBlock->pNext = memBlockHeader;
			memBlockHeader = newBlock;
		}
	}
	// 返回空节点闲链表的第一个节点。
	void* freeNode = freeNodeHeader;
	freeNodeHeader = freeNodeHeader->pNext;
	return freeNode;
}

// 释放已经分配的结点。
template<int ObjectSize, int NumofObjects>
void MemPool<ObjectSize, NumofObjects>::free(void* p) {
	FreeNode* pNode = (FreeNode*)p;
	pNode->pNext = freeNodeHeader;	//将释放的节点插入空闲节点头部
	freeNodeHeader = pNode;
}

class ActualClass {
	static int count;
	int No;

public:
	ActualClass() {
		No = count;
		count++;
	}

	void print() {
		cout << this << ": ";
		cout << "the " << No << "th object" << endl;
	}

	void* operator new(size_t size);
	void operator delete(void* p);
};

// 定义内存池对象
MemPool<sizeof(ActualClass), 2> mp;

void* ActualClass::operator new(size_t size){
	return mp.malloc();
}

void ActualClass::operator delete(void* p) {
	mp.free(p);
}

int ActualClass::count = 0;

int main() {
	ActualClass* p1 = new ActualClass;
	p1->print();

	ActualClass* p2 = new ActualClass;
	p2->print();
	delete p1;

	p1 = new ActualClass;
	p1->print();

	ActualClass* p3 = new ActualClass;
	p3->print();

	delete p1;
	delete p2;
	delete p3;
}


//源地址：https://blog.csdn.net/K346K346/article/details/49538975
/*
3.3 程序分析
阅读以上程序，应注意以下几点。
（1）对一种特定的类对象而言，内存池中内存块的大小是固定的，内存节点的大小也是固定的。内存块在申请之初就被划分为多个内存节点，每个 Node 的大小为 ItemSize。刚开始，所有的内存节点都是空闲的，被串成链表。

（2）成员指针变量 memBlockHeader 是用来把所有申请的内存块连接成一个内存块链表，以便通过它可以释放所有申请的内存。freeNodeHeader 变量则是把所有空闲内存节点串成一个链表。freeNodeHeader为空则表明没有可用的空闲内存节点，必须申请新的内存块。

（3）申请空间的过程如下。在空闲内存节点链表非空的情况下，malloc 过程只是从链表中取下空闲内存节点链表的头一个节点，然后把链表头指针移动到下一个节点上去。否则，意味着需要一个新的内存块。这个过程需要申请新的内存块切割成多个内存节点，并把它们串起来，内存池技术的主要开销就在这里。

（4）释放对象的过程就是把被释放的内存节点重新插入到内存节点链表的开头。最后被释放的节点就是下一个即将被分配的节点。

（5）内存池技术申请/释放内存的速度很快，其内存分配过程多数情况下复杂度为 O(1)，主要开销在 freeNodeHeader 为空时需要生成新的内存块。内存节点释放过程复杂度为 O(1)。

（6） 在上面的程序中，指针 p1 和 p2 连续两次申请空间，它们代表的地址之间的差值为 8，正好为一个内存节点的大小（sizeof(FreeNode)）。指针 p1 所指向的对象被释放后，再次申请空间，得到的地址与刚刚释放的地址正好相同。指针 p3 多代表的地址与前两个对象的地址相聚很远，原因是第一个内存块中的空闲内存节点已经分配完了，p3 指向的对象位于第二个内存块中。

以上内存池方案并不完美，比如，只能单个单个申请对象空间，不能申请对象数组，内存池中内存块的个数只能增大不能减少，未考虑多线程安全等问题。现在，已经有很多改进的方案，请读者自行查阅相关资料。

*/
