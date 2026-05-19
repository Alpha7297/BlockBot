#include "Runtime.h"

#include <cmath>

namespace core{

void BlockExecutor::executeOne(int blockType,double floatValue,RobotActions& actions) const{
    if(blockType==0){
        actions.turnLeft();
        return;
    }
    if(blockType==1){
        actions.turnRight();
        return;
    }
    if(blockType==3){
        actions.moveForward(floatValue);
        return;
    }
    if(blockType==4){
        actions.waitFrames(std::max(0.0,floatValue));
        return;
    }
}

}
