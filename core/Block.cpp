#include "Block.h"

#include <iostream>

namespace core{

Block::Block(){
    before=nullptr;
    next=nullptr;
    type=-1;
}

Block::Block(Block* bef,Block* nex){
    before=bef;
    next=nex;
    type=-1;
}

Block::Block(Block* bef,Block* nex,int _type){
    before=bef;
    next=nex;
    type=_type;
}

void Block::execute(){
    if(type==0){
        std::cout<<"turn left\n";
    }
    if(type==1){
        std::cout<<"turn right\n";
    }
    if(next!=nullptr){
        next->execute();
    }
}

void Block::execute(RobotActions& actions){
    if(type==0){
        actions.turnLeft();
    }
    if(type==1){
        actions.turnRight();
    }
    if(next!=nullptr){
        next->execute(actions);
    }
}

StartBlock::StartBlock(Block* bef,Block* nex,int _type):Block(bef,nex,_type){}

void StartBlock::execute(){
    if(next!=nullptr){
        next->execute();
    }
}

void StartBlock::execute(RobotActions& actions){
    if(next!=nullptr){
        next->execute(actions);
    }
}

UnaryBlock::UnaryBlock(Block* fore,Block* nex,int _type,FloatValue* _val):Block(fore,nex,_type){
    val=_val;
}

void UnaryBlock::execute(){
    double data=val->getval();
    if(type==0){
        std::cout<<"move "<<data<<"steps\n";
    }
    if(type==1){
        std::cout<<"wait "<<data<<"frames\n";
    }
    if(next!=nullptr){
        next->execute();
    }
}

void UnaryBlock::execute(RobotActions& actions){
    double data=val->getval();
    if(type==0){
        actions.moveForward(1.0);
    }
    if(type==1){
        actions.waitFrames(data);
    }
    if(next!=nullptr){
        next->execute(actions);
    }
}

BinaryBlock::BinaryBlock(Block* bef,Block* nex,int _type,std::string _name,FloatValue* _val):Block(bef,nex,_type){
    name=_name;
    val=_val;
}

void BinaryBlock::execute(){
    int idx=(*float_map.find(name)).second;
    float_variables[idx]=val->getval();
    if(next!=nullptr){
        next->execute();
    }
}

void BinaryBlock::execute(RobotActions& actions){
    int idx=(*float_map.find(name)).second;
    float_variables[idx]=val->getval();
    if(next!=nullptr){
        next->execute(actions);
    }
}

LogicBlock::LogicBlock(Block* a,Block* b,int _type,BoolValue* _op,Block* c):Block(a,b,_type){
    op=_op;
    inside=c;
}

void LogicBlock::execute(){
    if(type==0){
        if(op!=nullptr&&op->getval()&&inside!=nullptr){
            inside->execute();
        }
        if(next!=nullptr){
            next->execute();
        }
    }
    if(type==1){
        while(op!=nullptr&&op->getval()&&inside!=nullptr){
            inside->execute();
        }
        if(next!=nullptr){
            next->execute();
        }
    }
}

void LogicBlock::execute(RobotActions& actions){
    if(type==0){
        if(op!=nullptr&&op->getval()&&inside!=nullptr){
            inside->execute(actions);
        }
        if(next!=nullptr){
            next->execute(actions);
        }
    }
    if(type==1){
        while(op!=nullptr&&op->getval()&&inside!=nullptr){
            inside->execute(actions);
        }
        if(next!=nullptr){
            next->execute(actions);
        }
    }
}

}
