#include "Runtime.h"

#include <cmath>

namespace core{

void BlockExecutor::reset(Node firstBlock){
    current=firstBlock;
    frames.clear();
}

bool BlockExecutor::running() const{
    return current!=nullptr||!frames.empty();
}

BlockExecutor::Node BlockExecutor::currentNode() const{
    return current;
}

bool BlockExecutor::step(const BlockReader& readBlock,RobotActions& actions){
    if(readBlock==nullptr){
        current=nullptr;
        frames.clear();
        return false;
    }
    while(current==nullptr&&!frames.empty()){
        Frame frame=frames.back();
        frames.pop_back();
        current=frame.repeat?frame.control:frame.after;
    }
    if(current==nullptr){
        return false;
    }

    BlockSnapshot block=readBlock(current);
    if(block.type==5){
        if(std::abs(block.value)>=1e-8&&block.inside!=nullptr){
            frames.push_back({nullptr,block.next,false});
            current=block.inside;
        }
        else{
            current=block.next;
        }
        return true;
    }
    if(block.type==6){
        if(std::abs(block.value)>=1e-8&&block.inside!=nullptr){
            frames.push_back({current,block.next,true});
            current=block.inside;
        }
        else{
            current=block.next;
        }
        return true;
    }

    executeOne(block.type,block.value,actions);
    current=block.next;
    return true;
}

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
