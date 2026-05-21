#ifndef CORE_RUNTIME_H
#define CORE_RUNTIME_H

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace core{

class RuntimeState{
public:
    bool createVariable(const std::string& name);
    bool hasVariable(const std::string& name) const;
    bool getVariable(const std::string& name,double* value) const;
    bool setVariable(const std::string& name,double value);
    const std::map<std::string,double>& variables() const;

    bool createList(const std::string& name);
    bool hasList(const std::string& name) const;
    bool getListValue(const std::string& name,int index,double* value) const;
    bool pushList(const std::string& name,double value);
    bool setListValue(const std::string& name,int index,double value);
    bool clearList(const std::string& name);
    int listSize(const std::string& name) const;
    const std::map<std::string,std::vector<double>>& lists() const;

    void resetAll();

private:
    std::map<std::string,double> floatVariables;
    std::map<std::string,std::vector<double>> floatLists;
};

class RobotActions{
public:
    virtual ~RobotActions()=default;
    virtual void turnLeft()=0;
    virtual void turnRight()=0;
    virtual void moveForward(double steps)=0;
    virtual void waitFrames(double frames)=0;
    virtual void setVariable(const std::string& name,double value);
    virtual void pushList(const std::string& name,double value);
    virtual void setListValue(const std::string& name,double index,double value);
    virtual void clearList(const std::string& name);
    virtual void enterCustomBlock(const std::string& name,double value);
    virtual void leaveCustomBlock(const std::string& name);
};

class BlockExecutor{
public:
    using Node = void*;

    struct BlockSnapshot{
        int type = -1;
        double value = 0.0;
        double indexValue = 0.0;
        std::string variableName;
        std::string listName;
        std::string customName;
        Node next = nullptr;
        Node inside = nullptr;
        Node callTarget = nullptr;
    };

    using BlockReader = std::function<BlockSnapshot(Node)>;

    void reset(Node firstBlock);
    bool running() const;
    Node currentNode() const;
    bool step(const BlockReader& readBlock,RobotActions& actions);
    void executeOne(int blockType,double floatValue,RobotActions& actions) const;

private:
    struct Frame{
        Node control = nullptr;
        Node after = nullptr;
        bool repeat = false;
        std::string customName;
    };

    Node current = nullptr;
    std::vector<Frame> frames;
    Node waiting = nullptr;
    double waitRemaining = 0.0;
};

}

#endif
