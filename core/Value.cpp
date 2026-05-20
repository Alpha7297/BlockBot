#include "Value.h"

namespace core{

std::map<std::string,int> float_map;
std::vector<FloatValue> float_variables;
std::map<std::string,int> list_map;
std::vector<std::vector<FloatValue>> list_variables;

FloatValue::FloatValue(){
    num=0;
}

FloatValue::FloatValue(double _num){
    num=_num;
}

double FloatValue::getval(){
    return num;
}

std::ostream& operator<<(std::ostream& os,FloatValue& other){
    os<<other.getval();
    return os;
}

BoolValue::BoolValue(){
    num=false;
}

BoolValue::BoolValue(int _num){
    num=_num;
}

bool BoolValue::getval(){
    return num;
}

}
