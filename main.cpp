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
#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>
using std::vector;
using std::function;

const int shadowPadding=0;
const int screensize=40;
const int squaresize=320/screensize;
const double PI=3.14159265358979323846;
QTimer* timerPtr = nullptr;
class Button:public QGraphicsPolygonItem{
public:
    int type;
    QString s;
    QGraphicsPolygonItem* shadow;
    function<void()> onClick;
    Button(int _type,QString _s,QGraphicsItem* parent=nullptr):QGraphicsPolygonItem(parent){
        s=_s,type=_type;
        QPolygonF shape;
        double radius=10.0;
        for(int i=0;i<32;i++){
            double angle=2.0*PI*i/32.0;
            shape<<QPointF(radius+radius*std::cos(angle),radius+radius*std::sin(angle));
        }
        setPolygon(shape);
        setBrush(QColor(80,140,235));
        setPen(Qt::NoPen);

        QPolygonF shadowShape;
        shadowShape<<QPointF(0,0)<<QPointF(20,0)<<QPointF(20,20)<<QPointF(0,20);
        shadow=new QGraphicsPolygonItem();
        shadow->setPolygon(shadowShape);
        shadow->setBrush(QColor(180,180,180,120));
        shadow->setPen(Qt::NoPen);
        shadow->hide();

        setAcceptHoverEvents(true);
        setAcceptedMouseButtons(Qt::LeftButton);
    }
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override{
        if(scene()!=nullptr&&shadow->scene()==nullptr){
            scene()->addItem(shadow);
        }
        shadow->setPos(pos());
        shadow->setZValue(zValue()-1);
        shadow->show();
        event->accept();
    }
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override{
        shadow->hide();
        event->accept();
    }
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override{
        if(onClick){
            onClick();
        }
        event->accept();
    }
};
class CodeBlock:public QGraphicsPolygonItem{
public:
    int len;
    int wid;
    int idx;
    QString s;
    int type;
    bool dragging;
    QPointF mouseOffset;
    CodeBlock* pre;
    CodeBlock* next;
    QGraphicsPolygonItem* shadow;
    bool isbase;
    bool ismoving;
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
        QGraphicsTextItem* text=new QGraphicsTextItem(ss,this);
        text->setDefaultTextColor(Qt::white);
        text->setPos(10,10);
        len=text->boundingRect().width()+30;
        wid=40;
        s=ss;
        shape<<QPoint(0,0)<<QPoint(len,0)
             <<QPoint(len,wid)<<QPoint(0,wid);
        setPolygon(shape);
        setBrush(Qt::blue);
        setPen(Qt::NoPen);
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
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
};
class StartBlock:public CodeBlock{
public:
    StartBlock(int _idx,int base=false,QGraphicsItem * parent=nullptr):
        CodeBlock(-1,"start",_idx,base,parent){
        setBrush(QColor(40,160,90));
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

int maxZ=10;
int mapdata[screensize][screensize];
Robot* player=nullptr;
QGraphicsRectItem* stage;
QGraphicsRectItem* toolboxBackground;
QGraphicsRectItem* workspaceBackground;
QGraphicsRectItem* squares[screensize][screensize];
Button* runButton=nullptr;
vector<CodeBlock*> codeBlocks;
vector<CodeBlock*> baseCodeBlocks;
StartBlock* currentStartBlock=nullptr;
CodeBlock* runningBlock=nullptr;
bool inWorkspace(CodeBlock* block){
    QRectF workspaceRect(560,20,380,500);
    return workspaceRect.contains(block->sceneBoundingRect());
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
            maxZ+=10;
            curr->setZValue(maxZ);
        }
        maxZ+=10;
        tempBlock->setZValue(maxZ);
        tempBlock->shadow->setPos(tempBlock->pos());
        scene()->addItem(tempBlock->shadow);
        tempBlock->dragging=true;
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
        maxZ+=10;
        curr->setZValue(maxZ);
    }
    maxZ+=10;
    setZValue(maxZ);
    shadow->setPos(pos());
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

void drawToolbox(QGraphicsScene& scene){
    toolboxBackground=scene.addRect(360,20,180,500);
    toolboxBackground->setBrush(Qt::white);
    StartBlock* startBlock=new StartBlock(0,true);
    CodeBlock* turnLeftBlock=new CodeBlock(0,"turn left",0,true);
    CodeBlock* turnRightBlock=new CodeBlock(1,"turn right",1,true);
    CodeBlock* moveStraightBlock=new CodeBlock(2,"move 1 step",2,true);
    startBlock->setPos(380,40);
    turnLeftBlock->setPos(380,100);
    turnRightBlock->setPos(380,160);
    moveStraightBlock->setPos(380,220);
    startBlock->setZValue(10);
    turnLeftBlock->setZValue(10);
    turnRightBlock->setZValue(10);
    moveStraightBlock->setZValue(10);
    scene.addItem(startBlock);
    scene.addItem(turnLeftBlock);
    scene.addItem(turnRightBlock);
    scene.addItem(moveStraightBlock);
    baseCodeBlocks.push_back(startBlock);
    baseCodeBlocks.push_back(turnLeftBlock);
    baseCodeBlocks.push_back(turnRightBlock);
    baseCodeBlocks.push_back(moveStraightBlock);
}

void drawWorkspace(QGraphicsScene& scene){
    workspaceBackground=scene.addRect(560,20,380,500);
    workspaceBackground->setBrush(Qt::white);
}

int main(int argc,char* argv[]){
    init();
    QApplication app(argc,argv);

    player=new Robot();

    QGraphicsScene scene;
    drawStage(scene);
    drawToolbox(scene);
    drawWorkspace(scene);

    QTimer timer;
    timerPtr=&timer;
    QPoint stageoffset(20,20);
    QObject::connect(&timer,&QTimer::timeout,[&](){
        if(runningBlock==nullptr){
            timer.stop();
            return;
        }

        if(runningBlock->type==0){
            player->turnLeft();
        }
        else if(runningBlock->type==1){
            player->turnRight();
        }
        else if(runningBlock->type==2){
            player->moveForward(1);
        }

        player->SyncCell(squaresize,stageoffset);
        runningBlock=runningBlock->next;

        if(runningBlock==nullptr){
            timer.stop();
        }
    });

    QGraphicsView view(&scene);
    scene.setSceneRect(0,0,960,540);
    view.setAlignment(Qt::AlignLeft|Qt::AlignTop);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setFixedSize(960,540);
    view.setWindowFlags(view.windowFlags()&~Qt::WindowMaximizeButtonHint);
    view.show();

    return app.exec();
}
