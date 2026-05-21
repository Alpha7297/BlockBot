#include "Level.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace level{

namespace{

LevelConfig currentLevel;


int normalizedDirection(int direction){
    return ((direction%4)+4)%4;
}

}

int LevelTest::caseCount() const{
    return 1;
}

int LevelTest::testIntervalMs() const{
    return 40;
}

void LevelTest::prepareCase(int,core::RuntimeState&) const{
}

ReachPositionTest::ReachPositionTest(int targetX,int targetY):
    x(targetX),y(targetY){
}

TestResult ReachPositionTest::checkCase(int,const TestContext& context) const{
    if(context.robot.x==x&&context.robot.y==y){
        return {true,"Test passed: robot reached the target position."};
    }
    std::ostringstream stream;
    stream<<"Test failed: robot is at ("<<context.robot.x<<", "<<context.robot.y
          <<"), expected ("<<x<<", "<<y<<").";
    return {false,stream.str()};
}

void DataOutputTest::addCase(const DataTestCase& testCase){
    cases.push_back(testCase);
}

int DataOutputTest::caseCount() const{
    return std::max(1,static_cast<int>(cases.size()));
}

int DataOutputTest::testIntervalMs() const{
    return 1;
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
        return {false,"Test failed: data test case index is out of range."};
    }
    if(context.runtime==nullptr){
        return {false,"Test failed: runtime state is missing."};
    }
    const double eps=1e-8;
    const DataTestCase& testCase=cases[index];
    for(const auto& item:testCase.expectedVariables){
        double value=0.0;
        if(!context.runtime->getVariable(item.first,&value)){
            return {false,"Test failed: expected output variable \""+item.first+"\" does not exist."};
        }
        if(std::abs(value-item.second)>eps){
            std::ostringstream stream;
            stream<<"Test case "<<(index+1)<<" failed: variable \""<<item.first
                  <<"\" is "<<value<<", expected "<<item.second<<".";
            return {false,stream.str()};
        }
    }
    for(const auto& item:testCase.expectedLists){
        int actualSize=context.runtime->listSize(item.first);
        if(actualSize<0){
            return {false,"Test failed: expected output list \""+item.first+"\" does not exist."};
        }
        if(actualSize!=static_cast<int>(item.second.size())){
            std::ostringstream stream;
            stream<<"Test case "<<(index+1)<<" failed: list \""<<item.first
                  <<"\" size is "<<actualSize<<", expected "<<item.second.size()<<".";
            return {false,stream.str()};
        }
        for(int i=0;i<actualSize;i++){
            double value=0.0;
            context.runtime->getListValue(item.first,i,&value);
            if(std::abs(value-item.second[i])>eps){
                std::ostringstream stream;
                stream<<"Test case "<<(index+1)<<" failed: list \""<<item.first
                      <<"\"["<<i<<"] is "<<value<<", expected "<<item.second[i]<<".";
                return {false,stream.str()};
            }
        }
    }
    std::ostringstream stream;
    stream<<"Test case "<<(index+1)<<" passed.";
    return {true,stream.str()};
}

LevelConfig::LevelConfig(){
    reset();
}

void LevelConfig::reset(){
    mapData.clear();
    start=RobotState();
    blocks.clear();
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

void LevelConfig::enableBlock(const std::string& blockName){
    blocks.insert(blockName);
}

void LevelConfig::disableBlock(const std::string& blockName){
    blocks.erase(blockName);
}

bool LevelConfig::blockEnabled(const std::string& blockName) const{
    return blocks.empty()||blocks.find(blockName)!=blocks.end();
}

const std::set<std::string>& LevelConfig::enabledBlocks() const{
    return blocks;
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

int LevelConfig::testCaseCount() const{
    return test==nullptr?1:test->caseCount();
}

int LevelConfig::testIntervalMs() const{
    return test==nullptr?40:test->testIntervalMs();
}

void LevelConfig::prepareTestCase(int index,core::RuntimeState& runtime) const{
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

void enableActiveBlock(const std::string& blockName){
    currentLevel.enableBlock(blockName);
}

void disableActiveBlock(const std::string& blockName){
    currentLevel.disableBlock(blockName);
}

void setActiveReachPositionGoal(int x,int y){
    currentLevel.setReachPositionGoal(x,y);
}

void setActiveDataOutputCases(const std::vector<DataTestCase>& cases){
    currentLevel.setDataOutputCases(cases);
}

int activeTestCaseCount(){
    return currentLevel.testCaseCount();
}

int activeTestIntervalMs(){
    return currentLevel.testIntervalMs();
}

void prepareActiveTestCase(int index,core::RuntimeState& runtime){
    currentLevel.prepareTestCase(index,runtime);
}

TestResult testActiveLevelCase(int index,const TestContext& context){
    return currentLevel.runTestCase(index,context);
}

}
