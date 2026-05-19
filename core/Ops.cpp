#include "Ops.h"

#include <cmath>

namespace core{

const double EPS=1e-8;

UnaryBoolOp::UnaryBoolOp(BoolValue* _num,int _type){
    type=_type;
    num=_num;
}

bool UnaryBoolOp::getval(){
    bool val=num->getval();
    if(type==0){
        return !val;
    }
    return false;
}

BinaryBoolOp::BinaryBoolOp(FloatValue* aa,FloatValue* bb,int _type){
    type=_type;
    a=aa;
    b=bb;
}

bool BinaryBoolOp::getval(){
    double da=a->getval();
    double db=b->getval();
    if(type==0){
        return std::abs(da-db)<EPS;
    }
    if(type==1){
        return da<db-EPS;
    }
    if(type==2){
        return da>db+EPS;
    }
    if(type==3){
        return !(std::abs(da-db)<EPS);
    }
    return false;
}

UnaryFloatOp::UnaryFloatOp(){
    data=nullptr;
    type=-1;
}

UnaryFloatOp::UnaryFloatOp(FloatValue* slab,int _type){
    data=slab;
    type=_type;
}

double UnaryFloatOp::getval(){
    double val=data->getval();
    if(type==0){
        return std::sin(val);
    }
    if(type==1){
        return std::cos(val);
    }
    if(type==2){
        return std::tan(val);
    }
    if(type==3){
        return std::asin(val);
    }
    if(type==4){
        return std::acos(val);
    }
    if(type==5){
        return std::atan(val);
    }
    if(type==6){
        return std::log(val);
    }
    if(type==7){
        return std::log10(val);
    }
    if(type==8){
        return std::floor(val);
    }
    if(type==9){
        return std::abs(val);
    }
    if(type==10){
        return std::abs(val)<EPS?1.0:0.0;
    }
    return 0.0;
}

BinaryFloatOp::BinaryFloatOp(){
    a=nullptr;
    b=nullptr;
    type=-1;
}

BinaryFloatOp::BinaryFloatOp(FloatValue* s1,FloatValue* s2,int _type){
    a=s1;
    b=s2;
    type=_type;
}

double BinaryFloatOp::getval(){
    double v1=a->getval();
    double v2=b->getval();
    if(type==0){
        return v1+v2;
    }
    if(type==1){
        return v1-v2;
    }
    if(type==2){
        return v1*v2;
    }
    if(type==3){
        return v1/v2;
    }
    if(type==4){
        return std::pow(v1,v2);
    }
    if(type==5){
        return std::atan2(v2,v1);
    }
    if(type==6){
        return v1>v2?v1:v2;
    }
    if(type==7){
        return v1<v2?v1:v2;
    }
    if(type==8){
        return std::abs(v1-v2)<EPS?1.0:0.0;
    }
    if(type==9){
        return std::abs(v1-v2)>=EPS?1.0:0.0;
    }
    if(type==10){
        return v1<v2-EPS?1.0:0.0;
    }
    if(type==11){
        return v1>v2+EPS?1.0:0.0;
    }
    if(type==12){
        return (std::abs(v1)>=EPS&&std::abs(v2)>=EPS)?1.0:0.0;
    }
    if(type==13){
        return (std::abs(v1)>=EPS||std::abs(v2)>=EPS)?1.0:0.0;
    }
    return 0.0;
}

}
