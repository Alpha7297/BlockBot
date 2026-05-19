#ifndef CORE_RUNTIME_H
#define CORE_RUNTIME_H

namespace core{

class RobotActions{
public:
    virtual ~RobotActions()=default;
    virtual void turnLeft()=0;
    virtual void turnRight()=0;
    virtual void moveForward(double steps)=0;
    virtual void waitFrames(double frames)=0;
};

class BlockExecutor{
public:
    void executeOne(int blockType,double floatValue,RobotActions& actions) const;
};

}

#endif
