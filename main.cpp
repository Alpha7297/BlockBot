#include "level/MazeGenerator.h"
#include "tale/TaleWindow.h"
#include "ui/App.h"

int main(int argc,char* argv[]){
    level::runMazeGenerator();
    //return tale::runTale(argc,argv);
    return ui::runApp(argc,argv);
}
