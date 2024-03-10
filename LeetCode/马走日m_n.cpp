#include <iostream>
#include <cstring>
using namespace std;

/*

00	01	02	03	04	05
10	11	12	13	14	15
20	21	22	23	24	25
30	31	32	33	34	35
40	41	42	43	44	45

已知 m,n 二维数组
判断该数组是否存在一条线路，马可以走一次全部跳完
每次从（0，0）出发，马先走1点钟方向，顺时针搜索路线

如果存在打印线路
不存在打印 NAN

cmd /k pushd "$(CURRENT_DIRECTORY)" && g++ -o "$(NAME_PART).exe" "$(FULL_CURRENT_PATH)" && "$(NAME_PART)".exe & PAUSE & EXIT
*/

int dx[]={1,2, 2, 1,-1,-2,-2,-1};
int dy[]={2,1,-1,-2,-2,-1, 1, 2};

int method=0;

void print_arry(int* p,int m,int n)
{
	for(int x=0;x<m;x++)
	{
		for(int y=0;y<n;y++)
		{
			cout<<*(p+(x*n)+y)<<"\t";
		}
		cout<<endl;
	}
}
bool check(int* p,int step,int x,int y,int m,int n)
{
	if( (x>=0 && x<m) && (y>=0 && y<n)  )
	{
		if( *(p+(x*n)+y)==0)
		{
			return true;
		}
	}
	return false;
}

void jump(int* p,int step,int x,int y,int m,int n)
{
	if(step==(m*n))
	{
		if(method==0)
		{
			print_arry(p,m,n);
		}
		method++;
		return;
	}
	
	//八个方向扫描下一步
	for(int i=0;i<8;i++)
	{
		int x1=x+dx[i];
		int y1=y+dy[i];
		if(check(p,step+1,x1,y1,m,n))
		{
			*(p+(x1*n)+y1)=step+1;
			jump(p,step+1,x1,y1,m,n);
			*(p+(x1*n)+y1)=0;
		}
	}	
}

int main()
{
	int m,n;
	while(cin>>m>>n)
	{
		cout<<"m="<<m<<",n="<<n<<endl;
		int* p = new int[m*n];
		memset(p,0,sizeof(int)*m*n);
		
		int step=1;
		*p=1;
		method=0;
		jump(p,step,0,0, m, n);
		cout<<"method="<<method<<endl;
		
		delete[] p;
	}
	return 0;
}