#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPainterPath>
#include <QPen>
template<class T>
T max(const T & a,const T & b){
    return a>b?a:b;
}
template<class T>
T min(const T & a,const T & b){
    return a<b?a:b;
}
int clamp(const int & a){
    return min(79,max(0,a));
}
struct vec2{
    int x,y;
    vec2(){x=0,y=0;}
    vec2(int _x,int _y){
        x=clamp(_x);
        y=clamp(_y);
    }
    vec2 operator+(const vec2 & other)const{
        return vec2(clamp(x+other.x),clamp(y+other.y));
    }
    vec2 operator-(const vec2 & other)const{
        return vec2(clamp(x-other.x),clamp(y-other.y));
    }
    int dot(const vec2 & other)const{
        return x*other.x+y*other.y;
    }
    int cross(const vec2 & other)const{
        return x*other.y-y*other.x;
    }
};
class CodeBlock:public QGraphicsPathItem{
public:
    QPainterPath outline;
    bool dragging;
    QPointF dragstart;
    QGraphicsTextItem *text;
    QGraphicsPathItem *shadow;
    QPointF shadowpos;
    bool cansnap;
    QPointF lastpos;
    QPointF mouseoffset;
    CodeBlock *pre;
    CodeBlock *nxt;
    CodeBlock *snaptarget;
    CodeBlock *spawned;
    CodeBlock *dragtail;
    CodeBlock *previewtarget;
    CodeBlock *previewoldnext;
    CodeBlock *previewoldpre;
    int snapmode;
    int previewmode;
    int dragcount;
    int idx;
    qreal oldz;
    bool istemplate;
    int blocktype;
    QColor blockcolor;
    QString words;
    CodeBlock(const QColor & color,const QString & _words,int _idx,bool _istemplate=false,int _blocktype=0,QGraphicsItem * parent=nullptr):QGraphicsPathItem(parent){
        dragging=false;
        shadow=nullptr;
        cansnap=false;
        pre=nullptr;
        nxt=nullptr;
        snaptarget=nullptr;
        spawned=nullptr;
        dragtail=nullptr;
        previewtarget=nullptr;
        previewoldnext=nullptr;
        previewoldpre=nullptr;
        snapmode=0;
        previewmode=0;
        dragcount=0;
        oldz=10;
        istemplate=_istemplate;
        blocktype=_blocktype;
        blockcolor=color;
        words=_words;
        outline.moveTo(8,0);
        if(blocktype==0){
            outline.lineTo(24,0);
            outline.quadTo(27,10,33,10);
            outline.lineTo(43,10);
            outline.quadTo(49,10,52,0);
        }
        outline.lineTo(112,0);
        outline.quadTo(120,0,120,8);
        outline.lineTo(120,32);
        outline.quadTo(120,40,112,40);
        outline.lineTo(52,40);
        outline.quadTo(49,50,43,50);
        outline.lineTo(33,50);
        outline.quadTo(27,50,24,40);
        outline.lineTo(8,40);
        outline.quadTo(0,40,0,32);
        outline.lineTo(0,8);
        outline.quadTo(0,0,8,0);
        outline.closeSubpath();
        setPath(outline);
        setBrush(color);
        setPen(QPen(QColor(50,80,150),2));
        setFlag(QGraphicsItem::ItemIsMovable,false);
        setFlag(QGraphicsItem::ItemIsSelectable,false);
        setZValue(10);
        text=new QGraphicsTextItem(words,this);
        text->setDefaultTextColor(Qt::white);
        text->setPos(10,10);
        text->setZValue(1);
        idx=_idx;
    }
    void mousePressEvent(QGraphicsSceneMouseEvent * event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent * event);
    void begin_drag(const QPointF & scenemouse);
    void drag_to(const QPointF & scenemouse);
    void update_shadow();
    void finish_drag();
    bool in_chain(CodeBlock * block);
    bool in_drag_chain(CodeBlock * block);
    int chain_len();
    CodeBlock * chain_tail();
    void move_chain_by(const QPointF & delta);
    void move_next_by(const QPointF & delta);
    void move_drag_next_by(const QPointF & delta);
    void set_chain_z(qreal z);
    void set_drag_chain_z(qreal z);
    void detach_preview();
    void apply_preview();
    bool chain_in_workarea();
    void delete_single_block();
    CodeBlock * copy_chain(const QPointF & offset);
    void delete_chain();
};
const int screensize=40;
int map[screensize][screensize];
vec2 robotpos;
QGraphicsRectItem *screen;
QGraphicsRectItem *squares[screensize][screensize];
CodeBlock **moveblocks;
int lenmoveblocks;
const int maxmoveblocks=200;
QRectF workarearect(540,20,400,500);
qreal topz=1000;
void add_moveblock(CodeBlock * block){
    if(lenmoveblocks<maxmoveblocks){
        block->idx=lenmoveblocks;
        moveblocks[lenmoveblocks]=block;
        lenmoveblocks++;
    }
}
void remove_moveblock(CodeBlock * block){
    for(int i=0;i<lenmoveblocks;i++){
        if(moveblocks[i]==block){
            for(int j=i+1;j<lenmoveblocks;j++){
                moveblocks[j-1]=moveblocks[j];
                moveblocks[j-1]->idx=j-1;
            }
            lenmoveblocks--;
            return;
        }
    }
}
void delete_extra_blocks(){
    bool keep[maxmoveblocks];
    for(int i=0;i<maxmoveblocks;i++){
        keep[i]=false;
    }
    for(int i=0;i<lenmoveblocks;i++){
        CodeBlock * block=moveblocks[i];
        if(block->blocktype==1&&block->pre==nullptr){
            CodeBlock * now=block;
            while(now!=nullptr){
                if(now->idx>=0&&now->idx<maxmoveblocks){
                    keep[now->idx]=true;
                }
                now=now->nxt;
            }
        }
    }
    for(int i=lenmoveblocks-1;i>=0;i--){
        CodeBlock * block=moveblocks[i];
        if(!keep[i]){
            if(block->pre==nullptr){
                block->delete_chain();
            }
            else{
                block->delete_single_block();
            }
        }
    }
}
bool CodeBlock::in_chain(CodeBlock * block){
    CodeBlock * now=this;
    while(now!=nullptr){
        if(now==block){
            return true;
        }
        now=now->nxt;
    }
    return false;
}
bool CodeBlock::in_drag_chain(CodeBlock * block){
    CodeBlock * now=this;
    while(now!=nullptr){
        if(now==block){
            return true;
        }
        if(now==dragtail){
            break;
        }
        now=now->nxt;
    }
    return false;
}
int CodeBlock::chain_len(){
    int ans=0;
    CodeBlock * now=this;
    while(now!=nullptr){
        ans++;
        now=now->nxt;
    }
    return ans;
}
CodeBlock * CodeBlock::chain_tail(){
    CodeBlock * now=this;
    while(now->nxt!=nullptr){
        now=now->nxt;
    }
    return now;
}
void CodeBlock::move_chain_by(const QPointF & delta){
    CodeBlock * now=this;
    while(now!=nullptr){
        now->moveBy(delta.x(),delta.y());
        now=now->nxt;
    }
}
void CodeBlock::move_next_by(const QPointF & delta){
    CodeBlock * now=nxt;
    while(now!=nullptr){
        now->moveBy(delta.x(),delta.y());
        now=now->nxt;
    }
}
void CodeBlock::move_drag_next_by(const QPointF & delta){
    if(this==dragtail){
        return;
    }
    CodeBlock * now=nxt;
    while(now!=nullptr){
        now->moveBy(delta.x(),delta.y());
        if(now==dragtail){
            break;
        }
        now=now->nxt;
    }
}
void CodeBlock::set_chain_z(qreal z){
    CodeBlock * now=this;
    while(now!=nullptr){
        now->setZValue(z);
        now=now->nxt;
    }
}
void CodeBlock::set_drag_chain_z(qreal z){
    CodeBlock * now=this;
    int deep=0;
    while(now!=nullptr){
        now->setZValue(z+deep);
        if(now==dragtail){
            break;
        }
        now=now->nxt;
        deep++;
    }
}
void CodeBlock::detach_preview(){
    if(previewmode==1&&previewtarget!=nullptr){
        previewtarget->nxt=previewoldnext;
        if(previewoldnext!=nullptr){
            previewoldnext->pre=previewtarget;
            previewoldnext->move_chain_by(QPointF(0,-40*dragcount));
        }
        pre=nullptr;
        if(dragtail!=nullptr){
            dragtail->nxt=nullptr;
        }
    }
    if(previewmode==2&&previewtarget!=nullptr){
        if(previewoldpre!=nullptr){
            previewoldpre->nxt=previewtarget;
        }
        previewtarget->pre=previewoldpre;
        pre=nullptr;
        if(dragtail!=nullptr){
            dragtail->nxt=nullptr;
        }
    }
    previewtarget=nullptr;
    previewoldnext=nullptr;
    previewoldpre=nullptr;
    previewmode=0;
}
void CodeBlock::apply_preview(){
    if(!cansnap||snaptarget==nullptr){
        return;
    }
    if(snapmode==1){
        previewtarget=snaptarget;
        previewmode=1;
        previewoldnext=previewtarget->nxt;
        previewoldpre=nullptr;
        if(previewoldnext!=nullptr){
            previewoldnext->move_chain_by(QPointF(0,40*dragcount));
            previewoldnext->pre=dragtail;
        }
        previewtarget->nxt=this;
        pre=previewtarget;
        if(dragtail!=nullptr){
            dragtail->nxt=previewoldnext;
        }
    }
    if(snapmode==2){
        previewtarget=snaptarget;
        previewmode=2;
        previewoldpre=previewtarget->pre;
        previewoldnext=nullptr;
        if(previewoldpre!=nullptr){
            previewoldpre->nxt=this;
        }
        pre=previewoldpre;
        previewtarget->pre=dragtail;
        if(dragtail!=nullptr){
            dragtail->nxt=previewtarget;
        }
    }
}
bool CodeBlock::chain_in_workarea(){
    CodeBlock * now=this;
    while(now!=nullptr){
        if(!workarearect.contains(now->sceneBoundingRect())){
            return false;
        }
        if(dragtail!=nullptr&&now==dragtail){
            break;
        }
        now=now->nxt;
    }
    return true;
}
void CodeBlock::delete_single_block(){
    detach_preview();
    CodeBlock * oldpre=pre;
    CodeBlock * oldnext=nxt;
    if(oldpre!=nullptr){
        oldpre->nxt=oldnext;
    }
    if(oldnext!=nullptr){
        oldnext->pre=oldpre;
        oldnext->move_chain_by(QPointF(0,-40));
    }
    pre=nullptr;
    nxt=nullptr;
    if(shadow!=nullptr){
        scene()->removeItem(shadow);
        delete shadow;
        shadow=nullptr;
    }
    remove_moveblock(this);
    scene()->removeItem(this);
    delete this;
}
CodeBlock * CodeBlock::copy_chain(const QPointF & offset){
    CodeBlock * first=nullptr;
    CodeBlock * last=nullptr;
    CodeBlock * now=this;
    while(now!=nullptr){
        CodeBlock * copy=new CodeBlock(now->blockcolor,now->words,-1,false,now->blocktype);
        copy->setPos(now->scenePos()+offset);
        copy->setZValue(++topz);
        scene()->addItem(copy);
        add_moveblock(copy);
        if(first==nullptr){
            first=copy;
        }
        if(last!=nullptr){
            last->nxt=copy;
            copy->pre=last;
        }
        last=copy;
        now=now->nxt;
    }
    return first;
}
void CodeBlock::delete_chain(){
    detach_preview();
    if(pre!=nullptr){
        pre->nxt=nullptr;
        pre=nullptr;
    }
    CodeBlock * now=this;
    while(now!=nullptr){
        CodeBlock * next=now->nxt;
        bool stop=dragtail!=nullptr&&now==dragtail;
        if(now->pre!=nullptr){
            now->pre->nxt=nullptr;
        }
        if(now->nxt!=nullptr){
            now->nxt->pre=nullptr;
        }
        if(now->shadow!=nullptr){
            now->scene()->removeItem(now->shadow);
            delete now->shadow;
            now->shadow=nullptr;
        }
        remove_moveblock(now);
        now->scene()->removeItem(now);
        delete now;
        if(stop){
            break;
        }
        now=next;
    }
}
void CodeBlock::begin_drag(const QPointF & scenemouse){
    dragging=true;
    dragtail=chain_tail();
    dragcount=chain_len();
    dragstart=pos();
    lastpos=pos();
    mouseoffset=scenemouse-scenePos();
    if(pre!=nullptr){
        pre->nxt=nullptr;
        pre=nullptr;
    }
    setSelected(false);
    oldz=zValue();
    topz+=100;
    set_drag_chain_z(topz);
    if(shadow==nullptr){
        shadow=new QGraphicsPathItem();
        shadow->setPath(outline);
        shadow->setBrush(QColor(0,0,0,70));
        shadow->setPen(QPen(QColor(0,0,0,100),2));
        shadow->setZValue(5);
        scene()->addItem(shadow);
    }
    shadowpos=scenePos()+QPointF(6,6);
    shadow->setPos(shadowpos);
    shadow->show();
}
void CodeBlock::drag_to(const QPointF & scenemouse){
    setPos(scenemouse-mouseoffset);
    QPointF delta=pos()-lastpos;
    move_drag_next_by(delta);
    lastpos=pos();
    update_shadow();
}
void CodeBlock::update_shadow(){
    detach_preview();
    cansnap=false;
    snaptarget=nullptr;
    snapmode=0;
    shadowpos=scenePos()+QPointF(6,6);
    double bestdis=400;
    for(int i=0;i<lenmoveblocks;i++){
        if(in_drag_chain(moveblocks[i])){
            continue;
        }
        QPointF downtarget=moveblocks[i]->scenePos()+QPointF(0,40);
        QPointF downdis=scenePos()-downtarget;
        double downdis2=downdis.x()*downdis.x()+downdis.y()*downdis.y();
        if(downdis2<bestdis){
            cansnap=true;
            bestdis=downdis2;
            shadowpos=downtarget;
            snaptarget=moveblocks[i];
            snapmode=1;
        }
        QPointF uptarget=moveblocks[i]->scenePos()-QPointF(0,40*dragcount);
        QPointF updis=scenePos()-uptarget;
        double updis2=updis.x()*updis.x()+updis.y()*updis.y();
        if(updis2<bestdis){
            cansnap=true;
            bestdis=updis2;
            shadowpos=uptarget;
            snaptarget=moveblocks[i];
            snapmode=2;
        }
    }
    if(shadow!=nullptr){
        shadow->setPos(shadowpos);
    }
    apply_preview();
}
void CodeBlock::finish_drag(){
    dragging=false;
    if(cansnap){
        QPointF delta=shadowpos-pos();
        setPos(shadowpos);
        move_drag_next_by(delta);
    }
    else{
        detach_preview();
    }
    if(shadow!=nullptr){
        shadow->hide();
    }
    if(!chain_in_workarea()){
        delete_chain();
        return;
    }
    previewtarget=nullptr;
    previewoldnext=nullptr;
    previewoldpre=nullptr;
    previewmode=0;
    dragtail=nullptr;
    dragcount=0;
}
void CodeBlock::mousePressEvent(QGraphicsSceneMouseEvent * event){
    if(event->button()!=Qt::LeftButton){
        event->ignore();
        return;
    }
    if(istemplate){
        spawned=new CodeBlock(blockcolor,words,-1,false,blocktype);
        spawned->setPos(scenePos());
        scene()->addItem(spawned);
        add_moveblock(spawned);
        spawned->begin_drag(event->scenePos());
    }
    else{
        begin_drag(event->scenePos());
    }
    event->accept();
}
void CodeBlock::mouseMoveEvent(QGraphicsSceneMouseEvent * event){
    if(!(event->buttons()&Qt::LeftButton)){
        event->ignore();
        return;
    }
    if(istemplate){
        if(spawned!=nullptr){
            spawned->drag_to(event->scenePos());
        }
    }
    else{
        drag_to(event->scenePos());
    }
    event->accept();
}
void CodeBlock::mouseReleaseEvent(QGraphicsSceneMouseEvent * event){
    if(event->button()!=Qt::LeftButton){
        event->ignore();
        return;
    }
    if(istemplate){
        if(spawned!=nullptr){
            spawned->finish_drag();
            spawned=nullptr;
        }
    }
    else{
        finish_drag();
    }
    event->accept();
}
void CodeBlock::contextMenuEvent(QGraphicsSceneContextMenuEvent * event){
    if(istemplate){
        event->ignore();
        return;
    }
    QMenu menu;
    QAction * copyaction=menu.addAction("复制");
    QAction * deleteaction=menu.addAction("删除");
    QAction * action=menu.exec(event->screenPos());
    if(action==deleteaction){
        delete_single_block();
        return;
    }
    if(action==copyaction){
        copy_chain(QPointF(150,0));
        return;
    }
}
class GameScene:public QGraphicsScene{
public:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent * event)override{
        QGraphicsItem * item=itemAt(event->scenePos(),QTransform());
        while(item!=nullptr){
            if(dynamic_cast<CodeBlock *>(item)!=nullptr){
                QGraphicsScene::contextMenuEvent(event);
                return;
            }
            item=item->parentItem();
        }
        QMenu menu;
        QAction * cleanupaction=menu.addAction("删除多余积木");
        QAction * action=menu.exec(event->screenPos());
        if(action==cleanupaction){
            int ret=QMessageBox::question(nullptr,"确认","是否删除多余积木？",QMessageBox::Yes|QMessageBox::No);
            if(ret==QMessageBox::Yes){
                delete_extra_blocks();
            }
        }
    }
};
void init(){
    for(int i=0;i<screensize;i++){
        for(int j=0;j<screensize;j++){
            map[i][j]=rand()%2;
        }
    }
    robotpos=vec2(5,5);
}
void draw_screen(QGraphicsScene & scene){
    screen=scene.addRect(0,0,320,320);
    screen->setBrush(Qt::blue);
    screen->setPos(20,20);
    int squaresize=320/screensize;
    for(int i=0;i<screensize;i++){
        for(int j=0;j<screensize;j++){
            squares[i][j]=scene.addRect(0,0,squaresize,squaresize);
            if(map[i][j]==1){
                squares[i][j]->setBrush(Qt::red);
            }
            else{
                squares[i][j]->setBrush(Qt::gray);
            }
            squares[i][j]->setPos(20+i*squaresize,20+j*squaresize);
        }
    }
    int robotx=20+robotpos.x*squaresize;
    int roboty=20+robotpos.y*squaresize;
    QPen robotpen(Qt::green);
    robotpen.setWidth(2);
    scene.addLine(robotx+2,roboty+2,robotx+6,roboty+4,robotpen);
    scene.addLine(robotx+6,roboty+4,robotx+2,roboty+6,robotpen);
}
int main(int argc, char *argv[])
{
    init();
    QApplication app(argc, argv);

    GameScene scene;
    
    draw_screen(scene);

    QGraphicsRectItem *toolbox=scene.addRect(0,0,160,500);
    toolbox->setPos(360,20);
    toolbox->setBrush(Qt::white);

    lenmoveblocks=0;
    moveblocks=new CodeBlock*[maxmoveblocks];
    moveblocks[0]=new CodeBlock(QColor(95,140,235),"前进",0);
    moveblocks[0]->setPos(380,40);
    scene.addItem(moveblocks[0]);
    moveblocks[1]=new CodeBlock(QColor(95,140,235),"左转",1);
    moveblocks[1]->setPos(380,100);
    scene.addItem(moveblocks[1]);
    moveblocks[2]=new CodeBlock(QColor(95,140,235),"右转",2);
    moveblocks[2]->setPos(380,160);
    scene.addItem(moveblocks[2]);
    moveblocks[3]=new CodeBlock(QColor(95,140,235),"后退",3);
    moveblocks[3]->setPos(380,220);
    scene.addItem(moveblocks[3]);
    moveblocks[4]=new CodeBlock(QColor(95,140,235),"攻击",4);
    moveblocks[4]->setPos(380,280);
    scene.addItem(moveblocks[4]);
    moveblocks[5]=new CodeBlock(QColor(95,140,235),"start",5,true,1);
    moveblocks[5]->setPos(380,340);
    scene.addItem(moveblocks[5]);
    for(int i=0;i<6;i++){
        moveblocks[i]->istemplate=true;
        moveblocks[i]->idx=-1;
    }

    QGraphicsRectItem *workarea=scene.addRect(0,0,400,500);
    workarea->setPos(540,20);
    workarea->setBrush(Qt::white);
    

    QGraphicsView view(&scene);
    view.resize(960, 540);
    view.show();

    return app.exec();
}
