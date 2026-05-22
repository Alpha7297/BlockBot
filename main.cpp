#include "ui/App.h"
#include "level/Level.h"
#include "tale.h"
#include <vector>
using std::vector;

int main(int argc,char* argv[]){
    /*这是一个移动关卡
    ui::resetLevelConfig();
    ui::setRobotStart(2,2,0);
    for(int i=0;i<40;i++){
        for(int j=0;j<40;j++){
            ui::setLevelCell(i,j,1);
        }
    }
    for(int i=2;i<=5;i++){
        ui::setLevelCell(2,i,0);
    }
    for(int i=2;i<=5;i++){
        ui::setLevelCell(i,5,0);
    }
    for(int i=5;i<=20;i++){
        ui::setLevelCell(i,5,0);
    }
    for(int i=5;i<=20;i++){
        ui::setLevelCell(20,i,0);
    }
    ui::setReachPositionGoal(20,20);
    */
   //计算fib的第n项
    ui::resetLevelConfig();
    vector<level::DataTestCase> cases;
    level::DataTestCase c1;
    c1.inputVariables["n"]=3;
    c1.expectedVariables["ans"]=2;
    level::DataTestCase c2;
    c2.inputVariables["n"]=5;
    c2.expectedVariables["ans"]=5;
    level::DataTestCase c3;
    c3.inputVariables["n"]=10;
    c3.expectedVariables["ans"]=55;
    cases.push_back(c1);
    cases.push_back(c2);
    cases.push_back(c3);
    ui::setDataOutputCases(cases);
    //return tale::runEditor(argc,argv);
    return ui::runApp(argc,argv);
}
