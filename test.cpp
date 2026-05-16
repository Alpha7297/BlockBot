#include<bits/stdc++.h>
using std::string;
using std::map;
using std::vector;
class FloatValue;
class BoolValue;
map<string,int> float_map;
vector<FloatValue> float_variables;
const double EPS=1e-8;
class FloatValue{
public:
    double num;
    FloatValue(){};
    FloatValue(double _num){
        num=_num;
    }
    virtual double getval(){
        return num;
    }
    friend std::ostream & operator<<(std::ostream & os,FloatValue & other){
        os<<other.getval();
        return os;
    }
};
class BoolValue{
public:
    bool num;
    BoolValue(){};
    BoolValue(int _num){
        num=_num;
    }
    virtual bool getval(){
        return num;
    }
};
class Block{
public:
    Block* before;
    Block* next;
    int type;
    Block(){
        before=NULL;
        next=NULL;
        type=-1;
    }
    Block(Block * bef,Block * nex){
        before=bef;
        next=nex;
        type=-1;
    }
    Block(Block * bef,Block * nex,int _type){
        before=bef;
        next=nex;
        type=_type;
    }
    virtual void execute(){
        if(type==0){
            std::cout<<"turn left\n";
        }
        if(type==1){
            std::cout<<"turn right\n";
        }
        if(next!=NULL){
            next->execute();
        }
    };
};
class StartBlock:public Block{
public:
    StartBlock(Block* bef,Block* nex,int _type):Block(bef,nex,_type){}
    virtual void execute(){
        if(next!=NULL){
            next->execute();
        }
    };
};
class UnaryBlock:public Block{
public:
    FloatValue* val;
    UnaryBlock(Block* fore,Block* nex,int _type,FloatValue* _val):Block(fore,nex,_type){
        val=_val;
    }
    virtual void execute(){
        double data=val->getval();
        if(type==0){
            std::cout<<"move "<<data<<"steps\n";
        }
        if(type==1){
            std::cout<<"wait "<<data<<"frames\n";
        }
        if(next!=NULL){
            next->execute();
        }
    }
};
class BinaryBlock:public Block{
public:
    string name;
    FloatValue* val;
    BinaryBlock(Block* bef,Block* nex,int _type,string _name,FloatValue* _val):Block(bef,nex,_type){
        name=_name;
        val=_val;
    }
    virtual void execute(){
        int idx=(*float_map.find(name)).second;
        float_variables[idx]=val->getval();
        if(next!=NULL){
            next->execute();
        }
    }
};
class UnaryBoolOp:public BoolValue{
public:
    BoolValue* num;
    int type;
    UnaryBoolOp(BoolValue* _num,int _type){
        type=_type;
        num=_num;
    }
    virtual bool getval(){
        bool val=num->getval();
        if(type==0){
            return !val;
        }
        return 0;
    }
};
class BinaryBoolOp:public BoolValue{
public:
    FloatValue* a;
    FloatValue* b;
    int type;
    BinaryBoolOp(FloatValue* aa,FloatValue* bb,int _type){
        type=_type;
        a=aa;
        b=bb;
    }
    virtual bool getval(){
        double da=a->getval();
        double db=b->getval();
        if(type==0){
            return abs(da-db)<EPS;
        }
        if(type==1){
            return da<db+EPS;
        }
        if(type==2){
            return da>db-EPS;
        }
        if(type==3){
            return !(abs(da-db)<EPS);
        }
        return 0;
    }
};
class UnaryFloatOp:public FloatValue{
public:
    FloatValue* data;
    int type;
    UnaryFloatOp(){};
    UnaryFloatOp(FloatValue * slab,int _type){
        data=slab;
        type=_type;
    }
    virtual double getval(){
        double val=data->getval();
        if(type==0){
            return sin(val);
        }
        if(type==1){
            return cos(val);
        }
        if(type==2){
            return tan(val);
        }
        if(type==3){
            return asin(val);
        }
        if(type==4){
            return acos(val);
        }
        if(type==5){
            return atan(val);
        }
        if(type==6){
            return log(val);
        }
        if(type==7){
            return log10(val);
        }
        if(type==8){
            return floor(val);
        }
        if(type==9){
            return abs(val);
        }
        return 0.0;
    }
};
class BinaryFloatOp:public FloatValue{
public:
    FloatValue* a;
    FloatValue* b;
    int type;
    BinaryFloatOp(){};
    BinaryFloatOp(FloatValue * s1,FloatValue* s2,int _type){
        a=s1,b=s2;
        type=_type;
    }
    virtual double getval(){
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
            return pow(v1,v2);
        }
        if(type==5){
            return atan2(v2,v1);
        }
        if(type==6){
            return v1>v2?v1:v2;
        }
        if(type==7){
            return v1<v2?v1:v2;
        }
        return 0.0;
    }
};
class LogicBlock:public Block{
public:
    BoolValue* op;
    Block* inside;
    LogicBlock(Block* a,Block* b,int _type,BoolValue* _op,Block* c):Block(a,b,_type){
        op=_op;
        inside=c;
    }
    virtual void execute(){
        if(type==0){
            if(op!=NULL&&op->getval()&&inside!=NULL){
                inside->execute();
            }
            if(next!=NULL){
                next->execute();
            }
        }
        if(type==1){
            while(op!=NULL&&op->getval()&&inside!=NULL){
                inside->execute();
            }
            if(next!=NULL){
                next->execute();
            }
        }
    }
};
int main(void){
    float_map["a"]=0;
    float_variables.push_back(0);
    FloatValue iters=FloatValue(10);
    FloatValue single=FloatValue(1);
    BinaryBoolOp s0=BinaryBoolOp(&float_variables[0],&iters,1);
    BinaryFloatOp add=BinaryFloatOp(&float_variables[0],&single,0);
    
    BinaryBlock b0=BinaryBlock(NULL,NULL,0,"a",&add);
    UnaryBlock b1=UnaryBlock(NULL,&b0,0,&float_variables[0]);
    LogicBlock mywhile=LogicBlock(NULL,NULL,1,&s0,&b1);
    StartBlock start=StartBlock(NULL,&mywhile,-1);
    start.execute();
    return 0;
}
