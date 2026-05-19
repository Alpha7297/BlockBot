#include "core/Block.h"
#include "core/Ops.h"
#include "core/Value.h"

using namespace core;

int main(void){
    float_map["a"]=0;
    float_variables.push_back(0);

    FloatValue iters=FloatValue(10);
    FloatValue single=FloatValue(1);
    BinaryBoolOp s0=BinaryBoolOp(&float_variables[0],&iters,1);
    BinaryFloatOp add=BinaryFloatOp(&float_variables[0],&single,0);

    BinaryBlock b0=BinaryBlock(nullptr,nullptr,0,"a",&add);
    UnaryBlock b1=UnaryBlock(nullptr,&b0,0,&float_variables[0]);
    LogicBlock mywhile=LogicBlock(nullptr,nullptr,1,&s0,&b1);
    StartBlock start=StartBlock(nullptr,&mywhile,-1);
    start.execute();

    return 0;
}
