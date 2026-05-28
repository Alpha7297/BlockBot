#ifndef LEVEL_LEVEL_H
#define LEVEL_LEVEL_H

#include "../core/Runtime.h"

#include <memory>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace level{

inline constexpr int CellEmpty = 0;
inline constexpr int CellWall = 1;
inline constexpr int CellTrap = 2;

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
    const core::RuntimeState* runtime = nullptr;
};

struct TestResult{
    bool passed = false;
    std::string message;
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
    virtual int testIntervalMs() const;
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
    int testIntervalMs() const override;
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

    void enableBlock(const std::string& blockName);
    void disableBlock(const std::string& blockName);
    bool blockEnabled(const std::string& blockName) const;
    const std::set<std::string>& enabledBlocks() const;

    void setTest(std::unique_ptr<LevelTest> newTest);
    void setReachPositionGoal(int x,int y);
    void setDataOutputCases(const std::vector<DataTestCase>& cases);
    int testCaseCount() const;
    int testIntervalMs() const;
    void prepareTestCase(int index,core::RuntimeState& runtime) const;
    TestResult runTestCase(int index,const TestContext& context) const;

private:
    std::vector<std::vector<int>> mapData;
    RobotState start;
    std::set<std::string> blocks;
    std::unique_ptr<LevelTest> test;
};

LevelConfig& activeLevel();
void resetActiveLevel(int mapWidth,int mapHeight);
void setActiveMapCell(int x,int y,int type);
void setActiveRobotStart(int x,int y,int direction);
void enableActiveBlock(const std::string& blockName);
void disableActiveBlock(const std::string& blockName);
void setActiveReachPositionGoal(int x,int y);
void setActiveDataOutputCases(const std::vector<DataTestCase>& cases);
int activeTestCaseCount();
int activeTestIntervalMs();
void prepareActiveTestCase(int index,core::RuntimeState& runtime);
TestResult testActiveLevelCase(int index,const TestContext& context);
int activeLevelNumber();
LevelType activeLevelType();
LevelType defaultLevelTypeForNumber(int levelNumber);
void configureActiveLevel(int levelNumber,LevelType type);
void configureSandBoxLevel();
}

#endif
