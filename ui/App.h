#ifndef UI_APP_H
#define UI_APP_H

namespace ui{

void resetLevelConfig();
void setLevelCell(int x,int y,int type);
void setRobotStart(int x,int y,int direction);
int runApp(int argc,char* argv[]);

}

#endif
