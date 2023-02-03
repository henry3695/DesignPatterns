#include<iostream>
#include<algorithm>
using namespace std;
bool sign[5][5];
int  sign_data[5][5];
static int method = 0;
int dx[] = { -2,-2,-1,-1,1,1,2,2 };
int dy[] = { 1,-1,2,-2,2,-2,1,-1 };

bool check_boundary(int x, int y)
{
	if (false == ((x >= 0 && x < 5) && (y >= 0 && y < 5)))
		return false;

	if (sign[x][y] == true)
		return false;

	return true;
}

void dfs(int steps, int x, int y)
{
	if (steps == 24)
	{
		method++;
		for (int i = 0; i < 5;i++)
		{
			for (int j = 0; j < 5;j++)
			{
				cout << sign_data[i][j] << ",";
			}
			cout << endl;
		}
		return;
	}
	// Judge boundary 
	for (int i = 0; i <= 7; i++)
	{
		int m = x + dx[i];
		int n = y + dy[i];

		if (check_boundary(m, n))
		{
			sign[m][n] = true; //mark: this grid has been covered
			sign_data[m][n] = steps+1;
			dfs(steps + 1, m, n);
			sign[m][n] = false;
			sign_data[m][n] = -1;
		}
	}
}

int main()
{
	sign[0][0] = true;
	sign_data[0][0] = 0;
	dfs(0, 0, 0);
	std::cout << "total methods:" << method << std::endl;
}
