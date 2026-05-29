#include "Runtime.h"

#include <cmath>

namespace core{

bool RuntimeState::createVariable(const std::string& name,bool readOnly){
    bool created=floatVariables.emplace(name,0.0).second;
    if(created&&readOnly){
        readOnlyVariables.insert(name);
    }
    return created;
}

bool RuntimeState::createReadOnlyVariable(const std::string& name){
    return createVariable(name,true);
}

bool RuntimeState::hasVariable(const std::string& name) const{
    return floatVariables.find(name)!=floatVariables.end();
}

bool RuntimeState::variableReadOnly(const std::string& name) const{
    return readOnlyVariables.find(name)!=readOnlyVariables.end();
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
    if(it==floatVariables.end()||variableReadOnly(name)){
        return false;
    }
    it->second=value;
    return true;
}

bool RuntimeState::forceSetVariable(const std::string& name,double value,bool readOnly){
    floatVariables[name]=value;
    if(readOnly){
        readOnlyVariables.insert(name);
    }
    else{
        readOnlyVariables.erase(name);
    }
    return true;
}

const std::map<std::string,double>& RuntimeState::variables() const{
    return floatVariables;
}
const std::set<std::string>& RuntimeState::constVariables() const
{
    return readOnlyVariables;
}

bool RuntimeState::createList(const std::string& name,bool readOnly){
    bool created=floatLists.emplace(name,std::vector<double>()).second;
    if(created&&readOnly){
        readOnlyLists.insert(name);
    }
    return created;
}

bool RuntimeState::createReadOnlyList(const std::string& name){
    return createList(name,true);
}

bool RuntimeState::hasList(const std::string& name) const{
    return floatLists.find(name)!=floatLists.end();
}

bool RuntimeState::listReadOnly(const std::string& name) const{
    return readOnlyLists.find(name)!=readOnlyLists.end();
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
    if(it==floatLists.end()||listReadOnly(name)){
        return false;
    }
    it->second.push_back(value);
    return true;
}

bool RuntimeState::setListValue(const std::string& name,int index,double value){
    auto it=floatLists.find(name);
    if(it==floatLists.end()||listReadOnly(name)||
       index<0||index>=static_cast<int>(it->second.size())){
        return false;
    }
    it->second[index]=value;
    return true;
}

bool RuntimeState::removeListValue(const std::string& name,int index){
    auto it=floatLists.find(name);
    if(it==floatLists.end()||listReadOnly(name)||
       index<0||index>=static_cast<int>(it->second.size())){
        return false;
    }
    it->second.erase(it->second.begin()+index);
    return true;
}

bool RuntimeState::clearList(const std::string& name){
    auto it=floatLists.find(name);
    if(it==floatLists.end()||listReadOnly(name)){
        return false;
    }
    it->second.clear();
    return true;
}

bool RuntimeState::forceSetList(const std::string& name,const std::vector<double>& values,bool readOnly){
    floatLists[name]=values;
    if(readOnly){
        readOnlyLists.insert(name);
    }
    else{
        readOnlyLists.erase(name);
    }
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
const std::set<std::string>& RuntimeState::constLists() const
{
    return readOnlyLists;
}

void RuntimeState::resetAll(){
    for(auto& item:floatVariables){
        item.second=0.0;
    }
    for(auto& item:floatLists){
        item.second.clear();
    }
}
bool RuntimeState::removeVariable(const std::string name)
{
    auto it=floatVariables.find(name);
    if(it==floatVariables.end()||variableReadOnly(name)){
        return false;
    }
    floatVariables.erase(it);
    return true;
}
bool RuntimeState::removeList(const std::string name)
{
    auto it=floatLists.find(name);
    if(it==floatLists.end()||listReadOnly(name)){
        return false;
    }
    floatLists.erase(it);
    return true;
}
void RuntimeState::clearAll()
{
    floatVariables.clear();
    floatLists.clear();
    readOnlyVariables.clear();
    readOnlyLists.clear();
}
void RuntimeState::clearAllMutable()
{
    for (auto it = floatVariables.begin(); it != floatVariables.end(); ) {
        if (readOnlyVariables.find(it->first)==readOnlyVariables.end()) {
            it = floatVariables.erase(it); // erase 会返回指向下一个元素的有效迭代器
        }
        else ++it;
    }
    for (auto it = floatLists.begin(); it != floatLists.end(); ) {
        if (readOnlyLists.find(it->first)==readOnlyLists.end()) {
            it = floatLists.erase(it); // erase 会返回指向下一个元素的有效迭代器
        }
        else ++it;
    }
}
void RuntimeState::forceSetReadOnly(const std::set<std::string>& constVariables,const std::set<std::string>& constLists)
{
    readOnlyVariables=constVariables;
    readOnlyLists=constLists;
}
void RobotActions::setVariable(const std::string&,double){
}

void RobotActions::pushList(const std::string&,double){
}

void RobotActions::setListValue(const std::string&,double,double){
}

void RobotActions::removeListValue(const std::string&,double){
}

void RobotActions::clearList(const std::string&){
}

void RobotActions::enterCustomBlock(const std::string&,double){
}

void RobotActions::leaveCustomBlock(const std::string&){
}

void RobotActions::showMessage(const std::string&,double){
}

void BlockExecutor::reset(Node firstBlock){
    current=firstBlock;
    frames.clear();
    waiting=nullptr;
    waitRemaining=0.0;
    lastStepConsumedActionStep=false;
}

bool BlockExecutor::running() const{
    return current!=nullptr||!frames.empty();
}

BlockExecutor::Node BlockExecutor::currentNode() const{
    return current;
}

bool BlockExecutor::waitingOn(Node node) const{
    return waiting!=nullptr&&waiting==node;
}

bool BlockExecutor::didConsumeActionStep() const{
    return lastStepConsumedActionStep;
}

bool BlockExecutor::step(const BlockReader& readBlock,RobotActions& actions){
    lastStepConsumedActionStep=false;
    if(readBlock==nullptr){
        current=nullptr;
        frames.clear();
        return false;
    }
    while(current==nullptr&&!frames.empty()){
        Frame frame=frames.back();
        frames.pop_back();
        if(!frame.customName.empty()){
            actions.leaveCustomBlock(frame.customName);
        }
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
            frames.push_back({nullptr,block.next,false,""});
            current=block.inside;
        }
        else{
            current=block.next;
        }
        return true;
    }
    if(block.type==6){
        if(std::abs(block.value)>=1e-8&&block.inside!=nullptr){
            frames.push_back({current,block.next,true,""});
            current=block.inside;
        }
        else{
            current=block.next;
        }
        return true;
    }
    if(block.type==4){
        lastStepConsumedActionStep=true;
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
    if(block.type==13){
        actions.removeListValue(block.listName,block.indexValue);
        current=block.next;
        return true;
    }
    if(block.type==11){
        if(block.callTarget!=nullptr){
            actions.enterCustomBlock(block.customName,block.value);
            frames.push_back({nullptr,block.next,false,block.customName});
            current=block.callTarget;
        }
        else{
            current=block.next;
        }
        return true;
    }
    if(block.type==12){
        actions.showMessage(block.messageText,block.value);
        current=block.next;
        return true;
    }

    if(block.type==0||block.type==1||block.type==3){
        lastStepConsumedActionStep=true;
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
        actions.moveForward(1.0);
        return;
    }
    if(blockType==4){
        actions.waitFrames(std::max(0.0,floatValue));
        return;
    }
}

}
