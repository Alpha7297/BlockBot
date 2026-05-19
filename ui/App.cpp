#include <QApplication>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPen>
#include <QPoint>
#include <QGraphicsPolygonItem>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QPolygonF>
#include <QBrush>
#include <QTimer>
#include <QInputDialog>
#include <QTextDocument>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

#include "App.h"
#include "UiConstants.h"
#include "Widgets.h"
#include "../core/Runtime.h"
using std::vector;
using std::function;

QTimer* timerPtr = nullptr;

class FloatBlock;
void refreshFloatAncestors(FloatBlock* block);

class CodeBlock:public QGraphicsPolygonItem{
public:
    int len;
    int wid;
    int idx;
    QString s;
    int type;
    QGraphicsTextItem* text;
    bool dragging;
    QPointF mouseOffset;
    CodeBlock* pre;
    CodeBlock* next;
    QGraphicsPolygonItem* shadow;
    bool isbase;
    bool ismoving;
    int scrollArea;
    QPointF stagePos;
    QPointF blockOffset;
    int preidx;
    int nextidx;
    CodeBlock* preTarget;
    CodeBlock* nextTarget;
    CodeBlock(int _type,QString ss,int _idx,int base=false,
        QGraphicsItem * parent=nullptr):
            QGraphicsPolygonItem(parent){
        idx=_idx;
        type=_type;
        QPolygonF shape;
        text=new QGraphicsTextItem(ss,this);
        text->setDefaultTextColor(Qt::white);
        text->setPos(10,10);
        len=text->boundingRect().width()+30;
        wid=40;
        s=ss;
        shape<<QPoint(0,0)<<QPoint(len,0)
             <<QPoint(len,wid)<<QPoint(0,wid);
        setPolygon(shape);
        setBrush(Qt::blue);
        setPen(QPen(Qt::black,1.5));
        shadow=new QGraphicsPolygonItem();
        QPolygonF shadowShape;
        shadowShape<<QPointF(-shadowPadding,-shadowPadding)
                   <<QPointF(len+shadowPadding,-shadowPadding)
                   <<QPointF(len+shadowPadding,wid+shadowPadding)
                   <<QPointF(-shadowPadding,wid+shadowPadding);
        shadow->setPolygon(shadowShape);
        shadow->setBrush(Qt::gray);
        shadow->setPen(Qt::NoPen);
        shadow->setPos(pos());
        dragging=false;
        setAcceptedMouseButtons(Qt::LeftButton);
        mouseOffset=QPointF(0,0);
        pre=nullptr;
        next=nullptr;
        isbase=base;
        ismoving=false;
        scrollArea=scrollNone;
        stagePos=QPointF(0,0);
        blockOffset=QPointF(0,0);
        preidx=-1;
        nextidx=-1;
        preTarget=nullptr;
        nextTarget=nullptr;
    };
    virtual CodeBlock* copy(){
        CodeBlock* newBlock=new CodeBlock(type,s,idx,false);
        newBlock->setPos(pos());
        return newBlock;
    }
    virtual void refreshSize(){
    }
    QPointF calculatePos(){
        CodeBlock* curr=this;
        QPointF dist=QPointF(0,0);
        while(curr->pre!=nullptr){
            curr=curr->pre;
            dist+=QPointF(0,curr->wid);
        }
        dist+=curr->pos();
        return dist;
    }
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
};
class FloatBlock:public QGraphicsPolygonItem{
public:
    int wid,len,idx,type;
    QString s;
    double data;
    bool isbase;
    bool movable;
    int scrollArea;
    QPointF stagePos;
    bool dragging;
    bool moved;
    QPointF mouseOffset;
    FloatBlock* absorbTarget;
    QGraphicsPolygonItem* absorbShadow;
    QGraphicsTextItem* text;
    FloatBlock(int _type,int _idx,int base=false,
               QGraphicsItem * parent=nullptr):QGraphicsPolygonItem(parent){
        type=_type;
        s="";
        idx=_idx;
        data=0;
        isbase=base;
        movable=true;
        scrollArea=scrollNone;
        stagePos=QPointF(0,0);
        dragging=false;
        moved=false;
        mouseOffset=QPointF(0,0);
        absorbTarget=nullptr;
        absorbShadow=nullptr;
        text=new QGraphicsTextItem(formatData(),this);
        text->document()->setDocumentMargin(0);
        text->setDefaultTextColor(Qt::white);
        text->setPos(0,0);
        refreshSize();
        setBrush(QColor(92,102,116));
        setPen(QPen(Qt::black,1.5));
        setAcceptedMouseButtons(Qt::LeftButton);
    }
    virtual bool isOperator() const{
        return false;
    }
    virtual double getValue() const{
        return data;
    }
    QString formatData() const{
        return QString::number(data,'g',6);
    }
    virtual void setData(double value){
        data=value;
        text->setPlainText(formatData());
        refreshSize();
        if(parentItem()!=nullptr){
            FloatBlock* parentBlock=dynamic_cast<FloatBlock*>(parentItem());
            if(parentBlock!=nullptr){
                refreshFloatAncestors(parentBlock);
            }
            CodeBlock* codeParent=dynamic_cast<CodeBlock*>(parentItem());
            if(codeParent!=nullptr){
                codeParent->refreshSize();
            }
        }
    }
    virtual void refreshSize(){
        updateShape(text->boundingRect().width());
        centerText();
    }
    virtual void updateShape(qreal wantedLen){
        len=std::max(floatBlockWidth,static_cast<int>(std::ceil(wantedLen)));
        wid=floatBlockWidth;
        updatePolygon();
    }
    virtual void updatePolygon(){
        QPolygonF shape;
        shape<<QPointF(0,0)<<QPointF(len,0)
             <<QPointF(len,wid)<<QPointF(0,wid);
        setPolygon(shape);
    }
    void centerText(){
        QRectF textRect=text->boundingRect();
        text->setPos((len-textRect.width())/2,(wid-textRect.height())/2);
    }
    void setMovable(bool value){
        movable=value;
        setAcceptedMouseButtons(movable?Qt::LeftButton:Qt::NoButton);
    }
    virtual FloatBlock* copy(){
        FloatBlock* newBlock=new FloatBlock(type,idx,false);
        newBlock->setData(data);
        newBlock->setPos(pos());
        return newBlock;
    }
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
};

