#include <iostream>
#include <vector>
#include <random>
#include <queue>
#include <algorithm>
class Maze
{
private:
	struct Node
	{
		int r,c,p;
		Node(int r_=0,int c_=0,int p_=0):r(r_),c(c_),p(p_){}
		bool operator < (const Node b)const
		{
			return rand()%9-4+p>b.p;
		}
	};
	int n,m;
	int oldn,oldm;
	std::vector<std::vector<int> >Map;
	int cnt=0;
	int del[4][2];
	std::mt19937 rng;
	std::priority_queue<Node>q;
public:
	Maze(int n_,int m_):oldn(n_),oldm(m_),rng()//如果要改变种子，改rng的构造函数
	{
		del[0][0]=-1;del[0][1]=0;
		del[1][0]=0;del[1][1]=-1;
		del[2][0]=1;del[2][1]=0;
		del[3][0]=0;del[3][1]=1;
		n=oldn/2*2+1;
		m=oldm/2*2+1;
		Map.resize(n+2);
		Map.assign(n+2, std::vector<int>(m+2, 1));
	}
	void dfs()
	{
		int r=q.top().r,c=q.top().c,nowp=q.top().p;
		q.pop();
		int ord[4]={0,1,2,3};
		std::shuffle(ord,ord+4,rng);
		for(int i,j=0;j<4;j++)
		{
			i=ord[j];
			int nr=r+del[i][0]*2,nc=c+del[i][1]*2;
			if(!(nr>0&&nr<=n&&nc>0&&nc<=m))continue;
			if(r>oldn&&nr>oldn)continue;
			if(c>oldm&&nc>oldm)continue;
			if(nr>oldn&&rng()%29>14)continue;
			if(nc>oldm&&rng()%29>14)continue;
			if(Map[nr][nc]==1)
			{
				Map[r+del[i][0]][c+del[i][1]]=0;
				Map[nr][nc]=0;
				q.push(Node(nr,nc,nowp+rng()%7-1));
			}
		}
	}
	void generate()
	{
		Map[1][1]=0;
		while(!q.empty())q.pop();
		q.push(Node(1,1,0));
		while(!q.empty())dfs();
		Map[oldn][oldm]=0;
		if(Map[oldn-1][oldm]==0&&Map[oldn][oldm-1]==0)
		{
			if(rng()%2==1)Map[oldn-1][oldm]=1;
			else Map[oldn][oldm-1]=1;
		}
		if(Map[oldn-1][oldm]==1&&Map[oldn][oldm-1]==1)
		{
			if(rng()%2==1)Map[oldn-1][oldm]=0;
			else Map[oldn][oldm-1]=0;
		}
	}
	void print()
	{
		for(int j=0;j<=oldm+1;j++)printf("1 ");
		printf("\n");
		for(int i=1;i<=oldn;i++)
		{
			printf("1");
			for(int j=1;j<=oldm;j++)
			{
				if(Map[i][j]==1)printf("1 ");
				else if(Map[i][j]==0) printf("0 ");
				else printf("%d ",Map[i][j]);
			}
			printf("1\n");
		}
		for(int j=0;j<=oldm+1;j++)printf("1 ");
	}
	void writeAnsDFS(int r,int c)
	{
		if(r==oldn&&c==oldm)
		{
			Map[r][c]=2;
			return;
		}
		Map[r][c]=1;
		printf("\n%d %d",r,c);
		for(int i=0;i<4;i++)
		{
			int nr=r+del[i][0],nc=c+del[i][1];
			printf("\n	%d %d",nr,nc);
			if(nr>0&&nr<=oldn&&nc>0&&nc<=oldm&&Map[nr][nc]==0)
			{
				writeAnsDFS(nr,nc);
				if(Map[nr][nc]==2)
				{
					Map[r][c]=2;
					return;
				}
			}
		}
		Map[r][c]=0;
	}
	void writeAns()
	{
		writeAnsDFS(1,1);
		printf("\n\n");
		print();
	}
};
int main()
{
	Maze maze(38,38);//[1,oldn],[1,oldm]是有效迷宫范围，在外围是围墙
	maze.generate();
	maze.print();
	maze.writeAns();
	return 0;
}
