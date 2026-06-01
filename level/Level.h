#ifndef LEVEL_LEVEL_H
#define LEVEL_LEVEL_H

#include "../core/Runtime.h"

#include <memory>
#include <map>
#include <string>
#include <vector>

namespace level{

inline constexpr int CellEmpty=0;
inline constexpr int CellWall=1;
inline constexpr int CellEnd=3;
inline constexpr int CellStart=4;
inline constexpr int CellSpikeUp=5;
inline constexpr int CellSpikeDown=6;
inline constexpr int CellLightG=7;
inline constexpr int CellLightR=8;
inline constexpr int CellLightY=9;
inline constexpr int CellScope1=10;
inline constexpr int CellScope2=11;
inline constexpr int CellScope3=12;
inline constexpr int CellScope4=13;
inline constexpr int CellBeam1=14;
inline constexpr int CellLiquid=15;
inline constexpr int CellPlate=16;
inline constexpr int CellValve=17;
inline constexpr int CellAntenna=18;
inline constexpr int CellAntenna2=19;
inline constexpr int CellLightOff=20;
inline constexpr int CellBeam2=21;
inline constexpr int CellLiquid2=22;
inline constexpr int CellValve2=23;
enum class LevelType{
    Map,
    DataOutput,
    SandBox
};

struct RobotState{
    int x = 5;
    int y = 5;
    int direction = 0;
};

struct TestContext{
    std::vector<std::vector<int>> map;
    RobotState robot;
    core::RuntimeState* runtime = nullptr;
    int steps = 0;
    int time = 0;
    bool waited = false;
};

struct TestResult{
    bool passed = false;
    std::string message;
};

struct FreshResult{
    bool reachedGoal = false;
    bool trapped = false;
    std::string trapMessage;
};

struct DataTestCase{
    std::map<std::string,double> inputVariables;
    std::map<std::string,std::vector<double>> inputLists;
    std::map<std::string,double> expectedVariables;
    std::map<std::string,std::vector<double>> expectedLists;
};

class LevelTest{
public:
    virtual ~LevelTest() = default;
    virtual int caseCount() const;
    virtual void prepareCase(int index,core::RuntimeState& runtime) const;
    virtual TestResult checkCase(int index,const TestContext& context) const = 0;
};

class ReachPositionTest:public LevelTest{
public:
    ReachPositionTest(int targetX,int targetY);
    TestResult checkCase(int index,const TestContext& context) const override;

private:
    int x;
    int y;
};

class DataOutputTest:public LevelTest{
public:
    void addCase(const DataTestCase& testCase);
    int caseCount() const override;
    void prepareCase(int index,core::RuntimeState& runtime) const override;
    TestResult checkCase(int index,const TestContext& context) const override;

private:
    std::vector<DataTestCase> cases;
};

class LevelConfig{
public:
    LevelConfig();

    void reset();
    void resizeMap(int width,int height,int defaultCell = 0);
    void setMapCell(int x,int y,int type);
    int mapCell(int x,int y) const;
    const std::vector<std::vector<int>>& map() const;

    void setRobotStart(int x,int y,int direction);
    RobotState robotStart() const;

    void setTest(std::unique_ptr<LevelTest> newTest);
    void setReachPositionGoal(int x,int y);
    void setDataOutputCases(const std::vector<DataTestCase>& cases);
    void setInputCases(const std::vector<DataTestCase>& cases);
    int testCaseCount() const;
    void prepareTestCase(int index,core::RuntimeState& runtime) const;
    TestResult runTestCase(int index,const TestContext& context) const;

private:
    std::vector<std::vector<int>> mapData;
    RobotState start;
    std::vector<DataTestCase> inputCases;
    std::unique_ptr<LevelTest> test;
};

LevelConfig& activeLevel();
void resetActiveLevel(int mapWidth,int mapHeight);
void setActiveMapCell(int x,int y,int type);
void setActiveRobotStart(int x,int y,int direction);
void setActiveReachPositionGoal(int x,int y);
void setActiveDataOutputCases(const std::vector<DataTestCase>& cases);
void setActiveInputCases(const std::vector<DataTestCase>& cases);
int activeTestCaseCount();
void prepareActiveTestCase(int index,core::RuntimeState& runtime);
TestResult testActiveLevelCase(int index,const TestContext& context);
int testContextSteps(const TestContext& context);
int testContextTime(const TestContext& context);
int activeLevelNumber();
LevelType activeLevelType();
LevelType defaultLevelTypeForNumber(int levelNumber);
FreshResult fresh(const TestContext& context);
void configureActiveLevel(int levelNumber,LevelType type,bool regenerateDynamicMaps = true);
void configureSandBoxLevel();
}

#endif
