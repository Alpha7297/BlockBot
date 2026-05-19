#ifndef CORE_RUNTIME_H
#define CORE_RUNTIME_H

#include <functional>
#include <vector>

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
    using Node = void*;

    struct BlockSnapshot{
        int type = -1;
        double value = 0.0;
        Node next = nullptr;
        Node inside = nullptr;
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
    };

    Node current = nullptr;
    std::vector<Frame> frames;
};

}

#endif