class UnaryOpBlock:public FloatBlock{
public:
    FloatBlock* slab;
    UnaryOpBlock(int _type,QString ss,int _idx,FloatBlock* _slab=nullptr,
                 int base=false,QGraphicsItem * parent=nullptr):
        FloatBlock(_type,_idx,base,parent){
        s=ss;
        text->setPlainText(s);
        if(_slab==nullptr){
            slab=new FloatBlock(0,0,false,this);
        }
        else{
            slab=_slab->copy();
            slab->setParentItem(this);
        }
        slab->setMovable(!base);
        slab->setPos(0,0);
        refreshSize();
        setBrush(QColor(42,105,86));
    }
    bool isOperator() const override{
        return true;
    }
    double getValue() const override{
        double val=slab->getValue();
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
        return 0.0;
    }
    void refreshSize(){
        text->setDefaultTextColor(Qt::white);
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        qreal wanted=opHorizontalPadding+textWidth+opHorizontalPadding+slab->len+opHorizontalPadding;
        updateShape(wanted);
        wid=std::max(slab->wid,floatBlockWidth)+opVerticalPadding*2;
        updatePolygon();
        text->setPos(opHorizontalPadding,(wid-textHeight)/2);
        slab->setPos(opHorizontalPadding+textWidth+opHorizontalPadding,(wid-slab->wid)/2);
        setPen(QPen(Qt::black,1.5));
    }
    FloatBlock* copy() override{
        UnaryOpBlock* newBlock=new UnaryOpBlock(type,s,idx,slab,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class BinaryOpBlock:public FloatBlock{
public:
    FloatBlock* left;
    FloatBlock* right;
    BinaryOpBlock(int _type,QString ss,int _idx,FloatBlock* _left=nullptr,
                  FloatBlock* _right=nullptr,int base=false,
                  QGraphicsItem * parent=nullptr):
        FloatBlock(_type,_idx,base,parent){
        s=ss;
        text->setPlainText(s);
        if(_left==nullptr){
            left=new FloatBlock(0,0,false,this);
        }
        else{
            left=_left->copy();
            left->setParentItem(this);
        }
        if(_right==nullptr){
            right=new FloatBlock(0,0,false,this);
        }
        else{
            right=_right->copy();
            right->setParentItem(this);
        }
        left->setMovable(!base);
        right->setMovable(!base);
        left->setPos(0,0);
        right->setPos(0,0);
        refreshSize();
        setBrush(QColor(42,105,86));
    }
    bool isOperator() const override{
        return true;
    }
    double getValue() const override{
        double a=left->getValue();
        double b=right->getValue();
        if(type==0){
            return a+b;
        }
        if(type==1){
            return a-b;
        }
        if(type==2){
            return a*b;
        }
        if(type==3){
            return a/b;
        }
        if(type==4){
            return std::pow(a,b);
        }
        if(type==5){
            return std::atan2(b,a);
        }
        if(type==6){
            return a>b?a:b;
        }
        if(type==7){
            return a<b?a:b;
        }
        return 0.0;
    }
    void refreshSize(){
        text->setDefaultTextColor(Qt::white);
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        bool prefixLayout=type>=5;
        qreal wanted=0;
        if(prefixLayout){
            wanted=opHorizontalPadding+textWidth+opHorizontalPadding+
                   left->len+opHorizontalPadding+right->len+opHorizontalPadding;
        }
        else{
            wanted=opHorizontalPadding+left->len+opHorizontalPadding+
                   textWidth+opHorizontalPadding+right->len+opHorizontalPadding;
        }
        updateShape(wanted);
        wid=std::max(std::max(left->wid,right->wid),floatBlockWidth)+opVerticalPadding*2;
        updatePolygon();
        if(prefixLayout){
            text->setPos(opHorizontalPadding,(wid-textHeight)/2);
            left->setPos(opHorizontalPadding+textWidth+opHorizontalPadding,(wid-left->wid)/2);
            right->setPos(opHorizontalPadding+textWidth+opHorizontalPadding+
                          left->len+opHorizontalPadding,(wid-right->wid)/2);
        }
        else{
            left->setPos(opHorizontalPadding,(wid-left->wid)/2);
            text->setPos(opHorizontalPadding+left->len+opHorizontalPadding,(wid-textHeight)/2);
            right->setPos(opHorizontalPadding+left->len+opHorizontalPadding+
                          textWidth+opHorizontalPadding,(wid-right->wid)/2);
        }
        setPen(QPen(Qt::black,1.5));
    }
    FloatBlock* copy() override{
        BinaryOpBlock* newBlock=new BinaryOpBlock(type,s,idx,left,right,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class FloatCodeBlock:public CodeBlock{
public:
    FloatBlock* value;
    FloatCodeBlock(int _type,QString ss,int _idx,FloatBlock* _value=nullptr,
                   int base=false,QGraphicsItem * parent=nullptr):
        CodeBlock(_type,ss,_idx,base,parent){
        if(_value==nullptr){
            value=new FloatBlock(0,0,false,this);
        }
        else{
            value=_value->copy();
            value->setParentItem(this);
        }
        value->setMovable(!base);
        refreshSize();
    }
    void refreshSize(){
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        wid=std::max(40,value->wid+10);
        len=static_cast<int>(std::ceil(20+textWidth+10+value->len+10));
        QPolygonF shape;
        shape<<QPointF(0,0)<<QPointF(len,0)
             <<QPointF(len,wid)<<QPointF(0,wid);
        setPolygon(shape);
        text->setPos(10,(wid-textHeight)/2);
        value->setPos(20+textWidth,(wid-value->wid)/2);

        QPolygonF shadowShape;
        shadowShape<<QPointF(-shadowPadding,-shadowPadding)
                   <<QPointF(len+shadowPadding,-shadowPadding)
                   <<QPointF(len+shadowPadding,wid+shadowPadding)
                   <<QPointF(-shadowPadding,wid+shadowPadding);
        shadow->setPolygon(shadowShape);
    }
    CodeBlock* copy() override{
        FloatCodeBlock* newBlock=new FloatCodeBlock(type,s,idx,value,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class StartBlock:public CodeBlock{
public:
    StartBlock(int _idx,int base=false,QGraphicsItem * parent=nullptr):
        CodeBlock(-1,"start",_idx,base,parent){
        setBrush(QColor(156,118,42));
    }
    CodeBlock* copy() override{
        StartBlock* newBlock=new StartBlock(idx,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};
template<class T>
const T max(const T a,const T b){
    return a>b?a:b;
}
template<class T>
const T min(const T a,const T b){
    return a<b?a:b;
}

class Robot:public QGraphicsPolygonItem{
public:
    int gridx;
    int gridy;
    int direction;
    Robot(QGraphicsItem * parent=nullptr):QGraphicsPolygonItem(parent){
        QPolygonF shape;
        shape<<QPointF(2,2)<<QPointF(6,4)<<QPointF(2,6);
        setPolygon(shape);
        setBrush(Qt::green);
        setPen(Qt::NoPen);
        gridx=5,gridy=5;
        direction=0;
        setPos(gridx,gridy);
        setTransformOriginPoint(4,4);
        setRotation(direction*90);
    }
    void turnLeft(){
        direction=(direction+3)%4;
        setRotation(direction*90);
    }
    void turnRight(){
        direction=(direction+1)%4;
        setRotation(direction*90);
    }
    void moveForward(int step){
        int newx=gridx,newy=gridy;
        if(direction==0){
            newx+=step;
        }
        if(direction==1){
            newy+=step;
        }
        if(direction==2){
            newx-=step;
        }
        if(direction==3){
            newy-=step;
        }
        gridx=max(0,min(newx,screensize-1));
        gridy=max(0,min(newy,screensize-1));
    }
    void SyncCell(int cellSize,QPoint offset){
        setPos(gridx*cellSize+offset.x(),gridy*cellSize+offset.y());
        setRotation(direction*90);
    }
};

class RobotActionAdapter:public core::RobotActions{
public:
    Robot* robot;

    RobotActionAdapter(Robot* _robot){
        robot=_robot;
    }

    void turnLeft() override{
        robot->turnLeft();
    }

    void turnRight() override{
        robot->turnRight();
    }

    void moveForward(double steps) override{
        robot->moveForward(static_cast<int>(std::round(steps)));
    }

    void waitFrames(double) override{
    }
};

int maxZ=10;
int mapdata[screensize][screensize];
Robot* player=nullptr;
QGraphicsRectItem* stage;
QGraphicsRectItem* toolboxBackground;
QGraphicsRectItem* workspaceBackground;
QGraphicsRectItem* squares[screensize][screensize];
Button* runButton=nullptr;
ScrollSlider* toolboxSlider=nullptr;
ScrollSlider* workspaceSlider=nullptr;
vector<CodeBlock*> codeBlocks;
vector<CodeBlock*> baseCodeBlocks;
vector<FloatBlock*> floatBlocks;
StartBlock* currentStartBlock=nullptr;
CodeBlock* runningBlock=nullptr;
qreal toolboxScrollY=0;
qreal workspaceScrollY=0;

class AppGraphicsView:public QGraphicsView{
public:
    AppGraphicsView(QGraphicsScene* scene):QGraphicsView(scene){}

protected:
    void wheelEvent(QWheelEvent* event) override{
        QPointF scenePoint=mapToScene(event->position().toPoint());
        qreal delta=-event->angleDelta().y()/1200.0;
        if(QRectF(360,20,180,500).contains(scenePoint)&&toolboxSlider!=nullptr){
            toolboxSlider->setValue(toolboxSlider->value()+delta);
            event->accept();
            return;
        }
        if(QRectF(560,20,380,500).contains(scenePoint)&&workspaceSlider!=nullptr){
            workspaceSlider->setValue(workspaceSlider->value()+delta);
            event->accept();
            return;
        }
        QGraphicsView::wheelEvent(event);
    }
};

void clearAbsorbShadow(FloatBlock* block);
FloatBlock* findAbsorbTarget(FloatBlock* moving);
void attachOperatorToTarget(FloatBlock* moving,FloatBlock* target);
FloatBlock* detachOperatorFromParent(FloatBlock* moving);
void showAbsorbShadow(FloatBlock* moving,FloatBlock* target);
void syncScrollArea(int area);
void refreshFloatAncestors(FloatBlock* block);
bool inWorkspace(CodeBlock* block){
    QRectF workspaceRect(560,20,380,500);
    return workspaceRect.contains(block->sceneBoundingRect());
}

bool inWorkspace(FloatBlock* block){
    QRectF workspaceRect(560,20,380,500);
    return workspaceRect.contains(block->sceneBoundingRect());
}

double codeBlockFloatValue(CodeBlock* block){
    FloatCodeBlock* floatBlock=dynamic_cast<FloatCodeBlock*>(block);
    if(floatBlock!=nullptr&&floatBlock->value!=nullptr){
        return floatBlock->value->getValue();
    }
    return 0.0;
}

QPointF scrollAreaOrigin(int area){
    if(area==scrollToolbox){
        return QPointF(360,20);
    }
    if(area==scrollWorkspace){
        return QPointF(560,20);
    }
    return QPointF(0,0);
}

qreal scrollAreaOffset(int area){
    if(area==scrollToolbox){
        return toolboxScrollY;
    }
    if(area==scrollWorkspace){
        return workspaceScrollY;
    }
    return 0;
}

QPointF stageToScenePos(int area,QPointF stagePos){
    return scrollAreaOrigin(area)+stagePos-QPointF(0,scrollAreaOffset(area));
}

QPointF sceneToStagePos(int area,QPointF scenePos){
    return scenePos-scrollAreaOrigin(area)+QPointF(0,scrollAreaOffset(area));
}

void setCodeBlockStagePos(CodeBlock* block,int area,QPointF pos){
    if(block==nullptr){
        return;
    }
    block->scrollArea=area;
    block->stagePos=pos;
    block->setPos(stageToScenePos(area,pos));
}

void setFloatBlockStagePos(FloatBlock* block,int area,QPointF pos){
    if(block==nullptr){
        return;
    }
    block->scrollArea=area;
    block->stagePos=pos;
    block->setPos(stageToScenePos(area,pos));
}

void rememberCodeBlockStagePos(CodeBlock* block,int area){
    if(block==nullptr){
        return;
    }
    block->scrollArea=area;
    block->stagePos=sceneToStagePos(area,block->pos());
}

void rememberFloatBlockStagePos(FloatBlock* block,int area){
    if(block==nullptr){
        return;
    }
    block->scrollArea=area;
    block->stagePos=sceneToStagePos(area,block->pos());
}

void syncScrollArea(int area){
    for(CodeBlock* block:baseCodeBlocks){
        if(block->scrollArea==area&&!block->dragging){
            block->setPos(stageToScenePos(area,block->stagePos));
        }
    }
    for(CodeBlock* block:codeBlocks){
        if(block->scrollArea==area&&!block->dragging&&!block->ismoving){
            block->setPos(stageToScenePos(area,block->stagePos));
        }
    }
    if(currentStartBlock!=nullptr&&currentStartBlock->scrollArea==area&&!currentStartBlock->dragging){
        currentStartBlock->setPos(stageToScenePos(area,currentStartBlock->stagePos));
    }
    for(FloatBlock* block:floatBlocks){
        if(block->parentItem()==nullptr&&block->scrollArea==area&&!block->dragging){
            block->setPos(stageToScenePos(area,block->stagePos));
        }
    }
}

void deleteCodeBlock(CodeBlock* block){
    if(block==currentStartBlock){
        currentStartBlock=nullptr;
    }
    codeBlocks.erase(
        std::remove(codeBlocks.begin(),codeBlocks.end(),block),
        codeBlocks.end()
    );
    if(block->scene()!=nullptr){
        block->scene()->removeItem(block);
    }
    delete block;
}

void deleteFloatBlock(FloatBlock* block){
    clearAbsorbShadow(block);
    floatBlocks.erase(
        std::remove(floatBlocks.begin(),floatBlocks.end(),block),
        floatBlocks.end()
    );
    if(block->scene()!=nullptr){
        block->scene()->removeItem(block);
    }
    delete block;
}

void eraseFloatBlockRecord(FloatBlock* block){
    floatBlocks.erase(
        std::remove(floatBlocks.begin(),floatBlocks.end(),block),
        floatBlocks.end()
    );
}

FloatBlock* rootFloatBlock(FloatBlock* block){
    FloatBlock* root=block;
    QGraphicsItem* curr=block;
    while(curr->parentItem()!=nullptr){
        curr=curr->parentItem();
        FloatBlock* parentBlock=dynamic_cast<FloatBlock*>(curr);
        if(parentBlock!=nullptr){
            root=parentBlock;
        }
    }
    return root;
}

void refreshFloatAncestors(FloatBlock* block){
    FloatBlock* curr=block;
    QGraphicsItem* parent=nullptr;
    while(curr!=nullptr){
        curr->refreshSize();
        parent=curr->parentItem();
        curr=dynamic_cast<FloatBlock*>(parent);
    }
    CodeBlock* codeParent=dynamic_cast<CodeBlock*>(parent);
    if(codeParent!=nullptr){
        codeParent->refreshSize();
    }
}

void setInsertedOperatorInteractivity(FloatBlock* block){
    if(block==nullptr){
        return;
    }
    block->setMovable(true);
    UnaryOpBlock* unary=dynamic_cast<UnaryOpBlock*>(block);
    if(unary!=nullptr){
        setInsertedOperatorInteractivity(unary->slab);
        return;
    }
    BinaryOpBlock* binary=dynamic_cast<BinaryOpBlock*>(block);
    if(binary!=nullptr){
        setInsertedOperatorInteractivity(binary->left);
        setInsertedOperatorInteractivity(binary->right);
    }
}

void setFloatTreeZValue(FloatBlock* block,qreal z){
    if(block==nullptr){
        return;
    }
    block->setZValue(z);
    UnaryOpBlock* unary=dynamic_cast<UnaryOpBlock*>(block);
    if(unary!=nullptr){
        setFloatTreeZValue(unary->slab,z+1);
        return;
    }
    BinaryOpBlock* binary=dynamic_cast<BinaryOpBlock*>(block);
    if(binary!=nullptr){
        setFloatTreeZValue(binary->left,z+1);
        setFloatTreeZValue(binary->right,z+1);
    }
}

FloatBlock* detachOperatorFromParent(FloatBlock* moving){
    if(moving==nullptr||!moving->isOperator()){
        return nullptr;
    }
    FloatBlock* parentBlock=dynamic_cast<FloatBlock*>(moving->parentItem());
    FloatCodeBlock* codeParent=dynamic_cast<FloatCodeBlock*>(moving->parentItem());
    if(parentBlock==nullptr&&codeParent==nullptr){
        return nullptr;
    }

    QPointF oldScenePos=moving->scenePos();
    QGraphicsItem* parentItem=parentBlock!=nullptr?
        static_cast<QGraphicsItem*>(parentBlock):static_cast<QGraphicsItem*>(codeParent);
    FloatBlock* placeholder=new FloatBlock(0,0,false,parentItem);
    placeholder->setMovable(true);

    UnaryOpBlock* unaryParent=dynamic_cast<UnaryOpBlock*>(parentBlock);
    if(unaryParent!=nullptr&&unaryParent->slab==moving){
        unaryParent->slab=placeholder;
    }
    BinaryOpBlock* binaryParent=dynamic_cast<BinaryOpBlock*>(parentBlock);
    if(binaryParent!=nullptr){
        if(binaryParent->left==moving){
            binaryParent->left=placeholder;
        }
        if(binaryParent->right==moving){
            binaryParent->right=placeholder;
        }
    }
    if(codeParent!=nullptr&&codeParent->value==moving){
        codeParent->value=placeholder;
    }

    moving->setParentItem(nullptr);
    moving->setPos(oldScenePos);
    moving->setMovable(true);
    eraseFloatBlockRecord(moving);
    floatBlocks.push_back(moving);
    if(parentBlock!=nullptr){
        refreshFloatAncestors(parentBlock);
    }
    if(codeParent!=nullptr){
        codeParent->refreshSize();
    }
    showAbsorbShadow(moving,placeholder);
    moving->absorbTarget=placeholder;
    return placeholder;
}

void collectFloatValueTargets(FloatBlock* root,FloatBlock* moving,vector<FloatBlock*>& targets){
    if(root==nullptr||root==moving){
        return;
    }
    if(rootFloatBlock(root)==moving||rootFloatBlock(root)->isbase){
        return;
    }
    if(!root->isOperator()){
        targets.push_back(root);
        return;
    }
    UnaryOpBlock* unary=dynamic_cast<UnaryOpBlock*>(root);
    if(unary!=nullptr){
        collectFloatValueTargets(unary->slab,moving,targets);
        return;
    }
    BinaryOpBlock* binary=dynamic_cast<BinaryOpBlock*>(root);
    if(binary!=nullptr){
        collectFloatValueTargets(binary->left,moving,targets);
        collectFloatValueTargets(binary->right,moving,targets);
    }
}

FloatBlock* findAbsorbTarget(FloatBlock* moving){
    if(moving==nullptr||!moving->isOperator()){
        return nullptr;
    }
    vector<FloatBlock*> targets;
    for(FloatBlock* root:floatBlocks){
        collectFloatValueTargets(root,moving,targets);
    }
    for(CodeBlock* block:codeBlocks){
        FloatCodeBlock* floatCode=dynamic_cast<FloatCodeBlock*>(block);
        if(floatCode!=nullptr){
            collectFloatValueTargets(floatCode->value,moving,targets);
        }
    }
    FloatBlock* bestTarget=nullptr;
    qreal bestDistance=floatAttachDistance;
    for(FloatBlock* target:targets){
        qreal distance=QLineF(moving->scenePos(),target->scenePos()).length();
        if(distance<bestDistance){
            bestDistance=distance;
            bestTarget=target;
        }
    }
    return bestTarget;
}

void showAbsorbShadow(FloatBlock* moving,FloatBlock* target){
    if(moving==nullptr||target==nullptr||moving->scene()==nullptr){
        return;
    }
    if(moving->absorbShadow==nullptr){
        moving->absorbShadow=new QGraphicsPolygonItem();
        moving->absorbShadow->setBrush(Qt::NoBrush);
        moving->absorbShadow->setPen(QPen(QColor(255,210,0),2));
    }
    QPolygonF shape;
    shape<<QPointF(-2,-2)<<QPointF(target->len+2,-2)
         <<QPointF(target->len+2,target->wid+2)<<QPointF(-2,target->wid+2);
    moving->absorbShadow->setPolygon(shape);
    moving->absorbShadow->setPos(target->scenePos());
    moving->absorbShadow->setZValue(absorbHighlightZ);
    if(moving->absorbShadow->scene()==nullptr){
        moving->scene()->addItem(moving->absorbShadow);
    }
    moving->absorbShadow->setZValue(absorbHighlightZ);
    moving->absorbShadow->show();
}

void clearAbsorbShadow(FloatBlock* block){
    if(block==nullptr){
        return;
    }
    block->absorbTarget=nullptr;
    if(block->absorbShadow!=nullptr){
        if(block->absorbShadow->scene()!=nullptr){
            block->absorbShadow->scene()->removeItem(block->absorbShadow);
        }
        delete block->absorbShadow;
        block->absorbShadow=nullptr;
    }
}

void attachOperatorToTarget(FloatBlock* moving,FloatBlock* target){
    if(moving==nullptr||target==nullptr||!moving->isOperator()){
        return;
    }
    clearAbsorbShadow(moving);
    FloatBlock* parentBlock=dynamic_cast<FloatBlock*>(target->parentItem());
    FloatCodeBlock* codeParent=dynamic_cast<FloatCodeBlock*>(target->parentItem());
    if(codeParent!=nullptr&&codeParent->value==target){
        QPointF targetLocalPos=target->pos();
        codeParent->value=moving;
        eraseFloatBlockRecord(moving);
        moving->setParentItem(codeParent);
        moving->setPos(targetLocalPos);
        moving->dragging=false;
        moving->moved=false;
        setInsertedOperatorInteractivity(moving);
        delete target;
        codeParent->refreshSize();
        return;
    }
    if(parentBlock==nullptr){
        QPointF targetPos=target->pos();
        eraseFloatBlockRecord(target);
        if(target->scene()!=nullptr){
            target->scene()->removeItem(target);
        }
        delete target;
        moving->setPos(targetPos);
        rememberFloatBlockStagePos(moving,scrollWorkspace);
        return;
    }

    QPointF targetLocalPos=target->pos();
    UnaryOpBlock* unaryParent=dynamic_cast<UnaryOpBlock*>(parentBlock);
    if(unaryParent!=nullptr&&unaryParent->slab==target){
        unaryParent->slab=moving;
    }
    BinaryOpBlock* binaryParent=dynamic_cast<BinaryOpBlock*>(parentBlock);
    if(binaryParent!=nullptr){
        if(binaryParent->left==target){
            binaryParent->left=moving;
        }
        if(binaryParent->right==target){
            binaryParent->right=moving;
        }
    }

    eraseFloatBlockRecord(moving);
    moving->setParentItem(parentBlock);
    moving->setPos(targetLocalPos);
    moving->dragging=false;
    moving->moved=false;
    setInsertedOperatorInteractivity(moving);
    delete target;
    refreshFloatAncestors(parentBlock);
}

void FloatBlock::mousePressEvent(QGraphicsSceneMouseEvent* event){
    if(event->button()!=Qt::LeftButton){
        event->ignore();
        return;
    }
    if(!movable){
        event->ignore();
        return;
    }
    if(parentItem()!=nullptr){
        if(isOperator()){
            detachOperatorFromParent(this);
            setZValue(draggingZ);
            setFloatTreeZValue(this,draggingZ);
            dragging=true;
            moved=false;
            mouseOffset=event->pos();
            grabMouse();
            event->accept();
            return;
        }
        moved=false;
        event->accept();
        return;
    }
    if(isbase){
        FloatBlock* tempBlock=copy();
        tempBlock->idx=static_cast<int>(floatBlocks.size());
        floatBlocks.push_back(tempBlock);
        scene()->addItem(tempBlock);
        tempBlock->setZValue(draggingZ);
        setFloatTreeZValue(tempBlock,draggingZ);
        tempBlock->dragging=true;
        tempBlock->scrollArea=scrollNone;
        tempBlock->moved=false;
        tempBlock->mouseOffset=event->pos();
        tempBlock->grabMouse();
        event->accept();
        return;
    }
    setZValue(draggingZ);
    setFloatTreeZValue(this,draggingZ);
    scrollArea=scrollNone;
    dragging=true;
    moved=false;
    mouseOffset=event->pos();
    event->accept();
}

void FloatBlock::mouseMoveEvent(QGraphicsSceneMouseEvent* event){
    if(parentItem()!=nullptr){
        moved=true;
        event->accept();
        return;
    }
    if(dragging){
        if(QLineF(event->screenPos(),event->buttonDownScreenPos(Qt::LeftButton)).length()>3){
            moved=true;
        }
        setPos(event->scenePos()-mouseOffset);
        FloatBlock* target=findAbsorbTarget(this);
        if(target!=nullptr){
            absorbTarget=target;
            showAbsorbShadow(this,target);
        }
        else{
            clearAbsorbShadow(this);
        }
        event->accept();
        return;
    }
    QGraphicsPolygonItem::mouseMoveEvent(event);
}

void FloatBlock::mouseReleaseEvent(QGraphicsSceneMouseEvent* event){
    if(parentItem()!=nullptr){
        if(!moved&&!isOperator()){
            bool ok=false;
            double value=QInputDialog::getDouble(nullptr,"Edit data","data",data,-1000000,1000000,6,&ok);
            if(ok){
                setData(value);
            }
        }
        event->accept();
        return;
    }
    if(dragging){
        dragging=false;
        ungrabMouse();
        if(absorbTarget!=nullptr&&isOperator()){
            FloatBlock* target=absorbTarget;
            attachOperatorToTarget(this,target);
            event->accept();
            return;
        }
        clearAbsorbShadow(this);
        if(!inWorkspace(this)){
            deleteFloatBlock(this);
            return;
        }
        rememberFloatBlockStagePos(this,scrollWorkspace);
        if(!moved&&!isbase&&!isOperator()){
            bool ok=false;
            double value=QInputDialog::getDouble(nullptr,"Edit data","data",data,-1000000,1000000,6,&ok);
            if(ok){
                setData(value);
            }
        }
        event->accept();
        return;
    }
    QGraphicsPolygonItem::mouseReleaseEvent(event);
}

void CodeBlock::mousePressEvent(QGraphicsSceneMouseEvent* event){
    if(isbase){
        CodeBlock* tempBlock=copy();
        if(dynamic_cast<StartBlock*>(tempBlock)==nullptr){
            tempBlock->idx=codeBlocks.size();
            codeBlocks.push_back(tempBlock);
        }
        else{
            tempBlock->idx=-1;
        }
        scene()->addItem(tempBlock);

        tempBlock->ismoving=true;
        CodeBlock* curr=tempBlock;
        if(curr->pre!=nullptr){
            curr->pre->next=nullptr;
            curr->pre=nullptr;
        }
        while(curr->next!=nullptr){
            curr=curr->next;
            curr->ismoving=true;
            curr->setZValue(draggingZ);
            FloatCodeBlock* floatCode=dynamic_cast<FloatCodeBlock*>(curr);
            if(floatCode!=nullptr){
                setFloatTreeZValue(floatCode->value,draggingZ+1);
            }
        }
        tempBlock->setZValue(draggingZ);
        FloatCodeBlock* tempFloatCode=dynamic_cast<FloatCodeBlock*>(tempBlock);
        if(tempFloatCode!=nullptr){
            setFloatTreeZValue(tempFloatCode->value,draggingZ+1);
        }
        tempBlock->shadow->setPos(tempBlock->pos());
        tempBlock->shadow->setZValue(draggingZ-1);
        scene()->addItem(tempBlock->shadow);
        tempBlock->dragging=true;
        tempBlock->scrollArea=scrollNone;
        tempBlock->mouseOffset=event->pos();
        tempBlock->grabMouse();
        event->accept();
        return;
    }
    ismoving=true;
    CodeBlock* curr=this;
    if(curr->pre!=nullptr){
        curr->pre->next=nullptr;
        curr->pre=nullptr;
    }
    while(curr->next!=nullptr){
        curr=curr->next;
        curr->ismoving=true;
        curr->setZValue(draggingZ);
        FloatCodeBlock* floatCode=dynamic_cast<FloatCodeBlock*>(curr);
        if(floatCode!=nullptr){
            setFloatTreeZValue(floatCode->value,draggingZ+1);
        }
    }
    setZValue(draggingZ);
    FloatCodeBlock* selfFloatCode=dynamic_cast<FloatCodeBlock*>(this);
    if(selfFloatCode!=nullptr){
        setFloatTreeZValue(selfFloatCode->value,draggingZ+1);
    }
    scrollArea=scrollNone;
    shadow->setPos(pos());
    shadow->setZValue(draggingZ-1);
    scene()->addItem(shadow);
    dragging=true;
    mouseOffset=event->pos();
    event->accept();
}

void CodeBlock::mouseMoveEvent(QGraphicsSceneMouseEvent* event){
    int totalwid=wid;
    if(preTarget!=nullptr&&preTarget->next!=nullptr){
        CodeBlock* curr=preTarget->next;
        curr->setPos(curr->calculatePos());
        while(curr->next!=nullptr){
            curr=curr->next;
            curr->setPos(curr->calculatePos());
        }
    }
    CodeBlock* curr=this;
    preidx=-1;
    nextidx=-1;
    preTarget=nullptr;
    nextTarget=nullptr;
    while(curr->next!=nullptr){
        curr=curr->next;
        totalwid+=curr->wid;
    }
    if(dragging){
        setPos(event->scenePos()-mouseOffset);
        shadow->setPos(pos());
        if(dynamic_cast<StartBlock*>(this)==nullptr&&currentStartBlock!=nullptr&&!currentStartBlock->ismoving){
            QPointF startBottom=currentStartBlock->pos()+QPointF(0,currentStartBlock->wid);
            if(QLineF(pos(),startBottom).length()<20){
                preidx=currentStartBlock->idx;
                preTarget=currentStartBlock;
                shadow->setPos(startBottom);
            }
        }
        for(auto & otherBlock:codeBlocks){
            if(otherBlock==this){
                continue;
            }
            if(otherBlock->ismoving){
                otherBlock->setPos(otherBlock->calculatePos());
                continue;
            }
            QPointF otherPos=otherBlock->pos();
            if(dynamic_cast<StartBlock*>(this)==nullptr&&preTarget==nullptr&&nextTarget==nullptr&&QLineF(pos(),otherPos+
                QPointF(0,otherBlock->wid)).length()<20){
                preidx=otherBlock->idx;
                preTarget=otherBlock;
                shadow->setPos(otherBlock->pos()+QPointF(0,otherBlock->wid));
            }
        }
        for(auto & otherBlock:codeBlocks){
            if(otherBlock==this){
                continue;
            }
            if(otherBlock->ismoving){
                continue;
            }
            QPointF otherPos=otherBlock->pos();
            if(dynamic_cast<StartBlock*>(otherBlock)!=nullptr){
                continue;
            }
            if(nextTarget==nullptr&&preTarget==nullptr&&
                QLineF(pos()+QPointF(0,totalwid),otherPos).length()<20){
                if(otherBlock->pre!=nullptr){
                    continue;
                }
                nextidx=otherBlock->idx;
                nextTarget=otherBlock;
                shadow->setPos(otherBlock->pos()-QPointF(0,wid));
            }
        }
        if(preTarget!=nullptr&&preTarget->next!=nullptr){
            curr=preTarget->next;
            curr->setPos(curr->pos()+QPointF(0,totalwid));
            while(curr->next!=nullptr){
                curr=curr->next;
                curr->setPos(curr->pos()+QPointF(0,totalwid));
            }
        }
        event->accept();
        return;
    }
    QGraphicsPolygonItem::mouseMoveEvent(event);
}

void CodeBlock::mouseReleaseEvent(QGraphicsSceneMouseEvent* event){
    CodeBlock* curr=this;
    int totalwid=wid;
    while(curr->next!=nullptr){
        curr=curr->next;
        totalwid+=curr->wid;
    }
    if(isbase){
        ungrabMouse();
        event->accept();
        return;
    }
    curr=this;
    while(curr->next!=nullptr){
        curr=curr->next;
    }
    if(dragging){
        StartBlock* releasedStart=dynamic_cast<StartBlock*>(this);
        if(releasedStart!=nullptr&&currentStartBlock!=nullptr&&currentStartBlock!=releasedStart){
            releasedStart->ismoving=false;
            scene()->removeItem(shadow);
            deleteCodeBlock(this);
            return;
        }
        if(!isbase&&!inWorkspace(this)){
            scene()->removeItem(shadow);
            CodeBlock* curr=this;
            while(curr->next!=nullptr){
                CodeBlock* nxt=curr->next;
                deleteCodeBlock(curr);
                curr=nxt;
            }
            deleteCodeBlock(curr);
            return;
        }
        if(releasedStart!=nullptr){
            currentStartBlock=releasedStart;
            currentStartBlock->ismoving=false;
        }
        dragging=false;
        if(preTarget!=nullptr){
            CodeBlock* b=preTarget;
            if(b->next!=nullptr){
                curr=this;
                while(curr->next!=nullptr){
                    curr=curr->next;
                }
                curr->next=b->next;
                b->next->pre=curr;
                b->next=this;
                pre=b;
            }
            else{
                b->next=this;
                pre=b;
            }
            curr=this;
            while(curr->next!=nullptr){
                curr->setPos(curr->calculatePos());
                curr=curr->next;
            }
            curr->setPos(curr->calculatePos());
        }
        if(nextTarget!=nullptr){
            CodeBlock* b=nextTarget;
            curr=this;
            while(curr->next!=nullptr){
                curr->setPos(b->pos()-QPointF(0,totalwid));
                totalwid-=curr->wid;
                curr=curr->next;
            }
            curr->setPos(b->pos()-QPointF(0,totalwid));
            b->pre=curr;
            curr->next=b;
        }
        for(auto otherBlock:codeBlocks){
            otherBlock->ismoving=0;
        }
        if(currentStartBlock!=nullptr){
            currentStartBlock->ismoving=false;
        }
        curr=this;
        while(curr!=nullptr){
            rememberCodeBlockStagePos(curr,scrollWorkspace);
            curr=curr->next;
        }
        scene()->removeItem(shadow);
        ungrabMouse();
        event->accept();
        return;
    }
    curr=this;
    while(curr->next!=nullptr){
        curr=curr->next;
        curr->setPos(curr->calculatePos());
    }
    QGraphicsPolygonItem::mouseReleaseEvent(event);
}

void init(){
    for(int i=0;i<screensize;i++){
        for(int j=0;j<screensize;j++){
            mapdata[i][j]=0;
        }
    }
}

void resetRobot(){
    player->gridx=5;
    player->gridy=5;
    player->direction=0;
    player->SyncCell(squaresize,QPoint(20,20));
}

void drawStage(QGraphicsScene& scene){
    stage=scene.addRect(20,20,320,320);
    stage->setBrush(Qt::white);

    int squaresize=320/screensize;
    for(int i=0;i<screensize;i++){
        for(int j=0;j<screensize;j++){
            squares[i][j]=scene.addRect(0,0,squaresize,squaresize);
            if(mapdata[i][j]==1){
                squares[i][j]->setBrush(Qt::red);
            }
            else{
                squares[i][j]->setBrush(Qt::gray);
            }
            squares[i][j]->setPos(20+i*squaresize,20+j*squaresize);
        }
    }
    player->SyncCell(squaresize,QPoint(20,20));
    player->setZValue(10);
    scene.addItem(player);
    runButton=new Button(0,"");
    runButton->setPos(320,360);
    runButton->setZValue(20);
    runButton->onClick = [&](){
        resetRobot();
        if(currentStartBlock==nullptr||currentStartBlock->next==nullptr){
            return;
        }
        runningBlock=currentStartBlock->next;
        if(timerPtr!=nullptr){
            timerPtr->stop();
            timerPtr->start(300);
        }
    };
    scene.addItem(runButton);
}

void addPanelMasks(QGraphicsScene& scene,QRectF panelRect,bool protectStage=false){
    QColor maskColor(255,255,255);
    QVector<QRectF> masks;
    qreal leftMaskX=panelRect.left()-20;
    qreal leftMaskWidth=20;
    if(protectStage&&leftMaskX<341){
        leftMaskWidth=panelRect.left()-341;
        leftMaskX=341;
    }
    masks<<QRectF(panelRect.left(),0,panelRect.width(),panelRect.top())
         <<QRectF(panelRect.left(),panelRect.bottom(),panelRect.width(),540-panelRect.bottom())
         <<QRectF(leftMaskX,panelRect.top(),leftMaskWidth,panelRect.height())
         <<QRectF(panelRect.right(),panelRect.top(),20,panelRect.height());
    for(const QRectF& rect:masks){
        QGraphicsRectItem* mask=scene.addRect(rect);
        mask->setBrush(maskColor);
        mask->setPen(Qt::NoPen);
        mask->setZValue(panelMaskZ);
    }
    QGraphicsRectItem* border=scene.addRect(panelRect);
    border->setBrush(Qt::NoBrush);
    border->setPen(QPen(Qt::black,1.5));
    border->setZValue(panelMaskZ+1);
}

void drawToolbox(QGraphicsScene& scene){
    toolboxBackground=scene.addRect(360,20,180,500);
    toolboxBackground->setBrush(Qt::white);
    int y=20;
    auto addCode=[&](CodeBlock* block){
        setCodeBlockStagePos(block,scrollToolbox,QPointF(20,y));
        block->setZValue(10);
        scene.addItem(block);
        baseCodeBlocks.push_back(block);
        y+=60;
    };
    auto addFloat=[&](FloatBlock* block){
        setFloatBlockStagePos(block,scrollToolbox,QPointF(20,y));
        block->setZValue(10);
        scene.addItem(block);
        floatBlocks.push_back(block);
        y+=60;
    };

    addCode(new StartBlock(0,true));
    addCode(new CodeBlock(0,"turn left",0,true));
    addCode(new CodeBlock(1,"turn right",1,true));
    addCode(new FloatCodeBlock(3,"move",3,nullptr,true));
    addCode(new FloatCodeBlock(4,"wait",4,nullptr,true));

    const QString unaryNames[]={"sin","cos","tan","asin","acos","atan","ln","log10","floor","abs"};
    for(int i=0;i<10;i++){
        addFloat(new UnaryOpBlock(i,unaryNames[i],i,nullptr,true));
    }

    const QString binaryNames[]={"+","-","*","/","pow","arg","max","min"};
    for(int i=0;i<8;i++){
        addFloat(new BinaryOpBlock(i,binaryNames[i],i,nullptr,nullptr,true));
    }

    toolboxSlider=new ScrollSlider(524,30,470);
    toolboxSlider->onChanged=[](qreal value){
        toolboxScrollY=value*900;
        syncScrollArea(scrollToolbox);
    };
    scene.addItem(toolboxSlider);
    addPanelMasks(scene,QRectF(360,20,180,500),true);
}

void drawWorkspace(QGraphicsScene& scene){
    workspaceBackground=scene.addRect(560,20,380,500);
    workspaceBackground->setBrush(Qt::white);
    workspaceSlider=new ScrollSlider(924,30,470);
    workspaceSlider->onChanged=[](qreal value){
        workspaceScrollY=value*500;
        syncScrollArea(scrollWorkspace);
    };
    scene.addItem(workspaceSlider);
    addPanelMasks(scene,QRectF(560,20,380,500));
}

int ui::runApp(int argc,char* argv[]){
    init();
    QApplication app(argc,argv);

    player=new Robot();

    QGraphicsScene scene;
    QGraphicsRectItem* background=scene.addRect(-10000,-10000,20000,20000);
    background->setBrush(Qt::white);
    background->setPen(Qt::NoPen);
    background->setZValue(-100000);
    drawStage(scene);
    drawToolbox(scene);
    drawWorkspace(scene);

    QTimer timer;
    timerPtr=&timer;
    QPoint stageoffset(20,20);
    core::BlockExecutor executor;
    RobotActionAdapter robotActions(player);
    QObject::connect(&timer,&QTimer::timeout,[&](){
        if(runningBlock==nullptr){
            timer.stop();
            return;
        }

        executor.executeOne(runningBlock->type,codeBlockFloatValue(runningBlock),robotActions);

        player->SyncCell(squaresize,stageoffset);
        runningBlock=runningBlock->next;

        if(runningBlock==nullptr){
            timer.stop();
        }
    });

    AppGraphicsView view(&scene);
    scene.setSceneRect(0,0,960,540);
    view.setAlignment(Qt::AlignLeft|Qt::AlignTop);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setFixedSize(960,540);
    view.setWindowFlags(view.windowFlags()&~Qt::WindowMaximizeButtonHint);
    view.show();

    return app.exec();
}
