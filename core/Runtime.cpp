#include "Runtime.h"

#include <cmath>

namespace core{

bool RuntimeState::createVariable(const std::string& name){
    return floatVariables.emplace(name,0.0).second;
}

bool RuntimeState::hasVariable(const std::string& name) const{
    return floatVariables.find(name)!=floatVariables.end();
}

bool RuntimeState::getVariable(const std::string& name,double* value) const{
    auto it=floatVariables.find(name);
    if(it==floatVariables.end()){
        return false;
    }
    if(value!=nullptr){
        *value=it->second;
    }
    return true;
}

bool RuntimeState::setVariable(const std::string& name,double value){
    auto it=floatVariables.find(name);
    if(it==floatVariables.end()){
        return false;
    }
    it->second=value;
    return true;
}

const std::map<std::string,double>& RuntimeState::variables() const{
    return floatVariables;
}

bool RuntimeState::createList(const std::string& name){
    return floatLists.emplace(name,std::vector<double>()).second;
}

bool RuntimeState::hasList(const std::string& name) const{
    return floatLists.find(name)!=floatLists.end();
}

bool RuntimeState::getListValue(const std::string& name,int index,double* value) const{
    auto it=floatLists.find(name);
    if(it==floatLists.end()||index<0||index>=static_cast<int>(it->second.size())){
        return false;
    }
    if(value!=nullptr){
        *value=it->second[index];
    }
    return true;
}

bool RuntimeState::pushList(const std::string& name,double value){
    auto it=floatLists.find(name);
    if(it==floatLists.end()){
        return false;
    }
    it->second.push_back(value);
    return true;
}

bool RuntimeState::setListValue(const std::string& name,int index,double value){
    auto it=floatLists.find(name);
    if(it==floatLists.end()||index<0||index>=static_cast<int>(it->second.size())){
        return false;
    }
    it->second[index]=value;
    return true;
}

bool RuntimeState::clearList(const std::string& name){
    auto it=floatLists.find(name);
    if(it==floatLists.end()){
        return false;
    }
    it->second.clear();
    return true;
}

int RuntimeState::listSize(const std::string& name) const{
    auto it=floatLists.find(name);
    if(it==floatLists.end()){
        return -1;
    }
    return static_cast<int>(it->second.size());
}

const std::map<std::string,std::vector<double>>& RuntimeState::lists() const{
    return floatLists;
}

void RuntimeState::resetAll(){
    for(auto& item:floatVariables){
        item.second=0.0;
    }
    for(auto& item:floatLists){
        item.second.clear();
    }
}

void RobotActions::setVariable(const std::string&,double){
}

void RobotActions::pushList(const std::string&,double){
}

void RobotActions::setListValue(const std::string&,double,double){
}

void RobotActions::clearList(const std::string&){
}

void BlockExecutor::reset(Node firstBlock){
    current=firstBlock;
    frames.clear();
    waiting=nullptr;
    waitRemaining=0.0;
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
    if(block.type!=4&&waiting!=nullptr){
        waiting=nullptr;
        waitRemaining=0.0;
    }
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
    if(block.type==4){
        if(waiting!=current){
            waiting=current;
            waitRemaining=std::floor(std::max(0.0,block.value));
        }
        if(waitRemaining>1.0){
            actions.waitFrames(1.0);
            waitRemaining-=1.0;
            return true;
        }
        actions.waitFrames(waitRemaining);
        waiting=nullptr;
        waitRemaining=0.0;
        current=block.next;
        return true;
    }
    if(block.type==7){
        actions.setVariable(block.variableName,block.value);
        current=block.next;
        return true;
    }
    if(block.type==8){
        actions.pushList(block.listName,block.value);
        current=block.next;
        return true;
    }
    if(block.type==9){
        actions.setListValue(block.listName,block.indexValue,block.value);
        current=block.next;
        return true;
    }
    if(block.type==10){
        actions.clearList(block.listName);
        current=block.next;
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
