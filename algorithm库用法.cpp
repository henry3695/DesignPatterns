#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <set>
using namespace std;


void test1()
{
	cout << endl<< "----------------test1--------------" << endl;
	int a=25,b=100;
	cout << max(a, b) << endl;
	cout << min(a, b) << endl;
	cout << abs(a - b) << endl;
}


bool myfn(int i, int j) { return i < j; }

struct myclass {
	bool operator() (int i, int j) { return i < j; }
} myobj;


void test2()
{
	cout << endl << "----------------test2--------------" << endl;

	int myints[] = { 3,7,2,5,6,4,9 };

	// using default comparison:
	std::cout << "The smallest element is " << *std::min_element(myints, myints + 7) << '\n';
	std::cout << "The largest element is " << *std::max_element(myints, myints + 7) << '\n';

	// using function myfn as comp:
	std::cout << "The smallest element is " << *std::min_element(myints, myints + 7, myfn) << '\n';
	std::cout << "The largest element is " << *std::max_element(myints, myints + 7, myfn) << '\n';

	// using object myobj as comp:
	std::cout << "The smallest element is " << *std::min_element(myints, myints + 7, myobj) << '\n';
	std::cout << "The largest element is " << *std::max_element(myints, myints + 7, myobj) << '\n';


	std::cout << "The smallest element is " << *std::min_element(myints, myints + 7, [](int a, int b)->bool {return a < b; }) << '\n';
	std::cout << "The largest element is " << *std::max_element(myints, myints + 7, [](int a, int b)->bool {return a < b; }) << '\n';



	std::vector<int>v;
	v.push_back(10);
	v.push_back(20);
	v.push_back(30);
	std::cout << *max_element(v.begin(), v.end()) << std::endl;
}

void test3()
{
	cout << endl << "----------------test3--------------" << endl;
	int a = 10, b = 20;
	swap(a, b);
	cout << a << " " << b << endl;
	int nums[4] = { 0, 1, 2, 3 };
	swap(nums[0], nums[3]);
	for (int i = 0; i < 3; i++)
		cout << nums[i] << " ";
	cout << endl;
}

void test4()
{
	cout << endl << "----------------test4--------------" << endl;

	vector<int> array{1, 2, 3, 4, 5};//动态数组
	
	reverse(array.begin(), array.end());
	for (int i = 0; i < array.size(); i++)
		cout << array[i] << " ";
	cout << endl;


	cout << "--------" << endl;
	string str = "hello world";//字符串
	reverse(str.begin(), str.end());
	cout << str << endl;
	cout << "--------" << endl;

	int arr1[5] = {1,2,3,4,5}; //数组
	reverse(arr1, arr1 + 5);

	for (int i = 0; i < 5; i++)
	{
		cout << arr1[i] << " ";
	}
	cout << endl;
}


bool cmp(int a, int b)
{
	return a > b;
}

void test5()
{
	cout << endl << "----------------test5--------------" << endl;

	vector<int> sortv{ 9,2,3,4,5 };
	sort(sortv.begin(), sortv.end());
	for (int i = 0; i < sortv.size(); i++)
	{
		cout << sortv[i] << ",";
	}
	cout << endl;

	//数组
	int sortnums[5] = {3,2,1,4,5};	
	sort(sortnums, sortnums + 5);
	for (int i = 0; i < 5; i++)
		cout << sortnums[i] << ",";
	//此sort是升序排序
	cout << endl;


	//降序
	sort(sortv.begin(), sortv.end(), cmp);
	for (int i = 0; i < sortv.size(); i++)
	{
		cout << sortv[i] << ",";
	}
	cout << endl;



	sort(sortnums, sortnums + 5, [](int a, int b)->bool {return a > b; });
	for (int i = 0; i < 5; i++)
		cout << sortnums[i] << ",";

	cout << endl;




}



struct myComp
{
	bool operator()(const int &a, const int &b)
	{
		return a>b;
	}
}d;
		
struct Info
{
	Info(string n,float s)
	{
		name = n;
		score = s;
	}
	string name;
	float score;
	//重载“<”操作符，自定义排序规则
	bool operator < (const Info &a) const
	{
		//按score从大到小排列
		return a.score < score;
	}
};

bool compInof(const Info if1, const Info if2)
{
	return if1.score < if2.score;
}



int main()
{
	test1();
	test2();
	test3();
	test4();
	test5();

	set<int, myComp> s;
	s.insert(10);
	s.insert(20);
	s.insert(5);
	for (auto i:s)
	{
		cout << i<<",";
	}
	cout << endl;

	vector<int> v;
	v.push_back(3);
	v.push_back(5);
	v.push_back(4);
	sort(v.begin(), v.end(), d);
	for (auto i : v)
	{
		cout << i << ",";
	}
	cout << endl;


	set<Info> s1;
	s1.insert(Info("test4",4.4));
	s1.insert(Info("test1",1.1));
	s1.insert(Info("test2",2.2));
	s1.insert(Info("test3",3.3));
	for (auto i : s1)
	{
		cout << i.name<<i.score << "\n";
	}


	cout << endl;
	vector<Info> v1;
	v1.push_back(Info("test4", 4.4));
	v1.push_back(Info("test1", 1.1));
	v1.push_back(Info("test2", 2.2));
	v1.push_back(Info("test3", 3.3));
	
	sort(v1.begin(), v1.end(), compInof);
	for (auto i : v1)
	{
		cout << i.name << i.score << "\n";
	}

	cout << endl;
	system("pause");
}
