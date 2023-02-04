#if 0
#include <iostream>
using namespace std;
struct BiTNode {
	char data;
	struct BiTNode* lchild, *rchild; // 左右孩子
};
void CreatBiTree(BiTNode* &T) { // 先序递归创建二叉树
								// 先按顺序驶入二叉树中节点的值(一个字符),空格字符代表空树
	char ch;
	for (int j = 0; j < 13; ++j) {
		if ((ch = getchar()) == '#') // getchar() 为逐个读入标准库函数
			T = NULL;
		else {
			T = new BiTNode; // 产生新的子树
			T->data = ch; // 由getchar()逐个读进来
			CreatBiTree(T->lchild); // 递归创建左子树
			CreatBiTree(T->rchild); // 递归创建右子树
		}
	}
}
void PreOrderTraverse(BiTNode* &T) { // 先序遍历二叉树
	if (T) { // 当节点不为空时执行
		cout << T->data;
		PreOrderTraverse(T->lchild);
		PreOrderTraverse(T->rchild);
	}
	else
		cout << "";
}
void Inoder(BiTNode* &T) { // 中序遍历二叉树
	if (T) {
		Inoder(T->lchild);
		cout << T->data;
		Inoder(T->rchild);
	}
	else
		cout << "";

}
void Posoder(BiTNode* &T) { // 中序遍历二叉树
	if (T) {
		Posoder(T->lchild);
		Posoder(T->rchild);
		cout << T->data;
	}
	else
		cout << "";
}
int main() {
	cout << "创建一颗二叉树,其中A~Z字符代表树的数据,用'#'表示空树:" << endl;
	BiTNode* T;
	CreatBiTree(T);
	cout << "先序递归遍历" << endl;
	PreOrderTraverse(T);
	cout << "中序递归遍历" << endl;
	Inoder(T);
	cout << "后序递归遍历" << endl;
	Posoder(T);

	return 1;
}
#endif

#include <iostream>
#include <queue>
using namespace std;
//二叉树节点定义
typedef struct BiTNode//二叉树的结点数据结构
{
	int data;
	BiTNode *lchild, *rchild;
};

//按层序输入，结点为空时，输入'#'
BiTNode *CreateBiTree(int *a, int n, int start)//*a为data，n为数组长度，start为根节点
{
	if (a[start] == '#')
		return NULL;//当根节点为空，即空树

	BiTNode* root = new BiTNode;//新建一个根结点
	root->data = a[start];//给根结点root的成员变量data,lchild,rchild赋初值
	root->lchild = NULL;
	root->rchild = NULL;

	int lnode = 2 * start + 1;//用根节点确定左节点的位置
	int rnode = 2 * start + 2;//用根节点确定右节点的位置

	if (lnode < n ) 
		root->lchild = CreateBiTree(a, n, lnode);//lnode替换start，为左子树的根节点
		

	if (rnode < n) 
		root->rchild = CreateBiTree(a, n, rnode);//rnode替换start，为右子树的根节点	

	return root;
}

//先序遍历函数（递归完成）
void PreOrderTraversal(BiTNode *T)
{
	if (T) {
		cout << T->data << " ";
		PreOrderTraversal(T->lchild);
		PreOrderTraversal(T->rchild);
	}
}

void InOrderTraversal(BiTNode* &T) { // 中序遍历二叉树
	if (T) {
		InOrderTraversal(T->lchild);
		cout << T->data << " ";
		InOrderTraversal(T->rchild);
	}

}
void PostOrderTraversal(BiTNode* &T) { // 中序遍历二叉树
	if (T) {
		PostOrderTraversal(T->lchild);
		PostOrderTraversal(T->rchild);
		cout << T->data << " ";
	}
}

//层序遍历--队列
void LevelOrderTraverse(BiTNode *T)
{
	queue<BiTNode *> Q;//定义一个queue二叉树对象Q
	if (T == NULL) 
		return;

	Q.push(T);//入队根指针  push：入队
	while (!Q.empty()) {
		BiTNode *cur = Q.front();//当前指针指向队列的第一个元素
		Q.pop();// pop：出队
		cout << cur->data << " ";
		if (cur->lchild)
			Q.push(cur->lchild);

		if (cur->rchild) 
			Q.push(cur->rchild);
	}
	cout << endl;
}
//递归 - 深度优先搜索（DFS）
int maxDepthDFS(BiTNode* T) {
	if (T == NULL) 
		return 0;
	else {
		int left_height = maxDepthDFS(T->lchild);
		int right_height = maxDepthDFS(T->rchild);
		return max(left_height, right_height) + 1;
	}
}
//广度优先搜索（BFS）
int maxDepthBFS(BiTNode* T) {
	if (!T) 
		return 0;  //如果没有节点，输出0

	int depth = 0;
	queue <BiTNode*> q;   //定义结构体队列q
	q.push(T);  //将根节点加入进去
	while (!q.empty()) {  //当队列不为空时，即表示二叉树还存在左右子树
		int currentLevelSize = q.size();  //当前队列q内的元素个数
		for (int i = 1; i <= currentLevelSize; i++) {
			auto node = q.front(); q.pop();  //node取为q的第一个元素，再去掉这个元素
			if (node->lchild) q.push(node->lchild);  //如果存在左节点，给队列q加入该节点
			if (node->rchild) q.push(node->rchild);
		}
		depth++;
	}
	return depth;
}

int minDepth(BiTNode* T) {
	if (T == NULL) return 0;  //空树深度为0
	if (T->lchild == NULL && T->rchild == NULL) return 1; //左右子树为空，深度为1
															 //根据题意，若一颗子树为空，则最小深度为非空子树的最小深度加一
	if (T->lchild == NULL && T->rchild != NULL) return minDepth(T->rchild) + 1;
	if (T->lchild != NULL && T->rchild == NULL) return minDepth(T->lchild) + 1;
	//若两颗子树都非空，则最小深度为左右子树最小深度较小者加一
	return min(minDepth(T->lchild), minDepth(T->rchild)) + 1;
}

