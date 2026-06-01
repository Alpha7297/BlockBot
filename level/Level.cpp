#include "Level.h"
#include "LevelConstants.h"
#include "MazeGenerator.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <random>
#include <sstream>
#include <QDebug>

namespace level{

namespace{

LevelConfig currentLevel;
int currentLevelNumber=MinLevelNumber;
LevelType currentLevelType=LevelType::Map;

int randomInt(std::mt19937& rng,int minValue,int maxValue){
    std::uniform_int_distribution<int> dist(minValue,maxValue);
    return dist(rng);
}

int normalizedDirection(int direction){
    return ((direction%4)+4)%4;
}

void fillBorderWalls(LevelConfig& level,int size){
    for(int x=0;x<size;x++){
        level.setMapCell(x,0,CellWall);
        level.setMapCell(x,size-1,CellWall);
    }
    for(int y=0;y<size;y++){
        level.setMapCell(0,y,CellWall);
        level.setMapCell(size-1,y,CellWall);
    }
}

int mapSizeForLevel(int levelNumber){
    if(levelNumber==1){
        return 10;
    }
    if(levelNumber==2||levelNumber==3||levelNumber==4){
        return 20;
    }
    if(levelNumber==9){
        return 10;
    }
    return 40;
}

std::vector<std::vector<int>> readMapFile(const std::string& fileName,int width,int height){
    const std::vector<std::string> candidates={
        fileName,
        "../"+fileName,
        "../../"+fileName
    };
    for(const std::string& path:candidates){
        std::ifstream file(path);
        if(!file.is_open()){
            continue;
        }
        std::vector<std::vector<int>> rows;
        std::string line;
        while(std::getline(file,line)){
            std::istringstream stream(line);
            std::vector<int> row;
            int value=0;
            while(stream>>value){
                row.push_back(value);
            }
            if(!row.empty()){
                rows.push_back(row);
            }
        }
        if(static_cast<int>(rows.size())!=height){
            qDebug()<<"Map file has wrong row count:"<<path.c_str();
            continue;
        }
        bool valid=true;
        for(const auto& row:rows){
            if(static_cast<int>(row.size())!=width){
                valid=false;
                break;
            }
        }
        if(!valid){
            qDebug()<<"Map file has wrong column count:"<<path.c_str();
            continue;
        }
        return rows;
    }
    qDebug()<<"Failed to load map file:"<<fileName.c_str();
    return {};
}

int level3CellFromRaw(int raw){
    if(raw==1){
        return CellWall;
    }
    if(raw==0){
        return CellSpikeUp;
    }
    if(raw==2){
        return CellLightR;
    }
    if(raw==3){
        return CellEnd;
    }
    return CellEmpty;
}

void setLevel3LightCell(int gridIndex,int cellType){
    if(gridIndex<0||gridIndex>=9){
        return;
    }
    int gridx=gridIndex%3;
    int gridy=gridIndex/3;
    int baseX=1+gridx*6;
    int baseY=1+gridy*6;
    if(gridy==1){
        baseX=5+(2-gridx)*6;
    }
    currentLevel.setMapCell(baseX,baseY,cellType);
    currentLevel.setMapCell(baseX+1,baseY,cellType);
    currentLevel.setMapCell(baseX,baseY+1,cellType);
    currentLevel.setMapCell(baseX+1,baseY+1,cellType);
}

int level3GridIndexForCell(int gridx,int gridy){
    if(gridy==1){
        return (2-gridx)+gridy*3;
    }
    return gridx+gridy*3;
}

int level4CellFromRaw(int raw){
    if(raw==1){
        return CellWall;
    }
    if(raw==3){
        return CellScope1;
    }
    if(raw==4){
        return CellEnd;
    }
    return CellEmpty;
}

int level9CellFromRaw(int raw){
    if(raw==1){
        return CellWall;
    }
    if(raw==3){
        return CellPlate;
    }
    if(raw==4){
        return CellLiquid;
    }
    if(raw==5){
        return CellAntenna;
    }
    if(raw==6){
        return CellValve;
    }
    return CellEmpty;
}

bool isScopeCell(int cell){
    return cell==CellScope1||cell==CellScope2||cell==CellScope3||cell==CellScope4;
}

double runtimeVariableValue(core::RuntimeState* runtime,const std::string& name,double fallback=0.0){
    if(runtime==nullptr){
        return fallback;
    }
    double value=fallback;
    runtime->getVariable(name,&value);
    return value;
}

void forceRuntimeVariable(core::RuntimeState* runtime,const std::string& name,double value){
    if(runtime!=nullptr){
        runtime->forceSetVariable(name,value,true);
    }
}

bool findAdjacentDevice(int x,int y,int* deviceX,int* deviceY,int* deviceCell){
    const int dx[4]={1,-1,0,0};
    const int dy[4]={0,0,1,-1};
    for(int i=0;i<4;i++){
        int nx=x+dx[i];
        int ny=y+dy[i];
        int cell=currentLevel.mapCell(nx,ny);
        if(cell==CellLiquid||cell==CellLiquid2||
           cell==CellAntenna||cell==CellAntenna2||
           cell==CellValve||cell==CellValve2){
            if(deviceX!=nullptr)*deviceX=nx;
            if(deviceY!=nullptr)*deviceY=ny;
            if(deviceCell!=nullptr)*deviceCell=cell;
            return true;
        }
    }
    return false;
}

void resetLevel9DeviceCells(){
    for(int x=0;x<10;x++){
        for(int y=0;y<10;y++){
            int cell=currentLevel.mapCell(x,y);
            if(cell==CellLiquid2){
                currentLevel.setMapCell(x,y,CellLiquid);
            }
            else if(cell==CellAntenna2){
                currentLevel.setMapCell(x,y,CellAntenna);
            }
            else if(cell==CellValve2){
                currentLevel.setMapCell(x,y,CellValve);
            }
        }
    }
}

DataTestCase makeLevel6Case(const std::vector<double>& time,const std::vector<double>& pos,int n){
    DataTestCase testCase;
    testCase.inputVariables["n"]=n;
    testCase.inputLists["时间"]=time;
    testCase.inputLists["位置"]=pos;

    std::vector<std::pair<double,double>> valid;
    int count=std::min(n,static_cast<int>(std::min(time.size(),pos.size())));
    for(int i=0;i<count;i++){
        if(time[i]<0||pos[i]<0){
            continue;
        }
        valid.push_back({time[i],pos[i]});
    }
    std::sort(valid.begin(),valid.end(),[](const auto& lhs,const auto& rhs){
        if(lhs.first!=rhs.first){
            return lhs.first<rhs.first;
        }
        return lhs.second<rhs.second;
    });
    for(const auto& item:valid){
        testCase.expectedLists["有效结果"].push_back(item.second);
    }
    return testCase;
}

DataTestCase makeGeneratedLevel6Case(int seed,int n){
    std::vector<double> time;
    std::vector<double> pos;
    time.reserve(n);
    pos.reserve(n);
    std::mt19937 rng(seed);
    for(int i=0;i<n;i++){
        if(randomInt(rng,1,10)<2){
            if(randomInt(rng,0,1)==0){
                time.push_back(-1);
                pos.push_back(randomInt(rng,0,400));
            }
            else{
                time.push_back(randomInt(rng,0,1000));
                pos.push_back(-1);
            }
        }
        int rawTime=randomInt(rng,1,1000);
        int rawPos=randomInt(rng,1,400);
        time.push_back(rawTime);
        pos.push_back(rawPos);
    }
    return makeLevel6Case(time,pos,n);
}

DataTestCase makeLevel7Case(int seed){
    DataTestCase c;
    std::mt19937 rng(seed);
    int ans=0;
    int origin=0;
    do{
        origin=randomInt(rng,100000,999999);
        std::vector<int> numberlist;
        int value=origin;
        while(value!=0){
            int a=value%10;
            numberlist.push_back((a+3)%10);
            value=value/10;
        }
        ans=0;
        int n=numberlist.size();
        for(int i=0;i<n;i++){
            ans=ans*10+numberlist[(i-2+n)%n];
        }
    }while(ans<100000);
    c.expectedVariables["明文"]=origin;
    c.inputVariables["密码"]=ans;
    return c;
}

DataTestCase makeLevel8Case(int seed){
    DataTestCase c;
    std::mt19937 rng(seed);
    int n=randomInt(rng,1,100);
    c.inputVariables["n"]=n;
    std::vector<double> height;
    for(int i=0;i<n;i++){
        height.push_back(randomInt(rng,0,99));
    }
    c.inputLists["高度"]=height;
    std::vector<double> threshold;
    for(int i=0;i<n;i++){
        threshold.push_back(randomInt(rng,0,10));
    }
    c.inputLists["压力上限"]=threshold;
    int left=0,right=n-1;
    int leftMax=0,rightMax=0;
    std::vector<double> ans;
    while(left<=right){
        if(leftMax<=rightMax){
            int risk=std::max(0,leftMax-(int)height[left]);
            if(risk>threshold[left]){
                ans.push_back(left+1);
            }
            leftMax=std::max(leftMax,(int)height[left]);
            left++;
        }
        else{
            int risk=std::max(0,rightMax-(int)height[right]);
            if(risk>threshold[right]){
                ans.push_back(right+1);
            }
            rightMax=std::max(rightMax,(int)height[right]);
            right--;
        }
    }
    std::sort(ans.begin(),ans.end());
    c.expectedLists["危险点"]=ans;
    return c;
}

void configureMapLevel(int levelNumber,bool regenerateDynamicMaps){
    int size=mapSizeForLevel(levelNumber);
    resetActiveLevel(size,size);
    fillBorderWalls(currentLevel,size);
    if(levelNumber==1){
        currentLevel.setReachPositionGoal(7,5);
        currentLevel.setRobotStart(3,3,3);
        currentLevel.setMapCell(7,5,level::CellEnd);
    }
    if(levelNumber==2){
        currentLevel.setRobotStart(3,10,3);
        currentLevel.setReachPositionGoal(18,10);
        for(int i=1;i<19;i++){
            for(int j=1;j<19;j++){
                currentLevel.setMapCell(i,j,CellSpikeUp);
            }
        }
        for(int j=1;j<19;j++){
            currentLevel.setMapCell(9,j,CellSpikeDown);
        }
        currentLevel.setMapCell(3,10,CellEmpty);
        currentLevel.setMapCell(18,10,CellEnd);
    }
    if(levelNumber==3){
        currentLevel.setRobotStart(1,1,3);
        currentLevel.setReachPositionGoal(18,18);
        const std::vector<std::vector<int>> rawMap=readMapFile("level/level3.txt",20,20);
        if(!rawMap.empty()){
            for(int y=0;y<20;y++){
                for(int x=0;x<20;x++){
                    currentLevel.setMapCell(x,y,level3CellFromRaw(rawMap[y][x]));
                }
            }
        }
        currentLevel.setMapCell(18,18,CellEnd);
        DataTestCase c;
        c.inputLists["light"]={0,0,0,0,0,0,0,0,0};
        currentLevel.setInputCases({c});
    }
    if(levelNumber==4){
        const std::vector<std::vector<int>> rawMap=readMapFile("level/level4.txt",20,20);
        if(!rawMap.empty()){
            for(int y=0;y<20;y++){
                for(int x=0;x<20;x++){
                    int raw=rawMap[y][x];
                    currentLevel.setMapCell(x,y,level4CellFromRaw(raw));
                    if(raw==2){
                        currentLevel.setRobotStart(x,y,2);
                    }
                    else if(raw==4){
                        currentLevel.setReachPositionGoal(x,y);
                    }
                }
            }
            for(int x=0;x<20;x++){
                for(int y=0;y<20;y++){
                    int scopeCell=currentLevel.mapCell(x,y);
                    int dx=0,dy=0;
                    if(scopeCell==CellScope1) dy=-1;
                    else if(scopeCell==CellScope2) dy=1;
                    else if(scopeCell==CellScope3) dx=-1;
                    else if(scopeCell==CellScope4) dx=1;
                    else continue;
                    int beamCell=dx==0?CellBeam2:CellBeam1;
                    int beamX=x+dx;
                    int beamY=y+dy;
                    while(beamX>=0&&beamY>=0&&beamX<20&&beamY<20){
                        int cell=currentLevel.mapCell(beamX,beamY);
                        if(cell==CellWall||isScopeCell(cell)) break;
                        if(cell!=CellEnd){
                            currentLevel.setMapCell(beamX,beamY,beamCell);
                        }
                        beamX+=dx;
                        beamY+=dy;
                    }
                }
            }
        }
    }
    if(levelNumber==5){
        if(regenerateDynamicMaps){
            runMazeGenerator();
        }
        const std::vector<std::vector<int>> rawMap=readMapFile("level/level5.txt",40,40);
        if(!rawMap.empty()){
            for(int i=0;i<40;i++){
                for(int j=0;j<40;j++){
                    if(rawMap[i][j]==1){
                        currentLevel.setMapCell(i,j,CellWall);
                    }
                    else{
                        currentLevel.setMapCell(i,j,CellEmpty);
                    }
                }
            }
        }
        currentLevel.setMapCell(1,1,CellStart);
        currentLevel.setMapCell(38,38,CellEnd);
        currentLevel.setReachPositionGoal(38,38);
        currentLevel.setRobotStart(1,1,3);
    }
    if(levelNumber==9){
        const std::vector<std::vector<int>> rawMap=readMapFile("level/level9.txt",10,10);
        if(!rawMap.empty()){
            for(int y=0;y<10;y++){
                for(int x=0;x<10;x++){
                    int raw=rawMap[y][x];
                    currentLevel.setMapCell(x,y,level9CellFromRaw(raw));
                    if(raw==2){
                        currentLevel.setRobotStart(x,y,3);
                    }
                }
            }
        }
        DataTestCase c;
        c.inputVariables["天线稳定度"]=0;
        c.inputVariables["阀门稳定度"]=0;
        c.inputVariables["冷却值"]=30;
        currentLevel.setInputCases({c});
    }
}

void configureDataOutputLevel(int levelNumber){
    constexpr int size=40;
    resetActiveLevel(size,size);
    fillBorderWalls(currentLevel,size);
    currentLevel.setRobotStart(5,5,3);
    std::vector<DataTestCase> cases;
    std::mt19937 rng(13);
    if(levelNumber==6){
        cases.push_back(makeLevel6Case(
            {7,3,-1,12},
            {41,284,10,-1},
            4
        ));
        for(int i=0;i<9;i++){
            cases.push_back(makeGeneratedLevel6Case(
                randomInt(rng,0,std::numeric_limits<int>::max()),
                randomInt(rng,1,20)
            ));
        }
    }
    if(levelNumber==7){
        DataTestCase c1;
        c1.expectedVariables["明文"]=923456;
        c1.inputVariables["密码"]=529876;
        cases.push_back(c1);
        for(int i=0;i<9;i++){
            cases.push_back(makeLevel7Case(randomInt(rng,0,std::numeric_limits<int>::max())));
        }
    }
    if(levelNumber==8){
        DataTestCase c1;
        c1.inputVariables["n"]=5;
        c1.inputLists["高度"]={4,3,4,1,3};
        c1.inputLists["压力上限"]={0,0,0,1,1};
        c1.expectedLists["危险点"]={2,4};
        cases.push_back(c1);
        for(int i=0;i<9;i++){
            cases.push_back(makeLevel8Case(randomInt(rng,0,std::numeric_limits<int>::max())));
        }
    }
    currentLevel.setDataOutputCases(cases);
}

}

int LevelTest::caseCount() const{
    return 1;
}

void LevelTest::prepareCase(int,core::RuntimeState&) const{
}

ReachPositionTest::ReachPositionTest(int targetX,int targetY):
    x(targetX),y(targetY){
}

TestResult ReachPositionTest::checkCase(int,const TestContext& context) const{
    if(context.robot.x==x&&context.robot.y==y){
        return {true,"测试通过，机器人到达指定位置"};
    }
    std::ostringstream stream;
    stream<<"测试失败，机器人在 ("<<context.robot.x<<", "<<context.robot.y
          <<"), 而最终地点在 ("<<x<<", "<<y<<").";
    return {false,stream.str()};
}

void DataOutputTest::addCase(const DataTestCase& testCase){
    cases.push_back(testCase);
}

int DataOutputTest::caseCount() const{
    return std::max(1,static_cast<int>(cases.size()));
}

void DataOutputTest::prepareCase(int index,core::RuntimeState& runtime) const{
    if(index<0||index>=static_cast<int>(cases.size())){
        return;
    }
    const DataTestCase& testCase=cases[index];
    for(const auto& item:testCase.inputVariables){
        runtime.forceSetVariable(item.first,item.second,true);
    }
    for(const auto& item:testCase.inputLists){
        runtime.forceSetList(item.first,item.second,true);
    }
    for(const auto& item:testCase.expectedVariables){
        if(!runtime.hasVariable(item.first)){
            runtime.forceSetVariable(item.first,0.0,false);
        }
    }
    for(const auto& item:testCase.expectedLists){
        if(!runtime.hasList(item.first)){
            runtime.forceSetList(item.first,std::vector<double>(),false);
        }
    }
}

TestResult DataOutputTest::checkCase(int index,const TestContext& context) const{
    if(index<0||index>=static_cast<int>(cases.size())){
        return {false,"测试失败，索引超过上限"};
    }
    if(context.runtime==nullptr){
        return {false,"关卡信息错误，请联系游戏开发者"};
    }
    const double eps=1e-8;
    const DataTestCase& testCase=cases[index];
    for(const auto& item:testCase.expectedVariables){
        double value=0.0;
        if(!context.runtime->getVariable(item.first,&value)){
            return {false,"测试失败，答案 \""+item.first+"\" 不存在。"};
        }
        if(std::abs(value-item.second)>eps){
            std::ostringstream stream;
            stream<<"测试样例"<<(index+1)<<"失败，变量 \""<<item.first
                  <<"值为"<<value<<", 正确值为"<<item.second<<".";
            return {false,stream.str()};
        }
    }
    int levelNumber=activeLevelNumber();
    for(const auto& item:testCase.expectedLists){
        int actualSize=context.runtime->listSize(item.first);
        if(actualSize<0){
            return {false,"测试失败，需要的列表 \""+item.first+"\"不存在"};
        }
        if(actualSize!=static_cast<int>(item.second.size())){
            std::ostringstream stream;
            stream<<"测试样例"<<(index+1)<<"失败:列表 \""<<item.first
                  <<"\" 大小是 "<<actualSize<<", 正确答案大小是"<<item.second.size()<<".";
            return {false,stream.str()};
        }
        if(levelNumber==8){
            std::set<double> ans;
            std::set<double> playerAns;
            for(int i=0;i<actualSize;i++){
                double value=0.0;
                context.runtime->getListValue(item.first,i,&value);
                ans.insert(item.second[i]);
                playerAns.insert(value);
            }
            for(double data:ans){
                if(playerAns.find(data)==playerAns.end()){
                    std::ostringstream stream;
                    stream<<"测试样例"<<(index+1)<<" 失败:列表 \""<<item.first
                      <<"\"中"<<data<<"不存在";
                    return {false,stream.str()};
                }
            }
            std::ostringstream stream;
            stream<<"测试样例"<<(index+1)<<"通过";
            return {true,stream.str()};
        }
        for(int i=0;i<actualSize;i++){
            double value=0.0;
            context.runtime->getListValue(item.first,i,&value);
            if(std::abs(value-item.second[i])>eps){
                std::ostringstream stream;
                stream<<"测试样例"<<(index+1)<<" 失败:列表 \""<<item.first
                      <<"\""<<i+1<<"项是 "<<value<<", 正确值是 "<<item.second[i]<<".";
                return {false,stream.str()};
            }
        }
    }
    std::ostringstream stream;
    stream<<"测试样例"<<(index+1)<<"通过";
    return {true,stream.str()};
}

LevelConfig::LevelConfig(){
    reset();
}

void LevelConfig::reset(){
    mapData.clear();
    start=RobotState();
    inputCases.clear();
    test.reset();
}

void LevelConfig::resizeMap(int width,int height,int defaultCell){
    if(width<0){
        width=0;
    }
    if(height<0){
        height=0;
    }
    mapData.assign(width,std::vector<int>(height,defaultCell));
}

void LevelConfig::setMapCell(int x,int y,int type){
    if(x<0||y<0||x>=static_cast<int>(mapData.size())){
        return;
    }
    if(mapData.empty()||y>=static_cast<int>(mapData[x].size())){
        return;
    }
    mapData[x][y]=type;
}

int LevelConfig::mapCell(int x,int y) const{
    if(x<0||y<0||x>=static_cast<int>(mapData.size())){
        return 0;
    }
    if(mapData.empty()||y>=static_cast<int>(mapData[x].size())){
        return 0;
    }
    return mapData[x][y];
}

const std::vector<std::vector<int>>& LevelConfig::map() const{
    return mapData;
}

void LevelConfig::setRobotStart(int x,int y,int direction){
    start.x=x;
    start.y=y;
    start.direction=normalizedDirection(direction);
}

RobotState LevelConfig::robotStart() const{
    return start;
}

void LevelConfig::setTest(std::unique_ptr<LevelTest> newTest){
    test=std::move(newTest);
}

void LevelConfig::setReachPositionGoal(int x,int y){
    test=std::make_unique<ReachPositionTest>(x,y);
}

void LevelConfig::setDataOutputCases(const std::vector<DataTestCase>& cases){
    auto dataTest=std::make_unique<DataOutputTest>();
    for(const DataTestCase& testCase:cases){
        dataTest->addCase(testCase);
    }
    test=std::move(dataTest);
}

void LevelConfig::setInputCases(const std::vector<DataTestCase>& cases){
    inputCases=cases;
}

int LevelConfig::testCaseCount() const{
    int count=test==nullptr?1:test->caseCount();
    if(!inputCases.empty()){
        count=std::max(count,static_cast<int>(inputCases.size()));
    }
    return count;
}

void LevelConfig::prepareTestCase(int index,core::RuntimeState& runtime) const{
    if(!inputCases.empty()){
        int inputIndex=std::max(0,std::min(index,static_cast<int>(inputCases.size())-1));
        const DataTestCase& inputCase=inputCases[inputIndex];
        for(const auto& item:inputCase.inputVariables){
            runtime.forceSetVariable(item.first,item.second,true);
        }
        for(const auto& item:inputCase.inputLists){
            runtime.forceSetList(item.first,item.second,true);
        }
    }
    if(test!=nullptr){
        test->prepareCase(index,runtime);
    }
}

TestResult LevelConfig::runTestCase(int index,const TestContext& context) const{
    if(currentLevelNumber==9){
        double door=runtimeVariableValue(context.runtime,"阀门稳定度",0.0);
        double antenna=runtimeVariableValue(context.runtime,"天线稳定度",0.0);
        bool doorReady=door>=4.0;
        bool antennaReady=antenna>=5.0;
        bool overtime=context.time>60;
        if(!doorReady&&!antennaReady){
            return {false,"测试失败：阀门稳定度和天线稳定度都没有达标。"};
        }
        if(overtime){
            if(doorReady&&!antennaReady){
                return {true,"孤独逃离：超过 60 帧，闸门已经打开，但日志没有发送。"};
            }
            if(!doorReady&&antennaReady){
                return {true,"日志成功：超过 60 帧，日志已经发送，但闸门没有打开。"};
            }
            return {false,"测试失败：超过 60 帧，未在时间窗口内完成第九关目标。"};
        }
        if(doorReady&&!antennaReady){
            return {true,"孤独逃离：闸门已经打开，但日志没有发送。"};
        }
        if(!doorReady&&antennaReady){
            return {true,"日志成功：天线已经修复，但闸门没有打开。"};
        }
        return {true,"胜利：闸门打开，日志也成功发送。"};
    }
    if(test==nullptr){
        return {true,"No level test is configured."};
    }
    return test->checkCase(index,context);
}

LevelConfig& activeLevel(){
    return currentLevel;
}

void resetActiveLevel(int mapWidth,int mapHeight){
    currentLevel.reset();
    currentLevel.resizeMap(mapWidth,mapHeight);
}

void setActiveMapCell(int x,int y,int type){
    currentLevel.setMapCell(x,y,type);
}

void setActiveRobotStart(int x,int y,int direction){
    currentLevel.setRobotStart(x,y,direction);
}

void setActiveReachPositionGoal(int x,int y){
    currentLevel.setReachPositionGoal(x,y);
}

void setActiveDataOutputCases(const std::vector<DataTestCase>& cases){
    currentLevel.setDataOutputCases(cases);
}

void setActiveInputCases(const std::vector<DataTestCase>& cases){
    currentLevel.setInputCases(cases);
}

int activeTestCaseCount(){
    return currentLevel.testCaseCount();
}

void prepareActiveTestCase(int index,core::RuntimeState& runtime){
    currentLevel.prepareTestCase(index,runtime);
}

TestResult testActiveLevelCase(int index,const TestContext& context){
    return currentLevel.runTestCase(index,context);
}

int testContextSteps(const TestContext& context){
    return context.steps;
}

int testContextTime(const TestContext& context){
    return context.time;
}

int activeLevelNumber(){
    return currentLevelNumber;
}

LevelType activeLevelType(){
    return currentLevelType;
}

LevelType defaultLevelTypeForNumber(int levelNumber){
    levelNumber=std::max(MinLevelNumber,std::min(levelNumber,TotalLevelCount));
    if(levelNumber>=6&&levelNumber<=8)return LevelType::DataOutput;
    else if(levelNumber>=0)return LevelType::Map;
    else return LevelType::SandBox;
}
void configureSandBoxLevel()
{

    constexpr int size=40;
    resetActiveLevel(size,size);
    fillBorderWalls(currentLevel,size);
    currentLevel.setReachPositionGoal(-1,-1);
}
FreshResult fresh(const TestContext& context){
    if(currentLevelNumber==2){
        int time=context.time;
        for(int i=1;i<9;i++){
            for(int j=1;j<19;j++){
                int cell=currentLevel.mapCell(i,j);
                if(cell!=CellSpikeUp&&cell!=CellSpikeDown){
                    continue;
                }
                currentLevel.setMapCell(i,j,time%20<15?CellSpikeUp:CellSpikeDown);
            }
        }
        for(int j=1;j<19;j++){
            currentLevel.setMapCell(9,j,CellSpikeDown);
        }
        for(int i=10;i<19;i++){
            for(int j=1;j<19;j++){
                int cell=currentLevel.mapCell(i,j);
                if(cell!=CellSpikeUp&&cell!=CellSpikeDown){
                    continue;
                }
                currentLevel.setMapCell(i,j,time%20<10?CellSpikeUp:CellSpikeDown);
            }
        }
        int robotCell=currentLevel.mapCell(context.robot.x,context.robot.y);
        if(robotCell==CellSpikeUp){
            return {false,true,"测试失败：机器人被激活的尖刺击中。"};
        }
    }
    if(currentLevelNumber==3){
        int xx=context.robot.x,yy=context.robot.y;
        std::vector<double> value;
        for(int i=0;i<9;i++){
            double v;
            context.runtime->getListValue("light",i,&v);
            value.push_back(v);
        }
        int gridx=(xx-1)/6;
        int gridy=(yy-1)/6;
        int basex=gridx*6+1;
        int basey=gridy*6+1;
        int currGrid=level3GridIndexForCell(gridx,gridy);
        int time=context.time;
        const int unlockTimes[9]={10,20,30,60,90,120,150,180,210};
        if(currGrid>=0&&currGrid<9&&value[currGrid]==0){
            if(time>unlockTimes[currGrid]){
                value[currGrid]=1;
                context.runtime->forceSetList("light",value,true);
                setLevel3LightCell(currGrid,CellLightG);
                for(int i=0;i<6;i++){
                    for(int j=0;j<6;j++){
                        if(currentLevel.mapCell(i+basex,j+basey)==CellSpikeUp){
                            currentLevel.setMapCell(i+basex,j+basey,CellSpikeDown);
                        }
                    }
                }
            }
        }
    }
    if(currentLevelNumber==4){
        std::vector<std::pair<int,int>> scopes;
        for(int x=0;x<20;x++){
            for(int y=0;y<20;y++){
                int cell=currentLevel.mapCell(x,y);
                if(cell==CellBeam1||cell==CellBeam2){
                    currentLevel.setMapCell(x,y,CellEmpty);
                }
                if(cell==CellScope1){
                    currentLevel.setMapCell(x,y,CellScope4);
                    scopes.push_back({x,y});
                }
                else if(cell==CellScope4){
                    currentLevel.setMapCell(x,y,CellScope2);
                    scopes.push_back({x,y});
                }
                else if(cell==CellScope2){
                    currentLevel.setMapCell(x,y,CellScope3);
                    scopes.push_back({x,y});
                }
                else if(cell==CellScope3){
                    currentLevel.setMapCell(x,y,CellScope1);
                    scopes.push_back({x,y});
                }
            }
        }
        bool robotHit=false;
        for(const auto& scope:scopes){
            int scopeCell=currentLevel.mapCell(scope.first,scope.second);
            int dx=0,dy=0;
            if(scopeCell==CellScope1) dy=-1;
            else if(scopeCell==CellScope2) dy=1;
            else if(scopeCell==CellScope3) dx=-1;
            else if(scopeCell==CellScope4) dx=1;
            int beamCell=dx==0?CellBeam2:CellBeam1;
            int x=scope.first+dx;
            int y=scope.second+dy;
            while(x>=0&&y>=0&&x<20&&y<20){
                int cell=currentLevel.mapCell(x,y);
                if(cell==CellWall||isScopeCell(cell)){
                    break;
                }
                if(context.robot.x==x&&context.robot.y==y){
                    robotHit=true;
                }
                if(cell!=CellEnd){
                    currentLevel.setMapCell(x,y,beamCell);
                }
                x+=dx;
                y+=dy;
            }
        }
        if(robotHit){
            return {false,true,"测试失败：机器人被旋转光束击中。"};
        }
    }
    if(currentLevelNumber==9){
        resetLevel9DeviceCells();
        double cooling=runtimeVariableValue(context.runtime,"冷却值",30.0)-1.0;
        double door=runtimeVariableValue(context.runtime,"阀门稳定度",0.0);
        double antenna=runtimeVariableValue(context.runtime,"天线稳定度",0.0);

        int deviceX=-1;
        int deviceY=-1;
        int deviceCell=CellEmpty;
        bool onPlate=currentLevel.mapCell(context.robot.x,context.robot.y)==CellPlate;
        bool hasDevice=onPlate&&findAdjacentDevice(
            context.robot.x,context.robot.y,&deviceX,&deviceY,&deviceCell);
        if(hasDevice){
            if(deviceCell==CellValve){
                currentLevel.setMapCell(deviceX,deviceY,CellValve2);
            }
            else if(deviceCell==CellLiquid){
                currentLevel.setMapCell(deviceX,deviceY,CellLiquid2);
            }
            else if(deviceCell==CellAntenna){
                currentLevel.setMapCell(deviceX,deviceY,CellAntenna2);
            }
        }
        if(context.waited&&hasDevice){
            if(deviceCell==CellValve){
                door+=1.0;
            }
            else if(deviceCell==CellLiquid){
                cooling+=5.0;
            }
            else if(deviceCell==CellAntenna){
                antenna+=1.0;
            }
        }
        forceRuntimeVariable(context.runtime,"冷却值",cooling);
        forceRuntimeVariable(context.runtime,"阀门稳定度",door);
        forceRuntimeVariable(context.runtime,"天线稳定度",antenna);
        if(cooling<0.0){
            return {false,true,"测试失败：冷却值低于 0。"};
        }
        if(context.time>60||(door>=4.0&&antenna>=5.0)){
            return {true,false,""};
        }
    }
    int robotCell=currentLevel.mapCell(context.robot.x,context.robot.y);
    if(robotCell==CellSpikeUp){
        return {false,true,"测试失败：机器人被激活的尖刺击中。"};
    }
    if(robotCell==CellBeam1||robotCell==CellBeam2){
        return {false,true,"测试失败：机器人被旋转光束击中。"};
    }
    if(robotCell==CellEnd){
        return {true,false,""};
    }
    return {false,false,""};
}

void configureActiveLevel(int levelNumber,LevelType type,bool regenerateDynamicMaps){
    levelNumber=std::max(MinLevelNumber,std::min(levelNumber,TotalLevelCount));
    currentLevelNumber=levelNumber;
    currentLevelType=type;
    if(type==LevelType::DataOutput){
        configureDataOutputLevel(levelNumber);
        return;
    }
    else if(type==LevelType::Map)configureMapLevel(levelNumber,regenerateDynamicMaps);
    else configureSandBoxLevel();
}

}
