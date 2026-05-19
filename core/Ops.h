#ifndef OPS_H
#define OPS_H

#include "Value.h"

namespace core{

extern const double EPS;

class UnaryBoolOp:public BoolValue{
public:
    BoolValue* num;
    int type;

    UnaryBoolOp(BoolValue* _num,int _type);
    virtual bool getval();
};

class BinaryBoolOp:public BoolValue{
public:
    FloatValue* a;
    FloatValue* b;
    int type;

    BinaryBoolOp(FloatValue* aa,FloatValue* bb,int _type);
    virtual bool getval();
};

class UnaryFloatOp:public FloatValue{
public:
    FloatValue* data;
    int type;

    UnaryFloatOp();
    UnaryFloatOp(FloatValue* slab,int _type);
    virtual double getval();
};

class BinaryFloatOp:public FloatValue{
public:
    FloatValue* a;
    FloatValue* b;
    int type;

    BinaryFloatOp();
    BinaryFloatOp(FloatValue* s1,FloatValue* s2,int _type);
    virtual double getval();
};

}

#endif
