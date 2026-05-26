#include "ui/App.h"
#include "level/Level.h"
#include "tale.h"
#include <vector>
using std::vector;

int main(int argc,char* argv[]){
    //return tale::runEditor(argc,argv);
    return ui::runApp(argc,argv);
}
