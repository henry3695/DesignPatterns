#include <iostream>
using namespace std;

/*
00	01	02	03	04
10	11	12	13	14
20	21	22	23	24
30	31	32	33	34
40	41	42	43	44
求  
每次从（0，0）出发
每个点马只走1次,能全部走完，路线方案的个数
马先走1点钟方向，顺时针搜索路线

cmd /k pushd "$(CURRENT_DIRECTORY)" && g++ -o "$(NAME_PART).exe" "$(FULL_CURRENT_PATH)" && "$(NAME_PART)".exe & PAUSE & EXIT
*/

int dx[]={1,2, 2, 1,-1,-2,-2,-1};
int dy[]={2,1,-1,-2,-2,-1, 1, 2};
//从1开始走，记录步数
int sign[5][5]={0};
int method=0;

void print_arry(int m,int n)
{
	for(int x=0;x<m;x++)
	{
		for(int y=0;y<m;y++)
		{
			cout<<sign[x][y]<<"\t";
		}
		cout<<endl;
	}
}
bool check(int step,int x,int y)
{
	if( (x>=0 && x<5) && (y>=0 && y<5)  )
	{
		if(sign[x][y]==0)
		{
			return true;
		}
	}
	return false;
}

void jump(int step,int x,int y)
{
	if(step==25)
	{
		//刚好走完棋盘
		method++;
		if(method==1)
		{
			print_arry(5,5);
		}
		return;
	}
	
	//八个方向扫描下一步
	for(int i=0;i<8;i++)
	{
		int x1=x+dx[i];
		int y1=y+dy[i];
		if(check(step+1,x1,y1))
		{
			sign[x1][y1]=step+1;
			jump(step+1,x1,y1);
			//马儿会一直往前面跳，直到没有前面的路，如果 step=25 说明完成了一次有效寻路，把路线+1
			//不然就会标记(x1,y1)为0，进行另外一个方向的试探，直到8个方向都试过,根据递归栈的调用顺序一步步往回搜索，覆盖了所有可能
			sign[x1][y1]=0;
		}
	}	
}
int main()
{
	cout<<"start"<<endl;
	
	sign[0][0]=1;
	jump(1,0,0);
	
	cout<<"method="<<method<<endl;
}


