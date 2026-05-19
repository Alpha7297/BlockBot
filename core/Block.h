#ifndef BLOCK_H
#define BLOCK_H

#include "Value.h"
#include "Runtime.h"

#include <string>

namespace core{

class Block{
public:
    Block* before;
    Block* next;
    int type;

    Block();
    Block(Block* bef,Block* nex);
    Block(Block* bef,Block* nex,int _type);
    virtual ~Block()=default;
    virtual void execute();
    virtual void execute(RobotActions& actions);
};

class StartBlock:public Block{
public:
    StartBlock(Block* bef,Block* nex,int _type);
    virtual void execute();
    virtual void execute(RobotActions& actions);
};

class UnaryBlock:public Block{
public:
    FloatValue* val;

    UnaryBlock(Block* fore,Block* nex,int _type,FloatValue* _val);
    virtual void execute();
    virtual void execute(RobotActions& actions);
};

class BinaryBlock:public Block{
public:
    std::string name;
    FloatValue* val;

    BinaryBlock(Block* bef,Block* nex,int _type,std::string _name,FloatValue* _val);
    virtual void execute();
    virtual void execute(RobotActions& actions);
};

class LogicBlock:public Block{
public:
    BoolValue* op;
    Block* inside;

    LogicBlock(Block* a,Block* b,int _type,BoolValue* _op,Block* c);
    virtual void execute();
    virtual void execute(RobotActions& actions);
};

}

#endif
