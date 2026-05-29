#ifndef UI_APP_H
#define UI_APP_H

#include "../level/Level.h"

#include <vector>

namespace ui{

void resetLevelConfig();
void setLevelCell(int x,int y,int type);
void setRobotStart(int x,int y,int direction);
void setReachPositionGoal(int x,int y);
void setInputCases(const std::vector<level::DataTestCase>& cases);
void setDataOutputCases(const std::vector<level::DataTestCase>& cases);
int runApp(int argc,char* argv[]);

}

#endif
