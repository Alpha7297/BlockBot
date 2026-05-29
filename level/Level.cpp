#include "Level.h"
#include "LevelConstants.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace level{

namespace{

LevelConfig currentLevel;
int currentLevelNumber=MinLevelNumber;
LevelType currentLevelType=LevelType::Map;


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
    return 40;
}

DataTestCase makeLevel6Case(const std::vector<double>& time,const std::vector<double>& pos,int n){
    DataTestCase testCase;
    testCase.inputVariables["n"]=n;
    testCase.inputLists["time"]=time;
    testCase.inputLists["pos"]=pos;

    std::vector<std::pair<double,double>> valid;
    int count=std::min(n,static_cast<int>(std::min(time.size(),pos.size())));
    for(int i=0;i<count;i++){
        if(time[i]<1||time[i]>1000||pos[i]<1||pos[i]>=410){
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
        testCase.expectedLists["target"].push_back(item.second);
    }
    return testCase;
}

DataTestCase makeGeneratedLevel6Case(int seed,int n){
    std::vector<double> time;
    std::vector<double> pos;
    time.reserve(n);
    pos.reserve(n);
    std::srand(seed);
    for(int i=0;i<n;i++){
        int rawTime=std::rand()%1108-49;
        int rawPos=std::rand()%470-29;
        if(i%17==0){
            rawTime=-1;
        }
        else if(i%29==0){
            rawTime=1001;
        }
        if(i%13==0){
            rawPos=0;
        }
        else if(i%31==0){
            rawPos=410;
        }
        time.push_back(rawTime);
        pos.push_back(rawPos);
    }
    return makeLevel6Case(time,pos,n);
}

DataTestCase makeLevel7Case(int seed){
    DataTestCase c;
    std::srand(seed);
    int origin=100000+std::rand()%900000;
    c.expectedVariables["明文"]=origin;
    std::vector<int> numberlist;
    int value=origin;
    while(value!=0){
        int a=value%10;
        numberlist.push_back((a+3)%10);
        value=value/10;
    }
    int ans=0;
    int n=numberlist.size();
    for(int i=0;i<n;i++){
        ans=ans*10+numberlist[(i-2+n)%n];
    }
    c.inputVariables["密码"]=ans;
    return c;
}

DataTestCase makeLevel8Case(int seed){
    DataTestCase c;
    using std::rand;
    std::srand(seed);
    int n=rand()%1000+1;
    c.inputVariables["n"]=n;
    std::vector<double> height;
    for(int i=0;i<n;i++){
        height.push_back(rand()%100);
    }
    c.inputLists["高度"]=height;
    std::vector<double> threshold;
    for(int i=0;i<n;i++){
        threshold.push_back(rand()%5);
    }
    c.inputLists["阈值"]=threshold;
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

void configureMapLevel(int levelNumber){
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
        currentLevel.setMapCell(3,10,CellEmpty);
        currentLevel.setMapCell(18,10,CellEnd);
    }
}

void configureDataOutputLevel(int levelNumber){
    constexpr int size=40;
    resetActiveLevel(size,size);
    fillBorderWalls(currentLevel,size);
    currentLevel.setRobotStart(5,5,0);
    std::vector<DataTestCase> cases;
    if(levelNumber==6){
        cases.push_back(makeLevel6Case(
            {7,3},
            {41,284},
            2
        ));
        cases.push_back(makeLevel6Case(
            {-1,123,321,598,1001,1,1000,450,40,265},
            {12,411,410,0,88,1,999,284,-1,120},
            10
        ));
        cases.push_back(makeLevel6Case(
            {1000,999,1,2,500,501,250,750},
            {410,409,1,2,250,251,100,300},
            8
        ));
        cases.push_back(makeLevel6Case(
            {4,4,2,2,3,3,1,1},
            {80,20,30,10,60,50,40,70},
            8
        ));
        cases.push_back(makeLevel6Case(
            {0,1001,-1,2000},
            {1,2,3,4},
            4
        ));
        cases.push_back(makeLevel6Case(
            {10,20,30,40,50,60},
            {-1,0,410,999,120,1},
            6
        ));
        cases.push_back(makeGeneratedLevel6Case(7,50));
        cases.push_back(makeGeneratedLevel6Case(19,120));
        cases.push_back(makeGeneratedLevel6Case(31,500));
        cases.push_back(makeGeneratedLevel6Case(43,1000));
    }
    if(levelNumber==7){
        DataTestCase c1;
        c1.expectedVariables["明文"]=923456;
        c1.inputVariables["密码"]=529876;
        cases.push_back(c1);
        for(int i=0;i<9;i++){
            cases.push_back(makeLevel7Case(i+13));
        }
    }
    if(levelNumber==8){
        DataTestCase c1;
        c1.inputVariables["n"]=5;
        c1.inputLists["高度"]={4,3,4,1,3};
        c1.inputLists["阈值"]={0,0,0,1,1};
        c1.expectedLists["危险点"]={1,3};
        cases.push_back(c1);
        for(int i=0;i<9;i++){
            cases.push_back(makeLevel8Case(i+13));
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
        return {false,"关卡信息错误，请练习游戏开发者"};
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
                  <<"\"值为"<<value<<", 正确值为"<<item.second<<".";
            return {false,stream.str()};
        }
    }
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
        for(int i=0;i<actualSize;i++){
            double value=0.0;
            context.runtime->getListValue(item.first,i,&value);
            if(std::abs(value-item.second[i])>eps){
                std::ostringstream stream;
                stream<<"测试样例"<<(index+1)<<" 失败:列表 \""<<item.first
                      <<"\"["<<i<<"] is "<<value<<", 正确值是 "<<item.second[i]<<".";
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
TestResult fresh(const TestContext& context){
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
            return {false,"测试失败：机器人被激活的尖刺击中。"};
        }
    }
    return {true,""};
}

void configureActiveLevel(int levelNumber,LevelType type){
    levelNumber=std::max(MinLevelNumber,std::min(levelNumber,TotalLevelCount));
    currentLevelNumber=levelNumber;
    currentLevelType=type;
    if(type==LevelType::DataOutput){
        configureDataOutputLevel(levelNumber);
        return;
    }
    else if(type==LevelType::Map)configureMapLevel(levelNumber);
    else configureSandBoxLevel();
}

}
