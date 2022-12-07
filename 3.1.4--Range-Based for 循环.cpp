//Range-Based for 循环

#include <iostream>
#include <vector>
using namespace std;


int main()
{
	//local copy
	for (int i : {2,3,5,7,9,13,17,19})
	{
		cout << i<<",";
	}
	cout << endl;

	//reference
	vector<int> vec{1,2,3,4,5,6,7};
	for (auto& i:vec)
	{
		i *= 2;
	}

	for (auto i:vec)
	{
		cout << i <<",";
	}
	cout << endl;

}
