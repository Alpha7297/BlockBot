#include "MazeGenerator.h"

#include <algorithm>
#include <queue>
#include <random>
#include <vector>

namespace level{
namespace{

class Maze{
private:
    struct Node{
        int r,c,p;
        Node(int r_=0,int c_=0,int p_=0):r(r_),c(c_),p(p_){}
        bool operator < (const Node b)const{
            return rand()%9-4+p>b.p;
        }
    };

    int n,m;
    int oldn,oldm;
    std::vector<std::vector<int>> map;
    int del[4][2];
    std::mt19937 rng;
    std::priority_queue<Node> q;

public:
    Maze(int n_,int m_):oldn(n_),oldm(m_),rng(std::random_device{}()){
        del[0][0]=-1;del[0][1]=0;
        del[1][0]=0;del[1][1]=-1;
        del[2][0]=1;del[2][1]=0;
        del[3][0]=0;del[3][1]=1;
        n=oldn/2*2+1;
        m=oldm/2*2+1;
        map.assign(n+2,std::vector<int>(m+2,1));
    }

    void dfs(){
        int r=q.top().r,c=q.top().c,nowp=q.top().p;
        q.pop();
        int ord[4]={0,1,2,3};
        std::shuffle(ord,ord+4,rng);
        for(int i,j=0;j<4;j++){
            i=ord[j];
            int nr=r+del[i][0]*2,nc=c+del[i][1]*2;
            if(!(nr>0&&nr<=n&&nc>0&&nc<=m))continue;
            if(r>oldn&&nr>oldn)continue;
            if(c>oldm&&nc>oldm)continue;
            if(nr>oldn&&rng()%29>14)continue;
            if(nc>oldm&&rng()%29>14)continue;
            if(map[nr][nc]==1){
                map[r+del[i][0]][c+del[i][1]]=0;
                map[nr][nc]=0;
                q.push(Node(nr,nc,nowp+rng()%7-1));
            }
        }
    }

    void generate(){
        map[1][1]=0;
        while(!q.empty())q.pop();
        q.push(Node(1,1,0));
        while(!q.empty())dfs();
        map[oldn][oldm]=0;
        if(map[oldn-1][oldm]==0&&map[oldn][oldm-1]==0){
            if(rng()%2==1)map[oldn-1][oldm]=1;
            else map[oldn][oldm-1]=1;
        }
        if(map[oldn-1][oldm]==1&&map[oldn][oldm-1]==1){
            if(rng()%2==1)map[oldn-1][oldm]=0;
            else map[oldn][oldm-1]=0;
        }
    }

    std::vector<std::vector<int>> toMap() const{
        std::vector<std::vector<int>> result(oldn+2,std::vector<int>(oldm+2,1));
        for(int i=1;i<=oldn;i++){
            for(int j=1;j<=oldm;j++){
                result[i][j]=map[i][j]==1?1:0;
            }
        }
        return result;
    }
};

}

std::vector<std::vector<int>> generateMazeMap(){
    Maze maze(38,38);
    maze.generate();
    return maze.toMap();
}

}
