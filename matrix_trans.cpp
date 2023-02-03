#include <iostream>
#include <vector>
using namespace std;
vector<vector<int>> matrix_trans(vector<vector<int>>&  vec)
{
	vector<vector<int>>  vec_tran;
	vec_tran.resize(vec[0].size());
	for (int i = 0; i < vec_tran.size(); i++)
	{
		vec_tran[i].resize(vec.size());
	}

	for (int i = 0; i < vec_tran.size();i++)
	{
		for (int j = 0; j < vec_tran[i].size();j++)
		{
			vec_tran[i][j] = vec[j][i];
		}
	}

	return vec_tran;
}
void print_matrix(vector<vector<int>>&  vec)
{
	for (int i = 0; i < vec.size(); i++)
	{
		for (int j = 0; j < vec[i].size(); j++)
		{
			cout <<"("<< i << "," << j << ")=" << vec[i][j] << " ";
		}
		cout << endl;
	}
}
void main()
{
	vector<vector<int>>  vec;
	for (int i = 0; i < 5;i++)
	{
		vec.push_back(vector<int>());
		for (int j = 0; j < 7;j++)
		{
			vec[i].push_back(j+i*10);
		}		
	}

	print_matrix(vec);
	

	cout << endl;
	cout << endl;
	vector<vector<int>>  vec_tran= matrix_trans(vec);
	print_matrix(vec_tran);
	
}