bool check_tree(BiTNode *root)
{
	if (root == NULL)
		return true;

	if (root->lchild == NULL && root->rchild == NULL)
		return true;

	if (root->lchild == NULL && root->rchild!=NULL)
	{
		if (false == (root->rchild->data > root->data) )
		{
			return false;
		}
	}
	else if (root->lchild != NULL && root->rchild == NULL)
	{
		if (false == (root->lchild->data < root->data) )
		{
			return false;
		}
	}
	else
	{
		if (false == (root->rchild->data > root->data) && (root->lchild->data < root->data))
		{
			return false;
		}
	}
	
	return check_tree(root->lchild) && check_tree(root->rchild);
}

void InOrder(BiTNode* root, vector<int>& vecData)
{
	if (root!=NULL)
	{
		InOrder(root->lchild, vecData);
		vecData.push_back(root->data);
		InOrder(root->rchild, vecData);
	}
}
bool CheckBST1(BiTNode* root)
{
	if (root==NULL)
	{
		return false;
	}
	vector<int> vecData;
	InOrder(root, vecData);

	for (int i = 0; i < (vecData.size()-1);i++)
	{
		if (vecData[i]>vecData[i+1])
		{
			return false;
		}
	}

	return true;
}


bool CheckBST2(BiTNode* root,int nMin,int nMax)
{
#if 0
	if (root == NULL)
		return true;

	if (root->lchild == NULL && root->rchild == NULL)
		return true;

	else if (root->lchild == NULL && root->rchild != NULL)
	{
		if (root->rchild->data<nMin || root->rchild->data>nMax)
		{
			return false;
		}
	}
	else if (root->rchild == NULL && root->lchild != NULL)
	{
		if (root->lchild->data<nMin || root->lchild->data>nMax)
		{
			return false;
		}
	}
	else
	{
		if (false == (root->rchild->data > nMin && root->lchild->data < nMax))
			return false;
	}
	return CheckBST2(root->lchild, nMin, root->data) && CheckBST2(root->rchild, root->data, nMax);
#else
	if (root == NULL)
		return true;


	if (root->data>nMin && root->data<nMax)
	{
		return CheckBST2(root->lchild, nMin, root->data) && CheckBST2(root->rchild, root->data, nMax);
	}
	else
	{
		return false;
	}
#endif

}


bool helper(BiTNode* root, long low, long high) {
	// Empty trees are valid BSTs.
	if (root == NULL) {
		return true;
	}

	// Condition of Binary Search Tree 
	// left side node values of the root should be less value than the root and right side should be greater
	// If that is true check recursivley for the root child's

	if ((root->data < high) && (root->data > low)) {
		return (helper(root->lchild, low, root->data) && helper(root->rchild, root->data, high));

	}
	else {
		//if this root node violates the min/max constraint
		return false;
	}

}

bool isValidBST(BiTNode* root) {
	bool ans = helper(root, INT_MIN, INT_MAX);
	return ans;
}



void test1() {
	BiTNode *t;
	int data[] = { 1, 2, 3, 4, 5, '#', 6, '#', '#', 7, 8 };
	//前序遍历：1 2 4 5 7 8 3 6
	t = CreateBiTree(data, sizeof(data) / sizeof(data[0]), 0);

	printf("前序遍历:   ");
	PreOrderTraversal(t);
	cout << endl;


	printf("中序遍历:   ");
	InOrderTraversal(t);
	cout << endl;


	printf("后序遍历:   ");
	PostOrderTraversal(t);
	cout << endl;

	printf("层序遍历:   ");
	LevelOrderTraverse(t);
}

void test2() {
	BiTNode *t;
	int data[] = { 1, 2, 3, 4, 5, '#', 6, 7 };
	//前序遍历：1 2 4 7 5 3 6
	t = CreateBiTree(data, sizeof(data) / sizeof(data[0]), 0);

	printf("前序遍历:   ");
	PreOrderTraversal(t);
	cout << endl;

	printf("中序遍历:   ");
	InOrderTraversal(t);
	cout << endl;


	printf("后序遍历:   ");
	PostOrderTraversal(t);
	cout << endl;

	printf("层序遍历:   ");
	LevelOrderTraverse(t);
}

void test3()
{
	//判断二叉树,每个节点满足  左子节点<父节点<右子节点
	BiTNode *t;
	int data[] = { 5, 2, 9,'#',7,3,10 };
	t = CreateBiTree(data, sizeof(data) / sizeof(data[0]), 0);

	bool b = check_tree(t);
	cout << b;

	//LevelOrderTraverse(t);
	/*
	int depthMax = maxDepthDFS(t);
	depthMax = maxDepthBFS(t);
	int depthMin = minDepth(t);
	*/
}

void test4()
{
	//判断搜索二叉树
	//int a[] = { 1,2,3,'#',5,6,'#','#',9,10 };
	//int a[] = { 1, 2, 3, 4, 5, '#', 6, '#', '#', 7, 8 };

	int a[] = { 5,2,9,'#',7 };
	BiTNode*	root = CreateBiTree(a, sizeof(a) / sizeof(int), 0);
	InOrderTraversal(root);


	bool b = CheckBST1(root);
	cout << endl;
	cout << b;

	b = CheckBST2(root, INT_MIN, INT_MAX);
	cout << endl;
	cout << b;

	b = isValidBST(root);
	cout << endl;
	cout << b;
}




int main()
{
	//test1();
	//test2();

	test3();
	//test4()
	

	return 0;
}
