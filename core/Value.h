#ifndef VALUE_H
#define VALUE_H

#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace core{

class FloatValue{
public:
    double num;

    FloatValue();
    FloatValue(double _num);
    virtual ~FloatValue()=default;
    virtual double getval();

    friend std::ostream& operator<<(std::ostream& os,FloatValue& other);
};

class BoolValue{
public:
    bool num;

    BoolValue();
    BoolValue(int _num);
    virtual ~BoolValue()=default;
    virtual bool getval();
};

extern std::map<std::string,int> float_map;
extern std::vector<FloatValue> float_variables;
extern std::map<std::string,int> list_map;
extern std::vector<std::vector<FloatValue>> list_variables;

}

#endif
