#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QGraphicsPolygonItem>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QImage>
#include <QPolygonF>
#include <QBrush>
#include <QTimer>
#include <QInputDialog>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTextDocument>
#include <QTextStream>
#include <QThread>
#include <QTransform>
#include <QWheelEvent>
#include <QFileDialog>
#include <QCloseEvent>
#include <QMainWindow>
#include <QPixmap>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "App.h"
#include "hint.h"
#include "UiConstants.h"
#include "Widgets.h"
#include "../core/Runtime.h"
#include "../level/Level.h"
#include "../level/LevelConstants.h"
#include "../message/Message.h"
#include "SettingsDialog.h"
#include "parameter.h"
#include "MainWindow.h"
using std::vector;
using std::function;

QTimer* timerPtr = nullptr;
core::BlockExecutor* executorPtr = nullptr;
core::RuntimeState runtimeState;
QGraphicsScene* appScene = nullptr;
bool runtimeSkipCurrentBlock=false;
bool runtimeStopRequested=false;
bool programRunning=false;
bool levelTestRunning=false;
bool levelTestFailed=false;
int levelTestStepCount=0;
int levelTestTimeCount=0;
int levelTestCaseIndex=0;
int levelTestCaseTotal=1;
int levelNumberNow;
bool runtimeCountersActive=false;
bool stageExpanded=false;
bool editorExitToDesktopRequested=false;
bool contextMenuButtonPressed=false;
vector<QString> undoCheckpoints;
int undoCheckpointId=0;
bool restoringUndo=false;
bool suppressCustomReferenceRemoval=false;

class CodeBlock;
class FloatBlock;
class ControlCodeBlock;
class CustomHatBlock;
class CustomCallBlock;
class CustomParamBlock;
class StartBlock;
class EndBlock;
class VariableBlock;
class RobotCoordBlock;
class RobotFrontMapBlock;
class SetVariableBlock;
class IncreaseVariableBlock;
class OutputBlock;
class ListGetBlock;
class ListSizeBlock;
class PushListBlock;
class SetListBlock;
class RemoveListItemBlock;
class ClearListBlock;

vector<CodeBlock*> runtimeCodeRoots;
StartBlock* runtimeStartBlock=nullptr;
std::map<QString,CustomHatBlock*> runtimeCustomHatBlocks;
bool runtimeCodeSnapshotActive=false;
QString runtimeSnapshotError;
bool runtimeEndReached=false;
bool runtimeWaitActionFrame=false;

void recordRuntimeStepUse(){
    if(runtimeCountersActive){
        levelTestStepCount++;
    }
}

void recordRuntimeStepUse(int count){
    if(runtimeCountersActive&&count>0){
        levelTestStepCount+=count;
    }
}

level::TestContext currentLevelTestContext();
void finishLevelTest(bool forcedFail,const QString& message);
void clearRuntimeCodeSnapshot();
bool buildRuntimeCodeSnapshot();
void ensureBuiltInRuntimeVariables();
void syncMapDataFromActiveLevel();
void updateStageGeometry();
void stopProgram();
void installRuntimeCancelFilter();
void resetActiveLevelForRun();

void recordRuntimeTimeUse(){
    if(runtimeCountersActive){
        levelTestTimeCount++;
        if(level::activeLevelType()==level::LevelType::Map){
            runtimeState.forceSetVariable("time",levelTestTimeCount,true);
            level::FreshResult freshResult=level::fresh(currentLevelTestContext());
            runtimeWaitActionFrame=false;
            syncMapDataFromActiveLevel();
            updateStageGeometry();
            if(freshResult.reachedGoal){
                finishLevelTest(false,QString());
                return;
            }
            if(freshResult.trapped){
                finishLevelTest(true,QString::fromStdString(freshResult.trapMessage));
                return;
            }
        }
    }
}

void recordFloatOperatorUse(){
    recordRuntimeStepUse();
}

void waitRuntimeValueRead(){
    if(programRunning||levelTestRunning||runtimeCountersActive){
        QThread::msleep(1);
    }
}
void refreshFloatAncestors(FloatBlock* block);
void refreshAllControlLayouts();
void refreshVariableToolbox();
void updateToolboxScrollRange();
void checkEditedFloatWorkspaceWidth(FloatBlock* block);
void checkEditedCodeWorkspaceWidth(CodeBlock* block);
QRectF workspaceRect();
void clearContextMenu();
void showUndoContextMenu(QPointF scenePos);
void showCodeContextMenu(CodeBlock* block,QPointF scenePos);
void showFloatContextMenu(FloatBlock* block,QPointF scenePos);
bool isPlainNumberBlock(FloatBlock* block);
bool isPlaceholderFloatValue(FloatBlock* block);
bool isContextMenuItem(QGraphicsItem* item);
bool isWorkspaceContentItem(QGraphicsItem* item);
vector<CodeBlock*> workspaceCodeRootsFromScene();
vector<CodeBlock*> workspaceCodeItemsFromScene();
CodeBlock* codeBlockAtScenePoint(QGraphicsScene* scene,QPointF scenePoint);
FloatBlock* floatBlockAtScenePoint(QGraphicsScene* scene,QPointF scenePoint);
int runtimeIntervalForCodeBlock(CodeBlock* block);
void clearUndoCache();
void saveUndoCheckpoint();
void undoLastCheckpoint();
void rebuildFloatBlockRegistryFromScene();
void clearWorkspaceAndCacheOnExit();

constexpr qreal variableHorizontalPadding=6;

QColor variableColor(){
    return QColor(194,92,0);
}

QColor listColor(){
    return QColor(125,28,38);
}

QColor robotCoordColor(){
    return QColor(42,86,150);
}

QColor fileButtonColor(){
    return QColor(92,98,108);
}

QColor appBackgroundColor(){
    return QColor(34,38,44);
}

QColor panelBackgroundColor(){
    return QColor(45,50,58);
}

QPixmap loadImageAsset(const QString& fileName){
    QStringList roots;
    roots<<QDir::currentPath()
         <<QCoreApplication::applicationDirPath()
         <<QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("..")
         <<QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../..");
    QStringList imagePaths;
    imagePaths<<QString("images/%1").arg(fileName)
              <<QString("images/bars/%1").arg(fileName)
              <<QString("images/floor/%1").arg(fileName)
              <<QString("images/icons/%1").arg(fileName)
              <<QString("images/background/%1").arg(fileName);
    for(const QString& root:roots){
        for(const QString& relativePath:imagePaths){
            QString path=QDir(root).filePath(relativePath);
            if(QFileInfo::exists(path)){
                return QPixmap(path);
            }
        }
    }
    return QPixmap();
}

QPixmap loadPaintingAsset(const QString& fileName){
    QStringList roots;
    roots<<QDir::currentPath()
         <<QCoreApplication::applicationDirPath()
         <<QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("..")
         <<QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../..");
    for(const QString& root:roots){
        QString path=QDir(root).filePath(QString("painting/%1").arg(fileName));
        if(QFileInfo::exists(path)){
            return QPixmap(path);
        }
    }
    return QPixmap();
}




qreal scaledAssetHeight(const QString& fileName,qreal targetWidth,qreal fallbackHeight){
    QPixmap pixmap=loadImageAsset(fileName);
    if(pixmap.isNull()||pixmap.width()<=0){
        return fallbackHeight;
    }
    return std::round(targetWidth*pixmap.height()/double(pixmap.width()));
}

QPixmap trimTransparentPixmap(const QPixmap& pixmap){
    if(pixmap.isNull()){
        return pixmap;
    }
    QImage image=pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    int minX=image.width();
    int minY=image.height();
    int maxX=-1;
    int maxY=-1;
    for(int y=0;y<image.height();y++){
        const QRgb* line=reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for(int x=0;x<image.width();x++){
            if(qAlpha(line[x])>10){
                minX=std::min(minX,x);
                minY=std::min(minY,y);
                maxX=std::max(maxX,x);
                maxY=std::max(maxY,y);
            }
        }
    }
    if(maxX<minX||maxY<minY){
        return pixmap;
    }
    return pixmap.copy(QRect(minX,minY,maxX-minX+1,maxY-minY+1));
}

void addHeaderLogo(QGraphicsScene& scene){
    QPixmap logoPixmap=trimTransparentPixmap(loadImageAsset("logo-flatten.png"));
    if(!logoPixmap.isNull()){
        constexpr int logoHeight=48;
        int logoWidth=int(std::round(double(logoPixmap.width())*logoHeight/logoPixmap.height()));
        QGraphicsPixmapItem* logoImage=scene.addPixmap(
            logoPixmap.scaled(logoWidth,logoHeight,Qt::KeepAspectRatio,Qt::SmoothTransformation)
        );
        logoImage->setPos((appWidth-logoWidth)/2,16);
        logoImage->setZValue(topUiZ+1);
        logoImage->setAcceptedMouseButtons(Qt::NoButton);
        return;
    }

    QGraphicsTextItem* logoText=scene.addText("BlockBot:Factory");
    logoText->setDefaultTextColor(Qt::white);
    QRectF logoRect=logoText->boundingRect();
    logoText->setPos((appWidth-logoRect.width())/2,27);
    logoText->setZValue(topUiZ+1);
    logoText->setAcceptedMouseButtons(Qt::NoButton);
}

QString archiveDirectoryPath(){
    QString appData=qEnvironmentVariable("APPDATA");
    QDir dir(appData.isEmpty()?QDir::currentPath():appData);
    dir.mkpath("BlockBot");
    return dir.filePath("BlockBot");
}

QString archiveDefaultFilePath(){
    QDir dir(archiveDirectoryPath());
    return dir.filePath(QString("BlockBot_%1.json").arg(level::activeLevelNumber()));
}

bool validVariableName(const QString& name){
    QString trimmed=name.trimmed();
    if(trimmed.isEmpty()||trimmed.size()>10||trimmed!=name){
        return false;
    }
    for(QChar ch:trimmed){
        if(ch.isSpace()||ch.unicode()<32||ch.unicode()==127){
            return false;
        }
        if(!(ch.isLetterOrNumber()||ch=='_')){
            return false;
        }
    }
    return true;
}

double checkedDoubleResult(double value,const char* op){
    if(std::isfinite(value)){
        return value;
    }
    message::numericOutOfRange(op);
    if(value<0){
        return std::numeric_limits<double>::lowest();
    }
    return std::numeric_limits<double>::max();
}

class ClickTextItem:public QGraphicsTextItem{
public:
    std::function<void()> onClick;

    ClickTextItem(QGraphicsItem* parent=nullptr):QGraphicsTextItem(parent){
        setAcceptedMouseButtons(Qt::LeftButton);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override{
        if(event->button()!=Qt::LeftButton){
            event->ignore();
            return;
        }
        clearContextMenu();
        if(onClick){
            onClick();
        }
        event->accept();
    }
};

class TextButton:public QGraphicsPolygonItem{
public:
    QGraphicsTextItem* text;
    QGraphicsPixmapItem* texture;
    QPixmap texturePixmap;
    qreal buttonWidth;
    qreal buttonHeight;
    std::function<void()> onClick;

    TextButton(QString label,QGraphicsItem* parent=nullptr):QGraphicsPolygonItem(parent){
        texture=nullptr;
        buttonWidth=0;
        buttonHeight=0;
        text=new QGraphicsTextItem(label,this);
        text->document()->setDocumentMargin(0);
        text->setDefaultTextColor(Qt::white);
        text->setAcceptedMouseButtons(Qt::NoButton);
        text->setZValue(2);
        refreshShape();
        setBrush(QColor(80,120,170));
        setPen(QPen(Qt::black,1.5));
        setAcceptHoverEvents(true);
        setAcceptedMouseButtons(Qt::LeftButton);
    }

    void refreshShape(){
        QRectF rect=text->boundingRect();
        qreal width=rect.width()+20;
        qreal height=rect.height()+10;
        buttonWidth=width;
        buttonHeight=height;
        QPolygonF shape;
        shape<<QPointF(0,0)<<QPointF(width,0)
             <<QPointF(width,height)<<QPointF(0,height);
        setPolygon(shape);
        text->setPos(10,5);
        refreshTexture();
    }

    void setFixedSize(qreal width,qreal height){
        buttonWidth=width;
        buttonHeight=height;
        QPolygonF shape;
        shape<<QPointF(0,0)<<QPointF(width,0)
             <<QPointF(width,height)<<QPointF(0,height);
        setPolygon(shape);
        QRectF rect=text->boundingRect();
        text->setPos((width-rect.width())/2,(height-rect.height())/2);
        refreshTexture();
    }

    void setTexture(const QString& fileName){
        QPixmap pixmap=loadImageAsset(fileName);
        if(pixmap.isNull()){
            return;
        }
        texturePixmap=pixmap;
        if(texture==nullptr){
            texture=new QGraphicsPixmapItem(this);
            texture->setAcceptedMouseButtons(Qt::NoButton);
            texture->setZValue(1);
        }
        text->show();
        setBrush(Qt::NoBrush);
        setPen(Qt::NoPen);
        refreshTexture();
    }

    void refreshTexture(){
        if(texture==nullptr||texturePixmap.isNull()||buttonWidth<=0||buttonHeight<=0){
            return;
        }
        texture->setPixmap(texturePixmap.scaled(
            int(buttonWidth),
            int(buttonHeight),
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation
        ));
        texture->setPos(0,0);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override{
        if(onClick){
            onClick();
        }
        event->accept();
    }

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override{
        setCursor(Qt::PointingHandCursor);
        event->accept();
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override{
        unsetCursor();
        event->accept();
    }
};


class ContextMenuButton:public Button{
public:
    QGraphicsTextItem* label;

    ContextMenuButton(QString text,QGraphicsItem* parent=nullptr):Button(0,"",parent){
        label=new QGraphicsTextItem(text,this);
        label->document()->setDocumentMargin(0);
        label->setDefaultTextColor(Qt::white);
        label->setAcceptedMouseButtons(Qt::NoButton);
        QPolygonF shape;
        shape<<QPointF(0,0)<<QPointF(56,0)<<QPointF(56,24)<<QPointF(0,24);
        setPolygon(shape);
        setBrush(QColor(70,80,96));
        setPen(QPen(Qt::black,1.5));
        QRectF rect=label->boundingRect();
        label->setPos((56-rect.width())/2,(24-rect.height())/2);
    }
};

void editNameText(QString& name,QGraphicsTextItem* textItem,const QString& title,function<void()> onChanged){
    bool ok=false;
    QString newName=QInputDialog::getText(nullptr,title,"输入名称",
        QLineEdit::Normal,name,&ok).trimmed();
    if(!ok){
        return;
    }
    if(!validVariableName(newName)){
        message::invalidVariableName();
        return;
    }
    name=newName;
    if(textItem!=nullptr){
        textItem->setPlainText(name);
    }
    if(onChanged){
        onChanged();
    }
}

QPolygonF codeBlockPolygon(qreal len,qreal height,bool topSocket,bool bottomTab,
                           qreal connectorX=codeConnectorX){
    qreal connectorRight=connectorX+codeConnectorWidth;
    QPolygonF shape;
    shape<<QPointF(0,0);
    if(topSocket){
        shape<<QPointF(connectorX,0)
             <<QPointF(connectorX,codeConnectorHeight)
             <<QPointF(connectorRight,codeConnectorHeight)
             <<QPointF(connectorRight,0);
    }
    shape<<QPointF(len,0)
         <<QPointF(len,height);
    if(bottomTab){
        shape<<QPointF(connectorRight,height)
             <<QPointF(connectorRight,height+codeConnectorHeight)
             <<QPointF(connectorX,height+codeConnectorHeight)
             <<QPointF(connectorX,height);
    }
    shape<<QPointF(0,height);
    return shape;
}

QPolygonF codeBlockShadowPolygon(qreal len,qreal height,bool topSocket,bool bottomTab,
                                 qreal connectorX=codeConnectorX){
    return codeBlockPolygon(len,height,topSocket,bottomTab,connectorX);
}

void setCodeBlockShape(QGraphicsPolygonItem* block,QGraphicsPolygonItem* shadow,
                       qreal len,qreal height,bool topSocket,bool bottomTab,
                       qreal connectorX=codeConnectorX){
    block->setPolygon(codeBlockPolygon(len,height,topSocket,bottomTab,connectorX));
    if(shadow!=nullptr){
        shadow->setPolygon(codeBlockShadowPolygon(len,height,topSocket,bottomTab,connectorX));
    }
}

void setCodeShadowLikeBlock(QGraphicsPolygonItem* shadow,CodeBlock* block);

class CodeBlock:public QGraphicsPolygonItem{
public:
    int len;
    int wid;
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
    CodeBlock* preTarget;
    CodeBlock* nextTarget;
    ControlCodeBlock* insideParent;
    ControlCodeBlock* insideTarget;
    CodeBlock(int _type,QString ss,int base=false,
        QGraphicsItem * parent=nullptr):
            QGraphicsPolygonItem(parent){
        type=_type;
        QPolygonF shape;
        text=new QGraphicsTextItem(ss,this);
        text->setDefaultTextColor(Qt::white);
        text->setPos(10,10);
        len=text->boundingRect().width()+30;
        wid=40;
        s=ss;
        shape=codeBlockPolygon(len,wid,true,true);
        setPolygon(shape);
        setBrush(Qt::blue);
        setPen(QPen(Qt::black,1.5));
        shadow=new QGraphicsPolygonItem();
        shadow->setPolygon(codeBlockShadowPolygon(len,wid,true,true));
        shadow->setBrush(Qt::gray);
        shadow->setPen(Qt::NoPen);
        shadow->setPos(pos());
        dragging=false;
        setAcceptedMouseButtons(Qt::LeftButton|Qt::RightButton);
        mouseOffset=QPointF(0,0);
        pre=nullptr;
        next=nullptr;
        isbase=base;
        ismoving=false;
        scrollArea=scrollNone;
        stagePos=QPointF(0,0);
        blockOffset=QPointF(0,0);
        preTarget=nullptr;
        nextTarget=nullptr;
        insideParent=nullptr;
        insideTarget=nullptr;
    };
    virtual CodeBlock* copy(){
        CodeBlock* newBlock=new CodeBlock(type,s,false);
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
    int wid,len,type;
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
    FloatBlock(int _type,int base=false,
               QGraphicsItem * parent=nullptr):QGraphicsPolygonItem(parent){
        type=_type;
        s="";
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
        setAcceptedMouseButtons(Qt::LeftButton|Qt::RightButton);
    }
    virtual bool isOperator() const{
        return false;
    }
    virtual double getValue() const{
        return data;
    }
    virtual void editValue(){
        bool ok=false;
        double value=QInputDialog::getDouble(nullptr,"请输入一个数","数值",data,-1000000,1000000,6,&ok);
        if(ok){
            setData(value);
        }
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
                refreshAllControlLayouts();
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
        setAcceptedMouseButtons(movable?(Qt::LeftButton|Qt::RightButton):Qt::NoButton);
    }
    virtual FloatBlock* copy(){
        FloatBlock* newBlock=new FloatBlock(type,false);
        newBlock->setData(data);
        newBlock->setPos(pos());
        return newBlock;
    }
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
};

void setCodeShadowLikeBlock(QGraphicsPolygonItem* shadow,CodeBlock* block){
    if(shadow==nullptr||block==nullptr){
        return;
    }
    shadow->setPolygon(block->polygon());
}

class VariableBlock:public FloatBlock{
public:
    QString variableName;

    VariableBlock(QString name,int base=false,QGraphicsItem* parent=nullptr):
        FloatBlock(100,base,parent){
        variableName=name;
        text->setPlainText(variableName);
        refreshSize();
        setBrush(variableColor());
    }

    double getValue() const override{
        waitRuntimeValueRead();
        double value=0.0;
        std::string name=variableName.toStdString();
        if(!runtimeState.getVariable(name,&value)){
            QByteArray bytes=variableName.toUtf8();
            message::variableNotFound(bytes.constData());
            runtimeSkipCurrentBlock=true;
            runtimeStopRequested=true;
            return 0.0;
        }
        return value;
    }

    void setVariableName(const QString& name){
        variableName=name.trimmed();
        text->setPlainText(variableName);
        refreshSize();
        if(parentItem()!=nullptr){
            FloatBlock* parentBlock=dynamic_cast<FloatBlock*>(parentItem());
            if(parentBlock!=nullptr){
                refreshFloatAncestors(parentBlock);
            }
            CodeBlock* codeParent=dynamic_cast<CodeBlock*>(parentItem());
            if(codeParent!=nullptr){
                codeParent->refreshSize();
                refreshAllControlLayouts();
            }
        }
    }

    void refreshSize() override{
        updateShape(text->boundingRect().width()+variableHorizontalPadding*2);
        centerText();
    }

    void editValue() override{
        editNameText(variableName,text,"请输入变量名",[this](){
            setVariableName(variableName);
        });
    }

    FloatBlock* copy() override{
        VariableBlock* newBlock=new VariableBlock(variableName,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class UnaryOpBlock:public FloatBlock{
public:
    FloatBlock* slot;
    UnaryOpBlock(int _type,QString ss,FloatBlock* _slot=nullptr,
                 int base=false,QGraphicsItem * parent=nullptr):
        FloatBlock(_type,base,parent){
        s=ss;
        text->setPlainText(s);
        if(_slot==nullptr){
            slot=new FloatBlock(0,false,this);
        }
        else{
            slot=_slot->copy();
            slot->setParentItem(this);
        }
        slot->setMovable(!base);
        slot->setPos(0,0);
        refreshSize();
        setBrush(QColor(42,105,86));
    }
    bool isOperator() const override{
        return true;
    }
    double getValue() const override{
        recordFloatOperatorUse();
        double val=slot->getValue();
        if(type==0){
            return checkedDoubleResult(std::sin(val),"sin");
        }
        if(type==1){
            return checkedDoubleResult(std::cos(val),"cos");
        }
        if(type==2){
            return checkedDoubleResult(std::tan(val),"tan");
        }
        if(type==3){
            if(val<-1.0-EPS||val>1.0+EPS){
                message::inverseTrigOutOfRange("asin",val);
                return 0.0;
            }
            val=std::max(-1.0,std::min(1.0,val));
            return checkedDoubleResult(std::asin(val),"asin");
        }
        if(type==4){
            if(val<-1.0-EPS||val>1.0+EPS){
                message::inverseTrigOutOfRange("acos",val);
                return 0.0;
            }
            val=std::max(-1.0,std::min(1.0,val));
            return checkedDoubleResult(std::acos(val),"acos");
        }
        if(type==5){
            return checkedDoubleResult(std::atan(val),"atan");
        }
        if(type==6){
            return checkedDoubleResult(std::log(val),"ln");
        }
        if(type==7){
            return checkedDoubleResult(std::log10(val),"log10");
        }
        if(type==8){
            return checkedDoubleResult(std::floor(val),"floor");
        }
        if(type==9){
            return checkedDoubleResult(std::abs(val),"abs");
        }
        if(type==10){
            return std::abs(val)<EPS?1.0:0.0;
        }
        return 0.0;
    }
    void refreshSize(){
        text->setDefaultTextColor(Qt::white);
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        qreal wanted=opHorizontalPadding+textWidth+opHorizontalPadding+slot->len+opHorizontalPadding;
        updateShape(wanted);
        wid=std::max(slot->wid,floatBlockWidth)+opVerticalPadding*2;
        updatePolygon();
        text->setPos(opHorizontalPadding,(wid-textHeight)/2);
        slot->setPos(opHorizontalPadding+textWidth+opHorizontalPadding,(wid-slot->wid)/2);
        setPen(QPen(Qt::black,1.5));
    }
    FloatBlock* copy() override{
        UnaryOpBlock* newBlock=new UnaryOpBlock(type,s,slot,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class BinaryOpBlock:public FloatBlock{
public:
    FloatBlock* left;
    FloatBlock* right;
    BinaryOpBlock(int _type,QString ss,FloatBlock* _left=nullptr,
                  FloatBlock* _right=nullptr,int base=false,
                  QGraphicsItem * parent=nullptr):
        FloatBlock(_type,base,parent){
        s=ss;
        text->setPlainText(s);
        if(_left==nullptr){
            left=new FloatBlock(0,false,this);
        }
        else{
            left=_left->copy();
            left->setParentItem(this);
        }
        if(_right==nullptr){
            right=new FloatBlock(0,false,this);
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
        recordFloatOperatorUse();
        double a=left->getValue();
        double b=right->getValue();
        if(type==0){
            return checkedDoubleResult(a+b,"addition");
        }
        if(type==1){
            return checkedDoubleResult(a-b,"subtraction");
        }
        if(type==2){
            return checkedDoubleResult(a*b,"multiplication");
        }
        if(type==3){
            if(std::abs(b)<EPS){
                message::divisionByZero();
                return 0.0;
            }
            return checkedDoubleResult(a/b,"division");
        }
        if(type==4){
            return checkedDoubleResult(std::pow(a,b),"pow");
        }
        if(type==5){
            return checkedDoubleResult(std::atan2(b,a),"arg");
        }
        if(type==6){
            return a>b?a:b;
        }
        if(type==7){
            return a<b?a:b;
        }
        if(type==8){
            return std::abs(a-b)<EPS?1.0:0.0;
        }
        if(type==9){
            return std::abs(a-b)>=EPS?1.0:0.0;
        }
        if(type==10){
            return a<b-EPS?1.0:0.0;
        }
        if(type==11){
            return a>b+EPS?1.0:0.0;
        }
        if(type==12){
            return (std::abs(a)>=EPS&&std::abs(b)>=EPS)?1.0:0.0;
        }
        if(type==13){
            return (std::abs(a)>=EPS||std::abs(b)>=EPS)?1.0:0.0;
        }
        return 0.0;
    }
    void refreshSize(){
        text->setDefaultTextColor(Qt::white);
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        bool prefixLayout=(type>=5&&type<=7);
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
        BinaryOpBlock* newBlock=new BinaryOpBlock(type,s,left,right,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class FloatCodeBlock:public CodeBlock{
public:
    FloatBlock* value;
    QGraphicsTextItem* suffixText;
    FloatCodeBlock(int _type,QString ss,FloatBlock* _value=nullptr,
                   int base=false,QGraphicsItem * parent=nullptr):
        CodeBlock(_type,ss,base,parent){
        suffixText=new QGraphicsTextItem(this);
        suffixText->document()->setDocumentMargin(0);
        suffixText->setDefaultTextColor(Qt::white);
        suffixText->setAcceptedMouseButtons(Qt::NoButton);
        if(type==3){
            suffixText->setPlainText("步");
        }
        else if(type==4){
            suffixText->setPlainText("帧");
        }
        if(_value==nullptr){
            value=new FloatBlock(0,false,this);
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
        qreal suffixWidth=suffixText->boundingRect().width();
        qreal suffixHeight=suffixText->boundingRect().height();
        wid=std::max(40,value->wid+10);
        len=static_cast<int>(std::ceil(20+textWidth+10+value->len+10+suffixWidth+10));
        setCodeBlockShape(this,shadow,len,wid,true,true);
        text->setPos(10,(wid-textHeight)/2);
        value->setPos(20+textWidth,(wid-value->wid)/2);
        suffixText->setPos(20+textWidth+value->len+10,(wid-suffixHeight)/2);
    }
    CodeBlock* copy() override{
        FloatCodeBlock* newBlock=new FloatCodeBlock(type,s,value,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class OutputBlock:public CodeBlock{
public:
    QString messageText;
    ClickTextItem* messageItem;
    QGraphicsRectItem* messageFrame;
    FloatBlock* value;

    OutputBlock(QString textValue="x",FloatBlock* _value=nullptr,
                int base=false,QGraphicsItem* parent=nullptr):
        CodeBlock(12,"输出",base,parent){
        messageText=textValue;
        messageFrame=new QGraphicsRectItem(this);
        messageFrame->setBrush(QColor(220,220,220));
        messageFrame->setPen(QPen(Qt::black,1.5));
        messageFrame->setAcceptedMouseButtons(Qt::NoButton);
        messageFrame->setZValue(1);
        messageItem=new ClickTextItem(this);
        messageItem->setDefaultTextColor(Qt::black);
        messageItem->document()->setDocumentMargin(0);
        messageItem->setPlainText(messageText);
        messageItem->setZValue(2);
        if(!base){
            messageItem->onClick=[this](){
                bool ok=false;
                QString newText=QInputDialog::getText(nullptr,"输入输出文字","输入文字",
                    QLineEdit::Normal,messageText,&ok);
                if(!ok){
                    return;
                }
                messageText=newText;
                messageItem->setPlainText(messageText);
                refreshSize();
                refreshAllControlLayouts();
                checkEditedCodeWorkspaceWidth(this);
            };
        }
        else{
            messageItem->setAcceptedMouseButtons(Qt::NoButton);
        }
        if(_value==nullptr){
            value=new FloatBlock(0,false,this);
        }
        else{
            value=_value->copy();
            value->setParentItem(this);
        }
        value->setMovable(!base);
        setBrush(QColor(130,76,180));
        refreshSize();
    }

    void refreshSize() override{
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        qreal messageWidth=messageItem->boundingRect().width();
        qreal messageHeight=messageItem->boundingRect().height();
        qreal messageBoxWidth=messageWidth+variableHorizontalPadding*2;
        qreal messageBoxHeight=std::max<qreal>(floatBlockWidth,messageHeight+6);
        wid=std::max(40,value->wid+10);
        len=static_cast<int>(std::ceil(20+textWidth+10+messageBoxWidth+10+value->len+10));
        setCodeBlockShape(this,shadow,len,wid,true,true);
        text->setPos(10,(wid-textHeight)/2);
        qreal messageBoxX=20+textWidth;
        qreal messageBoxY=(wid-messageBoxHeight)/2;
        messageFrame->setRect(0,0,messageBoxWidth,messageBoxHeight);
        messageFrame->setPos(messageBoxX,messageBoxY);
        messageItem->setPos(messageBoxX+variableHorizontalPadding,(wid-messageHeight)/2);
        value->setPos(messageBoxX+messageBoxWidth+10,(wid-value->wid)/2);

    }

    CodeBlock* copy() override{
        OutputBlock* newBlock=new OutputBlock(messageText,value,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class SetVariableBlock:public CodeBlock{
public:
    QString variableName;
    ClickTextItem* variableText;
    QGraphicsRectItem* variableFrame;
    QGraphicsTextItem* suffixText;
    FloatBlock* value;

    SetVariableBlock(QString name="x",FloatBlock* _value=nullptr,
                     int base=false,QGraphicsItem* parent=nullptr):
        CodeBlock(7,"将变量",base,parent){
        variableName=name;
        variableFrame=new QGraphicsRectItem(this);
        variableFrame->setBrush(QColor(220,220,220));
        variableFrame->setPen(QPen(Qt::black,1.5));
        variableFrame->setAcceptedMouseButtons(Qt::NoButton);
        variableFrame->setZValue(1);
        variableText=new ClickTextItem(this);
        variableText->setDefaultTextColor(Qt::black);
        variableText->document()->setDocumentMargin(0);
        variableText->setPlainText(variableName);
        variableText->setZValue(2);
        suffixText=new QGraphicsTextItem("设置为",this);
        suffixText->document()->setDocumentMargin(0);
        suffixText->setDefaultTextColor(Qt::white);
        suffixText->setAcceptedMouseButtons(Qt::NoButton);
        if(!base){
            variableText->onClick=[this](){
                editNameText(variableName,variableText,"请输入变量名称",[this](){
                    refreshSize();
                    refreshAllControlLayouts();
                    checkEditedCodeWorkspaceWidth(this);
                });
            };
        }
        else{
            variableText->setAcceptedMouseButtons(Qt::NoButton);
        }
        if(_value==nullptr){
            value=new FloatBlock(0,false,this);
        }
        else{
            value=_value->copy();
            value->setParentItem(this);
        }
        value->setMovable(!base);
        setBrush(variableColor());
        refreshSize();
    }

    void refreshSize() override{
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        qreal variableWidth=variableText->boundingRect().width();
        qreal variableHeight=variableText->boundingRect().height();
        qreal suffixWidth=suffixText->boundingRect().width();
        qreal suffixHeight=suffixText->boundingRect().height();
        qreal variableBoxWidth=variableWidth+variableHorizontalPadding*2;
        qreal variableBoxHeight=std::max<qreal>(floatBlockWidth,variableHeight+6);
        wid=std::max(40,value->wid+10);
        len=static_cast<int>(std::ceil(20+textWidth+10+variableBoxWidth+10+
            suffixWidth+10+value->len+10));
        setCodeBlockShape(this,shadow,len,wid,true,true);
        text->setPos(10,(wid-textHeight)/2);
        qreal variableBoxX=20+textWidth;
        qreal variableBoxY=(wid-variableBoxHeight)/2;
        variableFrame->setRect(0,0,variableBoxWidth,variableBoxHeight);
        variableFrame->setPos(variableBoxX,variableBoxY);
        variableText->setPos(variableBoxX+variableHorizontalPadding,(wid-variableHeight)/2);
        suffixText->setPos(variableBoxX+variableBoxWidth+10,(wid-suffixHeight)/2);
        value->setPos(variableBoxX+variableBoxWidth+10+suffixWidth+10,(wid-value->wid)/2);

    }

    CodeBlock* copy() override{
        SetVariableBlock* newBlock=new SetVariableBlock(variableName,value,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class IncreaseVariableBlock:public SetVariableBlock{
public:
    IncreaseVariableBlock(QString name="x",FloatBlock* _value=nullptr,
                          int base=false,QGraphicsItem* parent=nullptr):
        SetVariableBlock(name,_value,base,parent){
        type=14;
        suffixText->setPlainText(QString::fromUtf8("增加"));
        refreshSize();
    }

    CodeBlock* copy() override{
        IncreaseVariableBlock* newBlock=new IncreaseVariableBlock(variableName,value,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class ListGetBlock:public FloatBlock{
public:
    QString listName;
    ClickTextItem* listText;
    QGraphicsRectItem* listFrame;
    QGraphicsTextItem* infixText;
    QGraphicsTextItem* suffixText;
    FloatBlock* index;

    ListGetBlock(QString name,FloatBlock* _index=nullptr,
                 int base=false,QGraphicsItem* parent=nullptr):
        FloatBlock(101,base,parent){
        listName=name;
        s="列表";
        text->setPlainText(s);
        listFrame=new QGraphicsRectItem(this);
        listFrame->setBrush(QColor(220,220,220));
        listFrame->setPen(QPen(Qt::black,1.5));
        listFrame->setAcceptedMouseButtons(Qt::NoButton);
        listFrame->setZValue(1);
        listText=new ClickTextItem(this);
        listText->setDefaultTextColor(Qt::black);
        listText->document()->setDocumentMargin(0);
        listText->setPlainText(listName);
        listText->setZValue(2);
        infixText=new QGraphicsTextItem("的第",this);
        infixText->document()->setDocumentMargin(0);
        infixText->setDefaultTextColor(Qt::white);
        infixText->setAcceptedMouseButtons(Qt::NoButton);
        suffixText=new QGraphicsTextItem("项",this);
        suffixText->document()->setDocumentMargin(0);
        suffixText->setDefaultTextColor(Qt::white);
        suffixText->setAcceptedMouseButtons(Qt::NoButton);
        if(!base){
            listText->onClick=[this](){
                editNameText(listName,listText,"请输入列表名称",[this](){
                    refreshSize();
                    refreshFloatAncestors(this);
                    checkEditedFloatWorkspaceWidth(this);
                });
            };
        }
        else{
            listText->setAcceptedMouseButtons(Qt::NoButton);
        }
        if(_index==nullptr){
            index=new FloatBlock(0,false,this);
        }
        else{
            index=_index->copy();
            index->setParentItem(this);
        }
        index->setMovable(!base);
        refreshSize();
        setBrush(listColor());
    }

    bool isOperator() const override{
        return true;
    }

    double getValue() const override{
        std::string name=listName.toStdString();
        if(!runtimeState.hasList(name)){
            QByteArray bytes=listName.toUtf8();
            message::listNotFound(bytes.constData());
            runtimeStopRequested=true;
            return 0.0;
        }
        int userIndex=static_cast<int>(std::floor(index->getValue()));
        int idx=userIndex-1;
        double value=0.0;
        if(!runtimeState.getListValue(name,idx,&value)){
            QByteArray bytes=listName.toUtf8();
            message::listIndexOutOfRange(bytes.constData(),userIndex);
            runtimeStopRequested=true;
            return 0.0;
        }
        return value;
    }

    void refreshSize() override{
        text->setDefaultTextColor(Qt::white);
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        qreal nameWidth=listText->boundingRect().width();
        qreal nameHeight=listText->boundingRect().height();
        qreal infixWidth=infixText->boundingRect().width();
        qreal infixHeight=infixText->boundingRect().height();
        qreal suffixWidth=suffixText->boundingRect().width();
        qreal suffixHeight=suffixText->boundingRect().height();
        qreal nameBoxWidth=nameWidth+variableHorizontalPadding*2;
        qreal nameBoxHeight=std::max<qreal>(floatBlockWidth,nameHeight+6);
        qreal wanted=opHorizontalPadding+textWidth+opHorizontalPadding+
                     nameBoxWidth+opHorizontalPadding+infixWidth+opHorizontalPadding+
                     index->len+opHorizontalPadding+suffixWidth+opHorizontalPadding;
        updateShape(wanted);
        wid=std::max(std::max(index->wid,floatBlockWidth),static_cast<int>(nameBoxHeight))+opVerticalPadding*2;
        updatePolygon();
        text->setPos(opHorizontalPadding,(wid-textHeight)/2);
        qreal boxX=opHorizontalPadding+textWidth+opHorizontalPadding;
        qreal boxY=(wid-nameBoxHeight)/2;
        listFrame->setRect(0,0,nameBoxWidth,nameBoxHeight);
        listFrame->setPos(boxX,boxY);
        listText->setPos(boxX+variableHorizontalPadding,(wid-nameHeight)/2);
        qreal infixX=boxX+nameBoxWidth+opHorizontalPadding;
        infixText->setPos(infixX,(wid-infixHeight)/2);
        qreal indexX=infixX+infixWidth+opHorizontalPadding;
        index->setPos(indexX,(wid-index->wid)/2);
        suffixText->setPos(indexX+index->len+opHorizontalPadding,(wid-suffixHeight)/2);
        setPen(QPen(Qt::black,1.5));
    }

    FloatBlock* copy() override{
        ListGetBlock* newBlock=new ListGetBlock(listName,index,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class ListSizeBlock:public FloatBlock{
public:
    QString listName;
    ClickTextItem* listText;
    QGraphicsRectItem* listFrame;
    QGraphicsTextItem* suffixText;

    ListSizeBlock(QString name,int base=false,QGraphicsItem* parent=nullptr):
        FloatBlock(104,base,parent){
        listName=name;
        s="列表";
        text->setPlainText(s);
        listFrame=new QGraphicsRectItem(this);
        listFrame->setBrush(QColor(220,220,220));
        listFrame->setPen(QPen(Qt::black,1.5));
        listFrame->setAcceptedMouseButtons(Qt::NoButton);
        listFrame->setZValue(1);
        listText=new ClickTextItem(this);
        listText->setDefaultTextColor(Qt::black);
        listText->document()->setDocumentMargin(0);
        listText->setPlainText(listName);
        listText->setZValue(2);
        suffixText=new QGraphicsTextItem("的长度",this);
        suffixText->document()->setDocumentMargin(0);
        suffixText->setDefaultTextColor(Qt::white);
        suffixText->setAcceptedMouseButtons(Qt::NoButton);
        if(!base){
            listText->onClick=[this](){
                editNameText(listName,listText,"请输入列表名称",[this](){
                    refreshSize();
                    refreshFloatAncestors(this);
                    checkEditedFloatWorkspaceWidth(this);
                });
            };
        }
        else{
            listText->setAcceptedMouseButtons(Qt::NoButton);
        }
        refreshSize();
        setBrush(listColor());
    }

    double getValue() const override{
        std::string name=listName.toStdString();
        int size=runtimeState.listSize(name);
        if(size<0){
            QByteArray bytes=listName.toUtf8();
            message::listNotFound(bytes.constData());
            runtimeStopRequested=true;
            return 0.0;
        }
        return size;
    }

    void refreshSize() override{
        text->setDefaultTextColor(Qt::white);
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        qreal nameWidth=listText->boundingRect().width();
        qreal nameHeight=listText->boundingRect().height();
        qreal suffixWidth=suffixText->boundingRect().width();
        qreal suffixHeight=suffixText->boundingRect().height();
        qreal nameBoxWidth=nameWidth+variableHorizontalPadding*2;
        qreal nameBoxHeight=std::max<qreal>(floatBlockWidth,nameHeight+6);
        qreal wanted=opHorizontalPadding+textWidth+opHorizontalPadding+
                     nameBoxWidth+opHorizontalPadding+suffixWidth+opHorizontalPadding;
        updateShape(wanted);
        wid=std::max(floatBlockWidth,static_cast<int>(nameBoxHeight))+opVerticalPadding*2;
        updatePolygon();
        text->setPos(opHorizontalPadding,(wid-textHeight)/2);
        qreal boxX=opHorizontalPadding+textWidth+opHorizontalPadding;
        qreal boxY=(wid-nameBoxHeight)/2;
        listFrame->setRect(0,0,nameBoxWidth,nameBoxHeight);
        listFrame->setPos(boxX,boxY);
        listText->setPos(boxX+variableHorizontalPadding,(wid-nameHeight)/2);
        suffixText->setPos(boxX+nameBoxWidth+opHorizontalPadding,(wid-suffixHeight)/2);
        setPen(QPen(Qt::black,1.5));
    }

    void editValue() override{
    }

    FloatBlock* copy() override{
        ListSizeBlock* newBlock=new ListSizeBlock(listName,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class PushListBlock:public CodeBlock{
public:
    QString listName;
    ClickTextItem* listText;
    QGraphicsRectItem* listFrame;
    QGraphicsTextItem* suffixText;
    FloatBlock* value;

    PushListBlock(QString name="x",FloatBlock* _value=nullptr,
                  int base=false,QGraphicsItem* parent=nullptr):
        CodeBlock(8,"在列表",base,parent){
        listName=name;
        listFrame=new QGraphicsRectItem(this);
        listFrame->setBrush(QColor(220,220,220));
        listFrame->setPen(QPen(Qt::black,1.5));
        listFrame->setAcceptedMouseButtons(Qt::NoButton);
        listFrame->setZValue(1);
        listText=new ClickTextItem(this);
        listText->setDefaultTextColor(Qt::black);
        listText->document()->setDocumentMargin(0);
        listText->setPlainText(listName);
        listText->setZValue(2);
        suffixText=new QGraphicsTextItem("末尾添加",this);
        suffixText->document()->setDocumentMargin(0);
        suffixText->setDefaultTextColor(Qt::white);
        suffixText->setAcceptedMouseButtons(Qt::NoButton);
        if(!base){
            listText->onClick=[this](){
                editNameText(listName,listText,"请输入列表名称",[this](){
                    refreshSize();
                    refreshAllControlLayouts();
                    checkEditedCodeWorkspaceWidth(this);
                });
            };
        }
        else{
            listText->setAcceptedMouseButtons(Qt::NoButton);
        }
        value=_value==nullptr?new FloatBlock(0,false,this):_value->copy();
        value->setParentItem(this);
        value->setMovable(!base);
        setBrush(listColor());
        refreshSize();
    }

    void refreshSize() override{
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        qreal nameWidth=listText->boundingRect().width();
        qreal nameHeight=listText->boundingRect().height();
        qreal suffixWidth=suffixText->boundingRect().width();
        qreal suffixHeight=suffixText->boundingRect().height();
        qreal nameBoxWidth=nameWidth+variableHorizontalPadding*2;
        qreal nameBoxHeight=std::max<qreal>(floatBlockWidth,nameHeight+6);
        wid=std::max(40,value->wid+10);
        len=static_cast<int>(std::ceil(20+textWidth+10+nameBoxWidth+10+
            suffixWidth+10+value->len+10));
        setCodeBlockShape(this,shadow,len,wid,true,true);
        text->setPos(10,(wid-textHeight)/2);
        qreal boxX=20+textWidth;
        qreal boxY=(wid-nameBoxHeight)/2;
        listFrame->setRect(0,0,nameBoxWidth,nameBoxHeight);
        listFrame->setPos(boxX,boxY);
        listText->setPos(boxX+variableHorizontalPadding,(wid-nameHeight)/2);
        suffixText->setPos(boxX+nameBoxWidth+10,(wid-suffixHeight)/2);
        value->setPos(boxX+nameBoxWidth+10+suffixWidth+10,(wid-value->wid)/2);

    }

    CodeBlock* copy() override{
        PushListBlock* newBlock=new PushListBlock(listName,value,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class SetListBlock:public CodeBlock{
public:
    QString listName;
    ClickTextItem* listText;
    QGraphicsRectItem* listFrame;
    QGraphicsTextItem* infixText;
    QGraphicsTextItem* suffixText;
    FloatBlock* index;
    FloatBlock* value;

    SetListBlock(QString name="x",FloatBlock* _index=nullptr,
                 FloatBlock* _value=nullptr,int base=false,QGraphicsItem* parent=nullptr):
        CodeBlock(9,"将列表",base,parent){
        listName=name;
        listFrame=new QGraphicsRectItem(this);
        listFrame->setBrush(QColor(220,220,220));
        listFrame->setPen(QPen(Qt::black,1.5));
        listFrame->setAcceptedMouseButtons(Qt::NoButton);
        listFrame->setZValue(1);
        listText=new ClickTextItem(this);
        listText->setDefaultTextColor(Qt::black);
        listText->document()->setDocumentMargin(0);
        listText->setPlainText(listName);
        listText->setZValue(2);
        infixText=new QGraphicsTextItem("的第",this);
        infixText->document()->setDocumentMargin(0);
        infixText->setDefaultTextColor(Qt::white);
        infixText->setAcceptedMouseButtons(Qt::NoButton);
        suffixText=new QGraphicsTextItem("项设置为",this);
        suffixText->document()->setDocumentMargin(0);
        suffixText->setDefaultTextColor(Qt::white);
        suffixText->setAcceptedMouseButtons(Qt::NoButton);
        if(!base){
            listText->onClick=[this](){
                editNameText(listName,listText,"请输入列表名称",[this](){
                    refreshSize();
                    refreshAllControlLayouts();
                    checkEditedCodeWorkspaceWidth(this);
                });
            };
        }
        else{
            listText->setAcceptedMouseButtons(Qt::NoButton);
        }
        index=_index==nullptr?new FloatBlock(0,false,this):_index->copy();
        value=_value==nullptr?new FloatBlock(0,false,this):_value->copy();
        index->setParentItem(this);
        value->setParentItem(this);
        index->setMovable(!base);
        value->setMovable(!base);
        setBrush(listColor());
        refreshSize();
    }

    void refreshSize() override{
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        qreal nameWidth=listText->boundingRect().width();
        qreal nameHeight=listText->boundingRect().height();
        qreal infixWidth=infixText->boundingRect().width();
        qreal infixHeight=infixText->boundingRect().height();
        qreal suffixWidth=suffixText->boundingRect().width();
        qreal suffixHeight=suffixText->boundingRect().height();
        qreal nameBoxWidth=nameWidth+variableHorizontalPadding*2;
        qreal nameBoxHeight=std::max<qreal>(floatBlockWidth,nameHeight+6);
        wid=std::max(40,std::max(index->wid,value->wid)+10);
        len=static_cast<int>(std::ceil(20+textWidth+10+nameBoxWidth+10+
            infixWidth+10+index->len+10+suffixWidth+10+value->len+10));
        setCodeBlockShape(this,shadow,len,wid,true,true);
        text->setPos(10,(wid-textHeight)/2);
        qreal boxX=20+textWidth;
        qreal boxY=(wid-nameBoxHeight)/2;
        listFrame->setRect(0,0,nameBoxWidth,nameBoxHeight);
        listFrame->setPos(boxX,boxY);
        listText->setPos(boxX+variableHorizontalPadding,(wid-nameHeight)/2);
        qreal infixX=boxX+nameBoxWidth+10;
        infixText->setPos(infixX,(wid-infixHeight)/2);
        qreal indexX=infixX+infixWidth+10;
        index->setPos(indexX,(wid-index->wid)/2);
        qreal suffixX=indexX+index->len+10;
        suffixText->setPos(suffixX,(wid-suffixHeight)/2);
        value->setPos(suffixX+suffixWidth+10,(wid-value->wid)/2);

    }

    CodeBlock* copy() override{
        SetListBlock* newBlock=new SetListBlock(listName,index,value,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class RemoveListItemBlock:public CodeBlock{
public:
    QString listName;
    ClickTextItem* listText;
    QGraphicsRectItem* listFrame;
    QGraphicsTextItem* infixText;
    QGraphicsTextItem* suffixText;
    FloatBlock* index;

    RemoveListItemBlock(QString name="x",FloatBlock* _index=nullptr,
                        int base=false,QGraphicsItem* parent=nullptr):
        CodeBlock(13,QString::fromUtf8("删除列表"),base,parent){
        listName=name;
        listFrame=new QGraphicsRectItem(this);
        listFrame->setBrush(QColor(220,220,220));
        listFrame->setPen(QPen(Qt::black,1.5));
        listFrame->setAcceptedMouseButtons(Qt::NoButton);
        listFrame->setZValue(1);
        listText=new ClickTextItem(this);
        listText->setDefaultTextColor(Qt::black);
        listText->document()->setDocumentMargin(0);
        listText->setPlainText(listName);
        listText->setZValue(2);
        infixText=new QGraphicsTextItem(QString::fromUtf8("的第"),this);
        infixText->document()->setDocumentMargin(0);
        infixText->setDefaultTextColor(Qt::white);
        infixText->setAcceptedMouseButtons(Qt::NoButton);
        suffixText=new QGraphicsTextItem(QString::fromUtf8("项"),this);
        suffixText->document()->setDocumentMargin(0);
        suffixText->setDefaultTextColor(Qt::white);
        suffixText->setAcceptedMouseButtons(Qt::NoButton);
        if(!base){
            listText->onClick=[this](){
                editNameText(listName,listText,QString::fromUtf8("请输入列表名称"),[this](){
                    refreshSize();
                    refreshAllControlLayouts();
                    checkEditedCodeWorkspaceWidth(this);
                });
            };
        }
        else{
            listText->setAcceptedMouseButtons(Qt::NoButton);
        }
        index=_index==nullptr?new FloatBlock(0,false,this):_index->copy();
        index->setParentItem(this);
        index->setMovable(!base);
        setBrush(listColor());
        refreshSize();
    }

    void refreshSize() override{
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        qreal nameWidth=listText->boundingRect().width();
        qreal nameHeight=listText->boundingRect().height();
        qreal infixWidth=infixText->boundingRect().width();
        qreal infixHeight=infixText->boundingRect().height();
        qreal suffixWidth=suffixText->boundingRect().width();
        qreal suffixHeight=suffixText->boundingRect().height();
        qreal nameBoxWidth=nameWidth+variableHorizontalPadding*2;
        qreal nameBoxHeight=std::max<qreal>(floatBlockWidth,nameHeight+6);
        wid=std::max(40,index->wid+10);
        len=static_cast<int>(std::ceil(20+textWidth+10+nameBoxWidth+10+
            infixWidth+10+index->len+10+suffixWidth+10));
        setCodeBlockShape(this,shadow,len,wid,true,true);
        text->setPos(10,(wid-textHeight)/2);
        qreal boxX=20+textWidth;
        qreal boxY=(wid-nameBoxHeight)/2;
        listFrame->setRect(0,0,nameBoxWidth,nameBoxHeight);
        listFrame->setPos(boxX,boxY);
        listText->setPos(boxX+variableHorizontalPadding,(wid-nameHeight)/2);
        qreal infixX=boxX+nameBoxWidth+10;
        infixText->setPos(infixX,(wid-infixHeight)/2);
        qreal indexX=infixX+infixWidth+10;
        index->setPos(indexX,(wid-index->wid)/2);
        suffixText->setPos(indexX+index->len+10,(wid-suffixHeight)/2);

    }

    CodeBlock* copy() override{
        RemoveListItemBlock* newBlock=new RemoveListItemBlock(listName,index,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class ClearListBlock:public CodeBlock{
public:
    QString listName;
    ClickTextItem* listText;
    QGraphicsRectItem* listFrame;

    ClearListBlock(QString name="x",int base=false,QGraphicsItem* parent=nullptr):
        CodeBlock(10,"清空列表",base,parent){
        listName=name;
        listFrame=new QGraphicsRectItem(this);
        listFrame->setBrush(QColor(220,220,220));
        listFrame->setPen(QPen(Qt::black,1.5));
        listFrame->setAcceptedMouseButtons(Qt::NoButton);
        listFrame->setZValue(1);
        listText=new ClickTextItem(this);
        listText->setDefaultTextColor(Qt::black);
        listText->document()->setDocumentMargin(0);
        listText->setPlainText(listName);
        listText->setZValue(2);
        if(!base){
            listText->onClick=[this](){
                editNameText(listName,listText,"请输入列表名称",[this](){
                    refreshSize();
                    refreshAllControlLayouts();
                    checkEditedCodeWorkspaceWidth(this);
                });
            };
        }
        else{
            listText->setAcceptedMouseButtons(Qt::NoButton);
        }
        setBrush(listColor());
        refreshSize();
    }

    void refreshSize() override{
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        qreal nameWidth=listText->boundingRect().width();
        qreal nameHeight=listText->boundingRect().height();
        qreal nameBoxWidth=nameWidth+variableHorizontalPadding*2;
        qreal nameBoxHeight=std::max<qreal>(floatBlockWidth,nameHeight+6);
        wid=40;
        len=static_cast<int>(std::ceil(20+textWidth+10+nameBoxWidth+10));
        setCodeBlockShape(this,shadow,len,wid,true,true);
        text->setPos(10,(wid-textHeight)/2);
        qreal boxX=20+textWidth;
        qreal boxY=(wid-nameBoxHeight)/2;
        listFrame->setRect(0,0,nameBoxWidth,nameBoxHeight);
        listFrame->setPos(boxX,boxY);
        listText->setPos(boxX+variableHorizontalPadding,(wid-nameHeight)/2);

    }

    CodeBlock* copy() override{
        ClearListBlock* newBlock=new ClearListBlock(listName,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class ControlCodeBlock:public CodeBlock{
public:
    int topHeight;
    int innerHeight;
    int bottomHeight;
    int leftWidth;
    FloatBlock* condition;
    QGraphicsTextItem* suffixText;
    CodeBlock* inside;

    ControlCodeBlock(int _type,QString ss,FloatBlock* _condition=nullptr,
                     int base=false,QGraphicsItem* parent=nullptr):
        CodeBlock(_type,ss,base,parent){
        topHeight=30;
        innerHeight=30;
        bottomHeight=20;
        leftWidth=static_cast<int>(codeInsideLeftWidth);
        inside=nullptr;
        if(_condition==nullptr){
            condition=new FloatBlock(0,false,this);
        }
        else{
            condition=_condition->copy();
            condition->setParentItem(this);
        }
        suffixText=new QGraphicsTextItem(this);
        suffixText->document()->setDocumentMargin(0);
        suffixText->setDefaultTextColor(Qt::white);
        suffixText->setAcceptedMouseButtons(Qt::NoButton);
        if(type==5){
            suffixText->setPlainText("执行");
        }
        else if(type==6){
            suffixText->setPlainText("时重复执行");
        }
        condition->setMovable(!base);
        setBrush(QColor(54,92,122));
        refreshSize();
    }

    void refreshSize() override{
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        qreal suffixWidth=suffixText->boundingRect().width();
        qreal suffixHeight=suffixText->boundingRect().height();
        topHeight=std::max(30,condition->wid+10);
        len=static_cast<int>(std::ceil(std::max<qreal>(
            90,20+textWidth+10+condition->len+10+suffixWidth+10
        )));
        wid=topHeight+innerHeight+bottomHeight;

        qreal innerConnectorX=leftWidth+codeConnectorX;
        qreal innerConnectorRight=innerConnectorX+codeConnectorWidth;
        qreal innerBottomY=topHeight+innerHeight;
        QPolygonF shape;
        shape<<QPointF(0,0)
             <<QPointF(codeConnectorX,0)
             <<QPointF(codeConnectorX,codeConnectorHeight)
             <<QPointF(codeConnectorX+codeConnectorWidth,codeConnectorHeight)
             <<QPointF(codeConnectorX+codeConnectorWidth,0)
             <<QPointF(len,0)
             <<QPointF(len,topHeight)
             <<QPointF(innerConnectorRight,topHeight)
             <<QPointF(innerConnectorRight,topHeight+codeConnectorHeight)
             <<QPointF(innerConnectorX,topHeight+codeConnectorHeight)
             <<QPointF(innerConnectorX,topHeight)
             <<QPointF(leftWidth,topHeight)
             <<QPointF(leftWidth,innerBottomY)
             <<QPointF(innerConnectorX,innerBottomY)
             <<QPointF(innerConnectorX,innerBottomY+codeConnectorHeight)
             <<QPointF(innerConnectorRight,innerBottomY+codeConnectorHeight)
             <<QPointF(innerConnectorRight,innerBottomY)
             <<QPointF(len,innerBottomY)
             <<QPointF(len,wid)
             <<QPointF(codeConnectorX+codeConnectorWidth,wid)
             <<QPointF(codeConnectorX+codeConnectorWidth,wid+codeConnectorHeight)
             <<QPointF(codeConnectorX,wid+codeConnectorHeight)
             <<QPointF(codeConnectorX,wid)
             <<QPointF(0,wid);
        setPolygon(shape);
        text->setPos(10,(topHeight-textHeight)/2);
        condition->setPos(20+textWidth,(topHeight-condition->wid)/2);
        suffixText->setPos(20+textWidth+condition->len+10,(topHeight-suffixHeight)/2);

        shadow->setPolygon(shape);
    }

    CodeBlock* copy() override{
        ControlCodeBlock* newBlock=new ControlCodeBlock(type,s,condition,false);
        newBlock->innerHeight=innerHeight;
        newBlock->refreshSize();
        newBlock->setPos(pos());
        return newBlock;
    }
};

class StartBlock:public CodeBlock{
public:
    StartBlock(int base=false,QGraphicsItem * parent=nullptr):
        CodeBlock(-1,"开始运行",base,parent){
        setBrush(QColor(156,118,42));
        setCodeBlockShape(this,shadow,len,wid,false,true);
    }
    CodeBlock* copy() override{
        StartBlock* newBlock=new StartBlock(false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class EndBlock:public CodeBlock{
public:
    EndBlock(int base=false,QGraphicsItem* parent=nullptr):
        CodeBlock(-4,QString::fromUtf8("结束"),base,parent){
        setBrush(QColor(156,118,42));
        setCodeBlockShape(this,shadow,len,wid,true,false);
    }

    CodeBlock* copy() override{
        EndBlock* newBlock=new EndBlock(false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

QColor customBlockColor(){
    return QColor(82,45,122);
}

double customParameterValue(const QString& customName);

class CustomParamBlock:public FloatBlock{
public:
    QString customName;
    QString parameterName;

    CustomParamBlock(QString _customName,QString _parameterName,
                     int base=false,QGraphicsItem* parent=nullptr):
        FloatBlock(105,base,parent){
        customName=_customName;
        parameterName=_parameterName;
        text->setPlainText(parameterName);
        refreshSize();
        setBrush(customBlockColor());
    }

    double getValue() const override{
        return customParameterValue(customName);
    }

    void setParameterName(const QString& name){
        parameterName=name.trimmed();
        text->setPlainText(parameterName);
        refreshSize();
    }

    void refreshSize() override{
        updateShape(text->boundingRect().width()+variableHorizontalPadding*2);
        centerText();
    }

    void editValue() override{
    }

    bool isOperator() const override{
        return false;
    }

    FloatBlock* copy() override{
        CustomParamBlock* newBlock=new CustomParamBlock(customName,parameterName,false);
        newBlock->setPos(pos());
        return newBlock;
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
};

class CustomHatBlock:public CodeBlock{
public:
    QString customName;
    QString parameterName;
    CustomParamBlock* parameterBlock;

    CustomHatBlock(QString name,QString paramName="x",int base=false,QGraphicsItem* parent=nullptr):
        CodeBlock(-3,name,base,parent){
        customName=name;
        parameterName=paramName.trimmed();
        parameterBlock=parameterName.isEmpty()?nullptr:
            new CustomParamBlock(customName,parameterName,true,this);
        setBrush(customBlockColor());
        refreshSize();
    }

    bool hasParameter() const{
        return !parameterName.isEmpty();
    }

    void refreshSize() override{
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        if(!hasParameter()){
            if(parameterBlock!=nullptr){
                parameterBlock->hide();
            }
            wid=40;
            len=static_cast<int>(std::ceil(textWidth+30));
            setCodeBlockShape(this,shadow,len,wid,false,true);
            text->setPos(10,(wid-textHeight)/2);
            return;
        }
        if(parameterBlock!=nullptr){
            parameterBlock->show();
        }
        wid=std::max(40,parameterBlock->wid+10);
        len=static_cast<int>(std::ceil(20+textWidth+10+parameterBlock->len+10));
        setCodeBlockShape(this,shadow,len,wid,false,true);
        text->setPos(10,(wid-textHeight)/2);
        parameterBlock->setParameterName(parameterName);
        parameterBlock->setPos(20+textWidth+10,(wid-parameterBlock->wid)/2);

    }

    CodeBlock* copy() override{
        CustomHatBlock* newBlock=new CustomHatBlock(customName,parameterName,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class CustomCallBlock:public CodeBlock{
public:
    QString customName;
    QString parameterName;
    FloatBlock* value;

    CustomCallBlock(QString name,QString paramName="",FloatBlock* _value=nullptr,
                    int base=false,QGraphicsItem* parent=nullptr):
        CodeBlock(11,name,base,parent){
        customName=name;
        parameterName=paramName.trimmed();
        value=nullptr;
        if(parameterName.isEmpty()){
            value=nullptr;
        }
        else if(_value==nullptr){
            value=new FloatBlock(0,false,this);
        }
        else{
            value=_value->copy();
            value->setParentItem(this);
        }
        if(value!=nullptr){
            value->setMovable(!base);
        }
        setBrush(customBlockColor());
        refreshSize();
    }

    bool hasParameter() const{
        return !parameterName.isEmpty();
    }

    void refreshSize() override{
        qreal textWidth=text->boundingRect().width();
        qreal textHeight=text->boundingRect().height();
        if(!hasParameter()||value==nullptr){
            wid=40;
            len=static_cast<int>(std::ceil(textWidth+30));
            setCodeBlockShape(this,shadow,len,wid,true,true);
            text->setPos(10,(wid-textHeight)/2);
            return;
        }
        wid=std::max(40,value->wid+10);
        len=static_cast<int>(std::ceil(20+textWidth+10+value->len+10));
        setCodeBlockShape(this,shadow,len,wid,true,true);
        text->setPos(10,(wid-textHeight)/2);
        value->setPos(20+textWidth,(wid-value->wid)/2);
    }

    CodeBlock* copy() override{
        CustomCallBlock* newBlock=new CustomCallBlock(customName,parameterName,value,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

bool isTopOnlyCodeBlock(CodeBlock* block){
    return dynamic_cast<StartBlock*>(block)!=nullptr||
           dynamic_cast<CustomHatBlock*>(block)!=nullptr;
}

bool isEndCodeBlock(CodeBlock* block){
    return dynamic_cast<EndBlock*>(block)!=nullptr;
}

CodeBlock* codeChainTail(CodeBlock* block){
    CodeBlock* curr=block;
    while(curr!=nullptr&&curr->next!=nullptr){
        curr=curr->next;
    }
    return curr;
}

bool codeChainEndsWithEndBlock(CodeBlock* block){
    return isEndCodeBlock(codeChainTail(block));
}

template<class T>
const T max(const T a,const T b){
    return a>b?a:b;
}
template<class T>
const T min(const T a,const T b){
    return a<b?a:b;
}

extern int mapdata[screensize][screensize];

int currentMapWidth(){
    int width=static_cast<int>(level::activeLevel().map().size());
    if(width<=0){
        width=screensize;
    }
    return std::max(1,std::min(width,screensize));
}

int currentMapHeight(){
    int height=0;
    for(const auto& column:level::activeLevel().map()){
        height=std::max(height,static_cast<int>(column.size()));
    }
    if(height<=0){
        height=screensize;
    }
    return std::max(1,std::min(height,screensize));
}

int currentMapSide(){
    return std::max(currentMapWidth(),currentMapHeight());
}

bool isBlockingMapCell(int cell);

class Robot:public QGraphicsPixmapItem{
public:
    int gridx;
    int gridy;
    int direction;
    QPixmap directionSprites[4];
    QPixmap iconSprite;
    int currentCellSize;

    Robot(QGraphicsItem * parent=nullptr):QGraphicsPixmapItem(parent){
        iconSprite=trimTransparentPixmap(loadImageAsset("icons/robot.png"));
        directionSprites[0]=trimTransparentPixmap(loadPaintingAsset("right.png"));
        directionSprites[1]=trimTransparentPixmap(loadPaintingAsset("back.png"));
        directionSprites[2]=trimTransparentPixmap(loadPaintingAsset("left.png"));
        directionSprites[3]=trimTransparentPixmap(loadPaintingAsset("straight.png"));
        gridx=0;
        gridy=0;
        direction=0;
        currentCellSize=squaresize;
        setTransformationMode(Qt::SmoothTransformation);
        setAcceptedMouseButtons(Qt::NoButton);
        refreshSprite(currentCellSize);
    }

    QPixmap fallbackSprite(int cellSize) const{
        QPixmap pixmap(cellSize,cellSize);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing,true);
        painter.setBrush(QColor(48,180,70));
        painter.setPen(Qt::NoPen);
        qreal pad=cellSize*0.22;
        QPolygonF shape;
        if(direction==0){
            shape<<QPointF(pad,pad)<<QPointF(cellSize-pad,cellSize/2.0)<<QPointF(pad,cellSize-pad);
        }
        else if(direction==1){
            shape<<QPointF(pad,pad)<<QPointF(cellSize-pad,pad)<<QPointF(cellSize/2.0,cellSize-pad);
        }
        else if(direction==2){
            shape<<QPointF(cellSize-pad,pad)<<QPointF(pad,cellSize/2.0)<<QPointF(cellSize-pad,cellSize-pad);
        }
        else{
            shape<<QPointF(pad,cellSize-pad)<<QPointF(cellSize-pad,cellSize-pad)<<QPointF(cellSize/2.0,pad);
        }
        painter.drawPolygon(shape);
        return pixmap;
    }

    void refreshSprite(int cellSize){
        currentCellSize=cellSize;
        QPixmap sprite=iconSprite.isNull()?directionSprites[direction]:rotatedIconSprite();
        QPixmap scaled=sprite.isNull()
            ?fallbackSprite(cellSize)
            :sprite.scaled(cellSize,cellSize,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        setPixmap(scaled);
        setOffset((cellSize-scaled.width())/2.0,(cellSize-scaled.height())/2.0);
    }

    QPixmap rotatedIconSprite() const{
        if(iconSprite.isNull()){
            return QPixmap();
        }
        // robot.png faces left. Directions are: 0 right, 1 down, 2 left, 3 up.
        const int rotationDegrees[]={180,90,0,270};
        QTransform transform;
        transform.rotate(rotationDegrees[direction]);
        return iconSprite.transformed(transform,Qt::SmoothTransformation);
    }

    void turnLeft(){
        direction=(direction+3)%4;
        refreshSprite(currentCellSize);
    }
    void turnRight(){
        direction=(direction+1)%4;
        refreshSprite(currentCellSize);
    }
    void moveForward(int step){
        if(step==0){
            return;
        }
        int sign=1;
        int newx=gridx;
        int newy=gridy;
        if(direction==0){
            newx+=sign;
        }
        if(direction==1){
            newy+=sign;
        }
        if(direction==2){
            newx-=sign;
        }
        if(direction==3){
            newy-=sign;
        }
        if(newx<0||newx>=currentMapWidth()||newy<0||newy>=currentMapHeight()){
            return;
        }
        int cell=mapdata[newx][newy];
        if(isBlockingMapCell(cell)){
            return;
        }
        gridx=newx;
        gridy=newy;
    }
    void SyncCell(int cellSize,QPoint offset){
        setPos(gridx*cellSize+offset.x(),gridy*cellSize+offset.y());
        refreshSprite(cellSize);
    }
};

extern int mapdata[screensize][screensize];
extern Robot* player;

bool isPassableMapCell(int cell){
    return cell==level::CellEmpty||
           cell==level::CellPlate||
           cell==level::CellLightG||
           cell==level::CellLightR||
           cell==level::CellLightY||
           cell==level::CellEnd;
}

bool isBlockingMapCell(int cell){
    return !isPassableMapCell(cell);
}

int frontMapType(){
    if(player==nullptr){
        return 1;
    }
    int x=player->gridx;
    int y=player->gridy;
    if(player->direction==0){
        x++;
    }
    else if(player->direction==1){
        y++;
    }
    else if(player->direction==2){
        x--;
    }
    else if(player->direction==3){
        y--;
    }
    if(x<0||x>=currentMapWidth()||y<0||y>=currentMapHeight()){
        return 1;
    }
    return isBlockingMapCell(mapdata[x][y])?1:0;
}

class RobotCoordBlock:public FloatBlock{
public:
    int coordType;

    RobotCoordBlock(int _coordType,int base=false,QGraphicsItem* parent=nullptr):
        FloatBlock(102,base,parent){
        coordType=_coordType;
        text->setPlainText(coordType==0?"当前x坐标":"当前y坐标");
        refreshSize();
        setBrush(robotCoordColor());
    }

    double getValue() const override{
        waitRuntimeValueRead();
        if(player==nullptr){
            return 0.0;
        }
        return coordType==0?player->gridx:player->gridy;
    }

    void refreshSize() override{
        updateShape(text->boundingRect().width()+variableHorizontalPadding*2);
        centerText();
    }

    void editValue() override{
    }

    FloatBlock* copy() override{
        RobotCoordBlock* newBlock=new RobotCoordBlock(coordType,false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

class RobotFrontMapBlock:public FloatBlock{
public:
    RobotFrontMapBlock(int base=false,QGraphicsItem* parent=nullptr):
        FloatBlock(103,base,parent){
        text->setPlainText("前方块类型");
        refreshSize();
        setBrush(robotCoordColor());
    }

    double getValue() const override{
        waitRuntimeValueRead();
        return frontMapType();
    }

    void refreshSize() override{
        updateShape(text->boundingRect().width()+variableHorizontalPadding*2);
        centerText();
    }

    void editValue() override{
    }

    FloatBlock* copy() override{
        RobotFrontMapBlock* newBlock=new RobotFrontMapBlock(false);
        newBlock->setPos(pos());
        return newBlock;
    }
};

extern std::map<QString,vector<double>> customParameterStacks;

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
        robot->moveForward(steps==0.0?0:1);
    }

    void waitFrames(double) override{
        runtimeWaitActionFrame=true;
    }

    void setVariable(const std::string& name,double value) override{
        QString key=QString::fromStdString(name);
        QByteArray bytes=key.toUtf8();
        if(runtimeState.variableReadOnly(name)){
            message::readOnlyValue(bytes.constData());
            runtimeStopRequested=true;
            return;
        }
        if(!runtimeState.setVariable(name,value)){
            message::variableNotFound(bytes.constData());
            runtimeStopRequested=true;
            return;
        }
    }

    void increaseVariable(const std::string& name,double value) override{
        QString key=QString::fromStdString(name);
        QByteArray bytes=key.toUtf8();
        if(runtimeState.variableReadOnly(name)){
            message::readOnlyValue(bytes.constData());
            runtimeStopRequested=true;
            return;
        }
        double current=0.0;
        if(!runtimeState.getVariable(name,&current)){
            message::variableNotFound(bytes.constData());
            runtimeStopRequested=true;
            return;
        }
        if(!runtimeState.setVariable(name,current+value)){
            message::variableNotFound(bytes.constData());
            runtimeStopRequested=true;
            return;
        }
    }

    void pushList(const std::string& name,double value) override{
        QString key=QString::fromStdString(name);
        QByteArray bytes=key.toUtf8();
        if(runtimeState.listReadOnly(name)){
            message::readOnlyValue(bytes.constData());
            runtimeStopRequested=true;
            return;
        }
        if(!runtimeState.pushList(name,value)){
            message::listNotFound(bytes.constData());
            runtimeStopRequested=true;
            return;
        }
    }

    void setListValue(const std::string& name,double index,double value) override{
        QString key=QString::fromStdString(name);
        QByteArray bytes=key.toUtf8();
        if(runtimeState.listReadOnly(name)){
            message::readOnlyValue(bytes.constData());
            runtimeStopRequested=true;
            return;
        }
        if(!runtimeState.hasList(name)){
            message::listNotFound(bytes.constData());
            runtimeStopRequested=true;
            return;
        }
        int userIndex=static_cast<int>(std::floor(index));
        int idx=userIndex-1;
        if(!runtimeState.setListValue(name,idx,value)){
            message::listIndexOutOfRange(bytes.constData(),userIndex);
            runtimeStopRequested=true;
            return;
        }
    }

    void removeListValue(const std::string& name,double index) override{
        QString key=QString::fromStdString(name);
        QByteArray bytes=key.toUtf8();
        if(runtimeState.listReadOnly(name)){
            message::readOnlyValue(bytes.constData());
            runtimeStopRequested=true;
            return;
        }
        if(!runtimeState.hasList(name)){
            message::listNotFound(bytes.constData());
            runtimeStopRequested=true;
            return;
        }
        int userIndex=static_cast<int>(std::floor(index));
        int idx=userIndex-1;
        if(!runtimeState.removeListValue(name,idx)){
            message::listIndexOutOfRange(bytes.constData(),userIndex);
            runtimeStopRequested=true;
            return;
        }
    }

    void clearList(const std::string& name) override{
        QString key=QString::fromStdString(name);
        QByteArray bytes=key.toUtf8();
        if(runtimeState.listReadOnly(name)){
            message::readOnlyValue(bytes.constData());
            runtimeStopRequested=true;
            return;
        }
        if(!runtimeState.clearList(name)){
            message::listNotFound(bytes.constData());
            runtimeStopRequested=true;
            return;
        }
    }

    void enterCustomBlock(const std::string& name,double value) override{
        customParameterStacks[QString::fromStdString(name)].push_back(value);
    }

    void leaveCustomBlock(const std::string& name) override{
        QString key=QString::fromStdString(name);
        auto it=customParameterStacks.find(key);
        if(it!=customParameterStacks.end()&&!it->second.empty()){
            it->second.pop_back();
        }
    }

    void showMessage(const std::string& text,double value) override{
        if(runtimeStopRequested){
            return;
        }
        QString output=QString("%1 %2")
            .arg(QString::fromStdString(text))
            .arg(QString::number(value,'g',6));
        message::output(output.toStdString());
    }
};

int maxZ=10;
int mapdata[screensize][screensize];
Robot* player=nullptr;
QGraphicsRectItem* stage;
QGraphicsRectItem* toolboxBackground;
QGraphicsRectItem* workspaceBackground;
QGraphicsRectItem* squares[screensize][screensize];
QGraphicsPixmapItem* platePairImages[screensize][screensize];
Button* runButton=nullptr;
Button* fastRunButton=nullptr;
TextButton* runTextButton=nullptr;
Button* activeRunButton=nullptr;
TextButton* testButton=nullptr;
TextButton* fullscreenButton=nullptr;
TextButton* shrinkStageButton=nullptr;
QGraphicsPixmapItem* informationImage=nullptr;
QGraphicsRectItem* informationFallbackBox=nullptr;
QGraphicsRectItem* fullscreenBackdrop=nullptr;
QGraphicsTextItem* testStatusText=nullptr;
QGraphicsTextItem* timeStatusText=nullptr;
TextButton* createVariableButton=nullptr;
TextButton* createListButton=nullptr;
TextButton* createCustomBlockButton=nullptr;
TextButton* exitButton=nullptr;
TextButton* levelInfoButton=nullptr;
LevelHintPanel* levelInfoPanel=nullptr;
ScrollSlider* toolboxSlider=nullptr;
ScrollSlider* workspaceSlider=nullptr;
vector<CodeBlock*> codeBlocks;
vector<CodeBlock*> baseCodeBlocks;
vector<FloatBlock*> floatBlocks;
vector<VariableBlock*> variableBaseBlocks;
vector<FloatBlock*> listLabelBaseBlocks;
vector<FloatBlock*> listFloatBaseBlocks;
vector<CodeBlock*> listCodeBaseBlocks;
vector<CustomCallBlock*> customCallBaseBlocks;
std::map<QString,CustomHatBlock*> customHatBlocks;
std::map<QString,vector<double>> customParameterStacks;
SetVariableBlock* variableSetBaseBlock=nullptr;
IncreaseVariableBlock* variableIncreaseBaseBlock=nullptr;
StartBlock* currentStartBlock=nullptr;
CodeBlock* runningBlock=nullptr;
vector<ContextMenuButton*> contextMenuButtons;
qreal toolboxScrollY=0;
qreal workspaceScrollY=0;
qreal toolboxMaxScrollY=0;
int variableToolboxStartY=0;
std::set<std::string> dataLockedVariableNames;
std::set<std::string> dataLockedListNames;

void resetSceneGlobals(){
    appScene=nullptr;
    stage=nullptr;
    toolboxBackground=nullptr;
    workspaceBackground=nullptr;
    for(int i=0;i<screensize;i++){
        for(int j=0;j<screensize;j++){
            squares[i][j]=nullptr;
            platePairImages[i][j]=nullptr;
        }
    }
    runButton=nullptr;
    fastRunButton=nullptr;
    runTextButton=nullptr;
    activeRunButton=nullptr;
    testButton=nullptr;
    fullscreenButton=nullptr;
    shrinkStageButton=nullptr;
    informationImage=nullptr;
    informationFallbackBox=nullptr;
    fullscreenBackdrop=nullptr;
    testStatusText=nullptr;
    timeStatusText=nullptr;
    createVariableButton=nullptr;
    createListButton=nullptr;
    createCustomBlockButton=nullptr;
    exitButton=nullptr;
    levelInfoButton=nullptr;
    levelInfoPanel=nullptr;
    toolboxSlider=nullptr;
    workspaceSlider=nullptr;
    codeBlocks.clear();
    baseCodeBlocks.clear();
    floatBlocks.clear();
    variableBaseBlocks.clear();
    listLabelBaseBlocks.clear();
    listFloatBaseBlocks.clear();
    listCodeBaseBlocks.clear();
    customCallBaseBlocks.clear();
    customHatBlocks.clear();
    customParameterStacks.clear();
    variableSetBaseBlock=nullptr;
    variableIncreaseBaseBlock=nullptr;
    currentStartBlock=nullptr;
    runningBlock=nullptr;
    contextMenuButtons.clear();
    toolboxScrollY=0;
    workspaceScrollY=0;
    toolboxMaxScrollY=0;
    variableToolboxStartY=0;
    maxZ=10;
    stageExpanded=false;
    contextMenuButtonPressed=false;
    restoringUndo=false;
    suppressCustomReferenceRemoval=false;
    clearRuntimeCodeSnapshot();
}

void ensureBuiltInRuntimeVariables(){
    if(level::activeLevelType()==level::LevelType::Map){
        runtimeState.forceSetVariable("time",levelTestTimeCount,true);
    }
}

void clearDataLockedNames(){
    dataLockedVariableNames.clear();
    dataLockedListNames.clear();
}

void lockDataCaseNames(const std::vector<level::DataTestCase>& cases){
    clearDataLockedNames();
    for(const level::DataTestCase& testCase:cases){
        for(const auto& item:testCase.inputVariables){
            dataLockedVariableNames.insert(item.first);
        }
        for(const auto& item:testCase.expectedVariables){
            dataLockedVariableNames.insert(item.first);
        }
        for(const auto& item:testCase.inputLists){
            dataLockedListNames.insert(item.first);
        }
        for(const auto& item:testCase.expectedLists){
            dataLockedListNames.insert(item.first);
        }
    }
}

void lockCurrentDataRuntimeNames(){
    clearDataLockedNames();
    if(level::activeLevelType()!=level::LevelType::DataOutput){
        return;
    }
    for(const auto& item:runtimeState.variables()){
        dataLockedVariableNames.insert(item.first);
    }
    for(const auto& item:runtimeState.lists()){
        dataLockedListNames.insert(item.first);
    }
}

bool canRenameVariableName(const QString& name){
    std::string key=name.toStdString();
    return dataLockedVariableNames.find(key)==dataLockedVariableNames.end()&&
           !runtimeState.variableReadOnly(key);
}

bool canRenameListName(const QString& name){
    std::string key=name.toStdString();
    return dataLockedListNames.find(key)==dataLockedListNames.end()&&
           !runtimeState.listReadOnly(key);
}

qreal nextDragZ(){
    maxZ++;
    return draggingZ+maxZ*1000;
}

double customParameterValue(const QString& customName){
    auto it=customParameterStacks.find(customName);
    if(it==customParameterStacks.end()||it->second.empty()){
        message::otherError("无法调用自定义积木局部变量");
        runtimeSkipCurrentBlock=true;
        runtimeStopRequested=true;
        return 0.0;
    }
    return it->second.back();
}

int stageDisplaySize(){
    return stageExpanded?720:stagePixelSize;
}

QPoint stageDisplayOrigin(){
    return stageExpanded?QPoint((appWidth-stageDisplaySize())/2,10):QPoint(stageX,stageY);
}

int stageDisplayCellSize(){
    return stageDisplaySize()/currentMapSide();
}

QRectF informationRect(){
    QPoint origin=stageDisplayOrigin();
    int y=stageExpanded?origin.y()+stageDisplaySize()+10:460;
    int infoWidth=320;
    int infoHeight=107;
    QPixmap pixmap=loadImageAsset("information.png");
    if(stageExpanded){
        infoHeight=80;
        infoWidth=int(std::round(double(infoHeight)*320.0/107.0));
        if(!pixmap.isNull()&&pixmap.height()>0){
            infoWidth=int(std::round(double(infoHeight)*pixmap.width()/pixmap.height()));
        }
    }
    else{
        infoWidth=320;
        if(!pixmap.isNull()&&pixmap.width()>0){
            infoHeight=int(std::round(double(infoWidth)*pixmap.height()/pixmap.width()));
        }
    }
    int x=stageExpanded?origin.x()+(stageDisplaySize()-infoWidth)/2:10;
    return QRectF(x,y,infoWidth,infoHeight);
}

void updateInformationGeometry(){
    QRectF rect=informationRect();
    qreal z=stageExpanded?panelMaskZ+100030:20;
    QPixmap pixmap=loadImageAsset("information.png");
    if(informationImage!=nullptr&&!pixmap.isNull()){
        informationImage->setPixmap(pixmap.scaled(
            int(rect.width()),
            int(rect.height()),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));
        informationImage->setPos(rect.topLeft());
        informationImage->setZValue(z);
    }
    if(informationFallbackBox!=nullptr){
        informationFallbackBox->setRect(rect);
        informationFallbackBox->setZValue(z);
    }
    if(testStatusText!=nullptr){
        qreal scale=rect.height()/107.0;
        testStatusText->setFont(QFont("Arial",std::max(8,int(std::round(20*scale))),QFont::Bold));
        testStatusText->setTextWidth(70*scale);
        testStatusText->setPos(rect.left()+72*scale,rect.top()+34*scale);
        testStatusText->setZValue(z+1);
    }
    if(timeStatusText!=nullptr){
        qreal scale=rect.height()/107.0;
        timeStatusText->setFont(QFont("Arial",std::max(8,int(std::round(20*scale))),QFont::Bold));
        timeStatusText->setTextWidth(70*scale);
        timeStatusText->setPos(rect.left()+225*scale,rect.top()+34*scale);
        timeStatusText->setZValue(z+1);
    }
}

QBrush scaledCellBrush(const QString& fileName,const QBrush& fallback,int cellSize){
    QPixmap pixmap=loadImageAsset(fileName);
    if(pixmap.isNull()){
        return fallback;
    }
    return QBrush(pixmap.scaled(
        cellSize,
        cellSize,
        Qt::IgnoreAspectRatio,
        Qt::SmoothTransformation
    ));
}

QBrush rotatedCellBrush(const QString& fileName,const QBrush& fallback,int cellSize,int degrees){
    QPixmap pixmap=loadImageAsset(fileName);
    if(pixmap.isNull()){
        return fallback;
    }
    QPixmap rotated=pixmap.transformed(QTransform().rotate(degrees),Qt::SmoothTransformation);
    return QBrush(rotated.scaled(
        cellSize,
        cellSize,
        Qt::IgnoreAspectRatio,
        Qt::SmoothTransformation
    ));
}

std::map<int,QBrush> buildCellBrushes(int cellSize){
    static std::map<int,std::map<int,QBrush>> cache;
    auto cached=cache.find(cellSize);
    if(cached!=cache.end()){
        return cached->second;
    }
    std::map<int,QBrush> brushes;
    brushes[level::CellEmpty]=scaledCellBrush("floor.png",QBrush(Qt::gray),cellSize);
    brushes[level::CellWall]=scaledCellBrush("box.png",QBrush(QColor(126,76,36)),cellSize);
    brushes[level::CellEnd]=scaledCellBrush("target.png",QBrush(QColor(178,48,48)),cellSize);
    brushes[level::CellSpikeUp]=scaledCellBrush("spike1.png",QBrush(QColor(132,132,132)),cellSize);
    brushes[level::CellSpikeDown]=scaledCellBrush("spike2.png",QBrush(QColor(86,86,86)),cellSize);
    brushes[level::CellLightOff]=scaledCellBrush("light1.png",brushes[level::CellEmpty],cellSize);
    brushes[level::CellLightG]=scaledCellBrush("light2.png",brushes[level::CellEmpty],cellSize);
    brushes[level::CellLightR]=scaledCellBrush("light3.png",brushes[level::CellEmpty],cellSize);
    brushes[level::CellLightY]=scaledCellBrush("light4.png",brushes[level::CellEmpty],cellSize);
    brushes[level::CellScope1]=scaledCellBrush("scope.png",brushes[level::CellEmpty],cellSize);
    brushes[level::CellScope2]=rotatedCellBrush("scope.png",brushes[level::CellEmpty],cellSize,180);
    brushes[level::CellScope3]=rotatedCellBrush("scope.png",brushes[level::CellEmpty],cellSize,270);
    brushes[level::CellScope4]=rotatedCellBrush("scope.png",brushes[level::CellEmpty],cellSize,90);
    brushes[level::CellBeam1]=scaledCellBrush("beam.png",brushes[level::CellEmpty],cellSize);
    brushes[level::CellBeam2]=rotatedCellBrush("beam.png",brushes[level::CellEmpty],cellSize,90);
    brushes[level::CellLiquid]=scaledCellBrush("liquid.png",brushes[level::CellEmpty],cellSize);
    brushes[level::CellPlate]=scaledCellBrush("plate.png",brushes[level::CellEmpty],cellSize);
    brushes[level::CellValve]=scaledCellBrush("valve.png",brushes[level::CellEmpty],cellSize);
    brushes[level::CellAntenna]=scaledCellBrush("antenna.png",brushes[level::CellEmpty],cellSize);
    brushes[level::CellAntenna2]=scaledCellBrush("antenna2.png",brushes[level::CellEmpty],cellSize);
    brushes[level::CellLiquid2]=scaledCellBrush("liquid2.png",brushes[level::CellEmpty],cellSize);
    brushes[level::CellValve2]=scaledCellBrush("valve2.png",brushes[level::CellEmpty],cellSize);
    cache[cellSize]=brushes;
    return brushes;
}

QBrush brushForCell(int cell,const std::map<int,QBrush>& brushes){
    auto it=brushes.find(cell);
    if(it!=brushes.end()){
        return it->second;
    }
    auto floorIt=brushes.find(level::CellEmpty);
    return floorIt==brushes.end()?QBrush(Qt::gray):floorIt->second;
}

QString platePairAssetForCells(int deviceCell,int plateCell){
    if(plateCell!=level::CellPlate){
        return QString();
    }
    if(deviceCell==level::CellValve){
        return QStringLiteral("valve-plate1.png");
    }
    if(deviceCell==level::CellValve2){
        return QStringLiteral("valve-plate2.png");
    }
    if(deviceCell==level::CellLiquid){
        return QStringLiteral("liquid-plate1.png");
    }
    if(deviceCell==level::CellLiquid2){
        return QStringLiteral("liquid-plate2.png");
    }
    if(deviceCell==level::CellAntenna){
        return QStringLiteral("antenna-plate1.png");
    }
    if(deviceCell==level::CellAntenna2){
        return QStringLiteral("antenna-plate2.png");
    }
    return QString();
}

void updatePlatePairImages(QPoint origin,int cellSize,qreal baseZ){
    for(int i=0;i<screensize;i++){
        for(int j=0;j<screensize;j++){
            if(platePairImages[i][j]!=nullptr){
                platePairImages[i][j]->setVisible(false);
            }
        }
    }
    int mapWidth=currentMapWidth();
    int mapHeight=currentMapHeight();
    for(int x=0;x+1<mapWidth;x++){
        for(int y=0;y<mapHeight;y++){
            QString asset=platePairAssetForCells(mapdata[x][y],mapdata[x+1][y]);
            if(asset.isEmpty()){
                continue;
            }
            if(squares[x][y]!=nullptr){
                squares[x][y]->setVisible(false);
            }
            if(squares[x+1][y]!=nullptr){
                squares[x+1][y]->setVisible(false);
            }
            if(platePairImages[x][y]==nullptr&&appScene!=nullptr){
                platePairImages[x][y]=appScene->addPixmap(QPixmap());
            }
            QGraphicsPixmapItem* item=platePairImages[x][y];
            if(item==nullptr){
                continue;
            }
            QPixmap pixmap=loadImageAsset(asset);
            if(!pixmap.isNull()){
                item->setPixmap(pixmap.scaled(
                    cellSize*2,
                    cellSize,
                    Qt::IgnoreAspectRatio,
                    Qt::SmoothTransformation
                ));
            }
            item->setPos(origin.x()+x*cellSize,origin.y()+y*cellSize);
            item->setZValue(baseZ+2);
            item->setVisible(true);
        }
    }
}

void updateStageGeometry(){
    if(stage==nullptr){
        return;
    }
    QPoint origin=stageDisplayOrigin();
    int size=stageDisplaySize();
    int cellSize=stageDisplayCellSize();
    int mapWidth=currentMapWidth();
    int mapHeight=currentMapHeight();
    qreal baseZ=stageExpanded?panelMaskZ+100010:0;

    stage->setRect(origin.x(),origin.y(),size,size);
    stage->setZValue(baseZ);

    std::map<int,QBrush> cellBrushes=buildCellBrushes(cellSize);
    for(int i=0;i<screensize;i++){
        for(int j=0;j<screensize;j++){
            if(squares[i][j]==nullptr){
                continue;
            }
            if(i>=mapWidth||j>=mapHeight){
                squares[i][j]->setVisible(false);
                continue;
            }
            squares[i][j]->setVisible(true);
            squares[i][j]->setRect(0,0,cellSize,cellSize);
            squares[i][j]->setBrush(brushForCell(mapdata[i][j],cellBrushes));
            squares[i][j]->setPos(origin.x()+i*cellSize,origin.y()+j*cellSize);
            squares[i][j]->setZValue(baseZ+1);
        }
    }
    updatePlatePairImages(origin,cellSize,baseZ);
    if(player!=nullptr){
        player->setZValue(baseZ+10);
        player->SyncCell(cellSize,origin);
    }
}

void setStageExpanded(bool expanded){
    stageExpanded=expanded;
    if(fullscreenBackdrop==nullptr&&appScene!=nullptr){
        fullscreenBackdrop=appScene->addRect(0,0,appWidth,appHeight);
        fullscreenBackdrop->setBrush(panelBackgroundColor());
        fullscreenBackdrop->setPen(Qt::NoPen);
        fullscreenBackdrop->setAcceptedMouseButtons(Qt::LeftButton);
    }
    if(fullscreenBackdrop!=nullptr){
        fullscreenBackdrop->setVisible(stageExpanded);
        fullscreenBackdrop->setRect(0,0,appWidth,appHeight);
        fullscreenBackdrop->setZValue(panelMaskZ+100000);
    }
    updateStageGeometry();
    updateInformationGeometry();
    if(fullscreenButton!=nullptr){
        fullscreenButton->setVisible(!stageExpanded);
    }
    if(shrinkStageButton!=nullptr){
        shrinkStageButton->setVisible(stageExpanded);
        shrinkStageButton->setZValue(panelMaskZ+100040);
        shrinkStageButton->setPos(informationRect().right()+20,informationRect().top());
    }
    if(appScene!=nullptr){
        appScene->setSceneRect(0,0,appWidth,appHeight);
        for(QGraphicsView* view:appScene->views()){
            if(stageExpanded){
                view->showNormal();
                view->setFixedSize(appWidth,appHeight);
            }
            else{
                view->showNormal();
                view->setFixedSize(appWidth,appHeight);
            }
        }
    }
}

void drawStage(QGraphicsScene& scene);
void drawToolbox(QGraphicsScene& scene);
void drawWorkspace(QGraphicsScene& scene);
void updateTestStatusText();
void finishLevelTest(bool forcedFail,const QString& message=QString());
void resetRunButtons();
core::BlockExecutor::BlockSnapshot readRuntimeBlock(core::BlockExecutor::Node node);
AppGraphicsView::AppGraphicsView(QGraphicsScene* scene):QGraphicsView(scene)
{
    installRuntimeCancelFilter();
    resetSceneGlobals();
    appScene=scene;
    QGraphicsRectItem* background=scene->addRect(-10000,-10000,20000,20000);
    background->setBrush(appBackgroundColor());
    background->setPen(Qt::NoPen);
    background->setZValue(-100000);
    drawStage(*scene);
    drawToolbox(*scene);
    drawWorkspace(*scene);
    clearUndoCache();
    saveUndoCheckpoint();

    timerPtr=new QTimer(this);
    executorPtr=new core::BlockExecutor();
    QTimer* ownedTimer=timerPtr;
    core::BlockExecutor* ownedExecutor=executorPtr;
    RobotActionAdapter* robotActions=new RobotActionAdapter(player);
    QObject::connect(this,&QObject::destroyed,this,[robotActions,ownedTimer,ownedExecutor](){
        delete robotActions;
        delete ownedExecutor;
        if(executorPtr==ownedExecutor){
            executorPtr=nullptr;
        }
        if(timerPtr==ownedTimer){
            timerPtr=nullptr;
        }
    });
    QObject::connect(timerPtr,&QTimer::timeout,this,[robotActions](){
        if(executorPtr==nullptr||timerPtr==nullptr){
            return;
        }
        if(!executorPtr->running()){
            timerPtr->stop();
            if(levelTestRunning){
                finishLevelTest(false);
                return;
            }
            clearRuntimeCodeSnapshot();
            programRunning=false;
            runtimeCountersActive=false;
            resetRunButtons();
            return;
        }

        runtimeWaitActionFrame=false;
        bool stepped=executorPtr->step(readRuntimeBlock,*robotActions);
        if(executorPtr->didConsumeActionStep()){
            recordRuntimeTimeUse();
        }
        updateTestStatusText();
        if(levelTestRunning){
            if(levelTestStepCount>=levelTestStepLimit){
                player->SyncCell(stageDisplayCellSize(),stageDisplayOrigin());
                finishLevelTest(true,
                    QString("测试失败，执行 %1 次，已超时")
                        .arg(levelTestStepLimit));
                return;
            }
        }
        if(runtimeStopRequested){
            player->SyncCell(stageDisplayCellSize(),stageDisplayOrigin());
            executorPtr->reset(nullptr);
            runningBlock=nullptr;
            timerPtr->stop();
            if(levelTestRunning){
                finishLevelTest(true);
                return;
            }
            clearRuntimeCodeSnapshot();
            programRunning=false;
            runtimeCountersActive=false;
            resetRunButtons();
            return;
        }

        player->SyncCell(stageDisplayCellSize(),stageDisplayOrigin());
        runningBlock=static_cast<CodeBlock*>(executorPtr->currentNode());

        if(!stepped||!executorPtr->running()){
            timerPtr->stop();
            runningBlock=nullptr;
            if(levelTestRunning){
                finishLevelTest(!stepped);
                return;
            }
            clearRuntimeCodeSnapshot();
            programRunning=false;
            runtimeCountersActive=false;
            resetRunButtons();
            return;
        }
        timerPtr->start(runtimeIntervalForCodeBlock(runningBlock));
    });
    exitButton->onClick=[this](){
        editorExitToDesktopRequested=false;
        if(timerPtr){
            timerPtr->stop();
        }
        programRunning=false;
        levelTestRunning=false;
        runtimeCountersActive=false;
        runtimeStopRequested=true;
        QTimer::singleShot(0,this,[this](){
            close();
        });
    };
    scene->setSceneRect(0,0,appWidth,appHeight);
    setAlignment(Qt::AlignLeft|Qt::AlignTop);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFixedSize(appWidth,appHeight);
    setWindowFlags(windowFlags()&~Qt::WindowMaximizeButtonHint);
}

void AppGraphicsView::closeEvent(QCloseEvent* event){
    clearWorkspaceAndCacheOnExit();
    if(!editorExitToDesktopRequested&&onClosed){
        onClosed();
    }
    editorExitToDesktopRequested=false;
    QGraphicsView::closeEvent(event);
}

void AppGraphicsView::mousePressEvent(QMouseEvent* event){
    QPointF scenePoint=mapToScene(event->pos());
    bool onContextMenu=false;
    bool onWorkspaceContent=false;
    for(QGraphicsItem* candidate:scene()->items(scenePoint)){
        if(isContextMenuItem(candidate)){
            onContextMenu=true;
        }
        if(isWorkspaceContentItem(candidate)){
            onWorkspaceContent=true;
        }
    }
    if(event->button()==Qt::RightButton&&workspaceRect().contains(scenePoint)){
        if(onContextMenu){
            QGraphicsView::mousePressEvent(event);
            return;
        }
        if(FloatBlock* floatBlock=floatBlockAtScenePoint(scene(),scenePoint)){
            showFloatContextMenu(floatBlock,scenePoint);
            event->accept();
            return;
        }
        if(CodeBlock* codeBlock=codeBlockAtScenePoint(scene(),scenePoint)){
            showCodeContextMenu(codeBlock,scenePoint);
            event->accept();
            return;
        }
        if(!onWorkspaceContent){
            showUndoContextMenu(scenePoint);
            event->accept();
            return;
        }
    }
    if(!onContextMenu){
        clearContextMenu();
    }
    QGraphicsView::mousePressEvent(event);
}
void AppGraphicsView::keyPressEvent(QKeyEvent* event){
    if(event->key()==Qt::Key_C&&(event->modifiers()&Qt::ControlModifier)&&
       (programRunning||levelTestRunning||runtimeCountersActive)){
        stopProgram();
        event->accept();
        return;
    }
    if(event->key()==Qt::Key_Z&&(event->modifiers()&Qt::ControlModifier)){
        undoLastCheckpoint();
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}
void AppGraphicsView::wheelEvent(QWheelEvent* event){
    QPointF scenePoint=mapToScene(event->position().toPoint());
    qreal delta=-event->angleDelta().y()/1200.0;
    if(QRectF(toolboxX,toolboxY,toolboxWidth,toolboxHeight).contains(scenePoint)&&toolboxSlider!=nullptr){
        toolboxSlider->setValue(toolboxSlider->value()+delta);
        event->accept();
        return;
    }
    if(workspaceRect().contains(scenePoint)&&workspaceSlider!=nullptr){
        workspaceSlider->setValue(workspaceSlider->value()+delta);
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}
void clearAbsorbShadow(FloatBlock* block);
void deleteCodeBlock(CodeBlock* block);
void deleteFloatBlock(FloatBlock* block);
void clearContextMenu();
void showCodeContextMenu(CodeBlock* block,QPointF scenePos);
void showFloatContextMenu(FloatBlock* block,QPointF scenePos);
void removeCustomBlockReferences(const QString& customName,CodeBlock* excludedTree=nullptr);
FloatBlock* findAbsorbTarget(FloatBlock* moving);
void attachOperatorToTarget(FloatBlock* moving,FloatBlock* target);
FloatBlock* detachOperatorFromParent(FloatBlock* moving);
void showAbsorbShadow(FloatBlock* moving,FloatBlock* target);
void refreshAllControlLayouts();
void refreshControlFromInside(ControlCodeBlock* control);
void syncControlInside(ControlCodeBlock* control);
ControlCodeBlock* findInsideOwner(CodeBlock* block);
void syncScrollArea(int area);
void syncCodeChainFrom(CodeBlock* head,QPointF startPos);
int codeChainHeight(CodeBlock* head);
void rememberCodeTreeStagePos(CodeBlock* head,int area);
void rememberFloatBlockStagePos(FloatBlock* block,int area);
void stopProgram();
void resetRobot();
void resetRunButtons();
void setRunTextButtonRunning();
void updateTestStatusText();
void beginLevelTestCase(int index);
void startLevelTestRun();
void setCodeBlockStagePos(CodeBlock* block,int area,QPointF pos);
void setFloatBlockStagePos(FloatBlock* block,int area,QPointF pos);
QPointF sceneToStagePos(int area,QPointF scenePos);
void setInsertedOperatorInteractivity(FloatBlock* block);
void setCodeTreeZValue(CodeBlock* block,qreal z);
void setFloatTreeZValue(FloatBlock* block,qreal z);
void resetFloatTreeZValue(FloatBlock* block);
CodeBlock* raiseCodeTreeAboveWorkspace(CodeBlock* block);
FloatBlock* raiseFloatTreeAboveWorkspace(FloatBlock* block);
void replaceFloatExpressionWithValue(FloatBlock* root);
void refreshFloatAncestors(FloatBlock* block);
FloatBlock* rootFloatBlock(FloatBlock* block);
void eraseFloatTreeRecords(FloatBlock* block);
void eraseCodeEmbeddedFloatRecords(CodeBlock* block);

class RuntimeCancelFilter:public QObject{
public:
    explicit RuntimeCancelFilter(QObject* parent=nullptr):QObject(parent){}

protected:
    bool eventFilter(QObject* object,QEvent* event) override{
        if(event->type()==QEvent::KeyPress){
            QKeyEvent* keyEvent=static_cast<QKeyEvent*>(event);
            if(keyEvent->key()==Qt::Key_C&&(keyEvent->modifiers()&Qt::ControlModifier)&&
               (programRunning||levelTestRunning||runtimeCountersActive)){
                stopProgram();
                QWidget* modal=QApplication::activeModalWidget();
                if(modal!=nullptr){
                    modal->close();
                }
                keyEvent->accept();
                return true;
            }
        }
        return QObject::eventFilter(object,event);
    }
};

void installRuntimeCancelFilter(){
    static RuntimeCancelFilter* filter=nullptr;
    if(filter==nullptr&&QApplication::instance()!=nullptr){
        filter=new RuntimeCancelFilter(QApplication::instance());
        QApplication::instance()->installEventFilter(filter);
    }
}

bool inWorkspace(CodeBlock* block){
    return workspaceRect().contains(block->sceneBoundingRect());
}

bool inWorkspace(FloatBlock* block){
    return workspaceRect().contains(block->sceneBoundingRect());
}

QRectF workspaceRect(){
    return QRectF(workspaceX,workspaceY,workspaceWidth,workspaceHeight);
}

qreal workspaceMaxScrollY(){
    return workspaceHeight*settings::WorkspaceEditLength;
}

QRectF workspaceTotalStageRect(){
    return QRectF(0,0,workspaceWidth,workspaceHeight+workspaceMaxScrollY());
}

QRectF sceneRectToWorkspaceStageRect(const QRectF& rect){
    return QRectF(
        sceneToStagePos(scrollWorkspace,rect.topLeft()),
        sceneToStagePos(scrollWorkspace,rect.bottomRight())
    ).normalized();
}

bool inWorkspaceTotal(const QRectF& rect){
    return workspaceTotalStageRect().contains(sceneRectToWorkspaceStageRect(rect));
}

bool outsideWorkspaceHorizontal(const QRectF& rect){
    QRectF bounds=workspaceRect();
    return rect.left()<bounds.left()||rect.right()>bounds.right();
}

bool outsideWorkspaceVertical(const QRectF& rect){
    QRectF bounds=workspaceRect();
    return rect.top()<bounds.top()||rect.bottom()>bounds.bottom();
}

QRectF codeTreeSceneRect(CodeBlock* head){
    QRectF rect;
    bool initialized=false;
    CodeBlock* curr=head;
    while(curr!=nullptr){
        QRectF currRect=curr->sceneBoundingRect();
        if(!initialized){
            rect=currRect;
            initialized=true;
        }
        else{
            rect=rect.united(currRect);
        }
        ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(curr);
        if(control!=nullptr&&control->inside!=nullptr){
            rect=rect.united(codeTreeSceneRect(control->inside));
        }
        curr=curr->next;
    }
    return rect;
}

QPointF clampVerticalDeltaToWorkspace(const QRectF& rect){
    QRectF bounds=workspaceRect();
    qreal dy=0;
    if(rect.height()>bounds.height()){
        dy=bounds.top()-rect.top();
    }
    else if(rect.top()<bounds.top()){
        dy=bounds.top()-rect.top();
    }
    else if(rect.bottom()>bounds.bottom()){
        dy=bounds.bottom()-rect.bottom();
    }
    return QPointF(0,dy);
}

CodeBlock* codeChainHead(CodeBlock* block){
    if(block==nullptr){
        return nullptr;
    }
    CodeBlock* head=block;
    while(head->pre!=nullptr){
        head=head->pre;
    }
    return head;
}

CodeBlock* containingCodeBlock(QGraphicsItem* item){
    QGraphicsItem* curr=item;
    while(curr!=nullptr){
        CodeBlock* block=dynamic_cast<CodeBlock*>(curr);
        if(block!=nullptr){
            return block;
        }
        curr=curr->parentItem();
    }
    return nullptr;
}

FloatBlock* floatBlockAtScenePoint(QGraphicsScene* scene,QPointF scenePoint){
    if(scene==nullptr){
        return nullptr;
    }
    QRectF pickRect(scenePoint.x()-3,scenePoint.y()-3,6,6);
    for(QGraphicsItem* item:scene->items(pickRect,Qt::IntersectsItemShape,
                                         Qt::DescendingOrder)){
        if(item==nullptr||!item->isVisible()||isContextMenuItem(item)){
            continue;
        }
        QGraphicsItem* curr=item;
        while(curr!=nullptr){
            FloatBlock* block=dynamic_cast<FloatBlock*>(curr);
            if(block!=nullptr&&!block->isbase&&block->scrollArea!=scrollToolbox){
                return block;
            }
            curr=curr->parentItem();
        }
    }
    return nullptr;
}

CodeBlock* codeBlockAtScenePoint(QGraphicsScene* scene,QPointF scenePoint){
    if(scene==nullptr){
        return nullptr;
    }
    QRectF pickRect(scenePoint.x()-3,scenePoint.y()-3,6,6);
    for(QGraphicsItem* item:scene->items(pickRect,Qt::IntersectsItemShape,
                                         Qt::DescendingOrder)){
        if(item==nullptr||!item->isVisible()||isContextMenuItem(item)){
            continue;
        }
        QGraphicsItem* curr=item;
        while(curr!=nullptr){
            CodeBlock* block=dynamic_cast<CodeBlock*>(curr);
            if(block!=nullptr&&!block->isbase&&block->scrollArea!=scrollToolbox){
                return block;
            }
            curr=curr->parentItem();
        }
    }
    return nullptr;
}

void deleteCodeChain(CodeBlock* head){
    CustomHatBlock* customHat=dynamic_cast<CustomHatBlock*>(head);
    bool oldSuppress=suppressCustomReferenceRemoval;
    if(customHat!=nullptr&&!suppressCustomReferenceRemoval){
        removeCustomBlockReferences(customHat->customName,head);
        suppressCustomReferenceRemoval=true;
    }
    CodeBlock* curr=head;
    while(curr!=nullptr){
        CodeBlock* next=curr->next;
        curr->next=nullptr;
        deleteCodeBlock(curr);
        curr=next;
    }
    suppressCustomReferenceRemoval=oldSuppress;
}

void addCodeTreeToScene(CodeBlock* head){
    CodeBlock* curr=head;
    while(curr!=nullptr){
        codeBlocks.push_back(curr);
        if(appScene!=nullptr&&curr->scene()==nullptr){
            appScene->addItem(curr);
        }
        CustomHatBlock* customHat=dynamic_cast<CustomHatBlock*>(curr);
        if(customHat!=nullptr){
            customHatBlocks[customHat->customName]=customHat;
        }
        ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(curr);
        if(control!=nullptr){
            CodeBlock* inside=control->inside;
            while(inside!=nullptr){
                inside->insideParent=control;
                inside=inside->next;
            }
            addCodeTreeToScene(control->inside);
        }
        curr=curr->next;
    }
}

CodeBlock* cloneCodeTree(CodeBlock* source){
    CodeBlock* first=nullptr;
    CodeBlock* prev=nullptr;
    CodeBlock* curr=source;
    while(curr!=nullptr){
        if(isEndCodeBlock(curr)&&curr->next==nullptr){
            break;
        }
        CodeBlock* clone=curr->copy();
        clone->pre=prev;
        clone->next=nullptr;
        clone->insideParent=nullptr;
        clone->insideTarget=nullptr;
        clone->preTarget=nullptr;
        clone->nextTarget=nullptr;
        clone->dragging=false;
        clone->ismoving=false;
        if(prev!=nullptr){
            prev->next=clone;
        }
        else{
            first=clone;
        }
        ControlCodeBlock* sourceControl=dynamic_cast<ControlCodeBlock*>(curr);
        ControlCodeBlock* cloneControl=dynamic_cast<ControlCodeBlock*>(clone);
        if(sourceControl!=nullptr&&cloneControl!=nullptr){
            cloneControl->inside=cloneCodeTree(sourceControl->inside);
            CodeBlock* inside=cloneControl->inside;
            while(inside!=nullptr){
                inside->insideParent=cloneControl;
                inside=inside->next;
            }
            refreshControlFromInside(cloneControl);
        }
        prev=clone;
        curr=curr->next;
    }
    return first;
}

void copyCodeContext(CodeBlock* block){
    if(block==nullptr||block->isbase){
        return;
    }
    CodeBlock* source=isTopOnlyCodeBlock(block)?block->next:block;
    if(source==nullptr){
        return;
    }
    CodeBlock* copy=cloneCodeTree(source);
    if(copy==nullptr){
        return;
    }
    addCodeTreeToScene(copy);
    QPointF targetPos=source->pos()+QPointF(0,codeChainHeight(source)+20);
    syncCodeChainFrom(copy,targetPos);
    setCodeTreeZValue(copy,10);
    if(outsideWorkspaceHorizontal(codeTreeSceneRect(copy))){
        message::workspaceWidthLimitReached();
        deleteCodeChain(copy);
        refreshAllControlLayouts();
        return;
    }
    CodeBlock* zRoot=raiseCodeTreeAboveWorkspace(copy);
    rememberCodeTreeStagePos(zRoot,scrollWorkspace);
    refreshAllControlLayouts();
    saveUndoCheckpoint();
}

void deleteCodeContext(CodeBlock* block){
    if(block==nullptr||block->isbase){
        return;
    }
    if(block->pre!=nullptr){
        block->pre->next=nullptr;
        block->pre=nullptr;
    }
    ControlCodeBlock* owner=block->insideParent;
    if(owner!=nullptr&&owner->inside==block){
        owner->inside=nullptr;
        block->insideParent=nullptr;
        refreshControlFromInside(owner);
    }
    deleteCodeChain(block);
    refreshAllControlLayouts();
    saveUndoCheckpoint();
}

void copyFloatContext(FloatBlock* block){
    if(block==nullptr||block->isbase||isPlaceholderFloatValue(block)||appScene==nullptr){
        return;
    }
    FloatBlock* copy=block->copy();
    copy->setParentItem(nullptr);
    floatBlocks.push_back(copy);
    appScene->addItem(copy);
    copy->setPos(block->scenePos()+QPointF(0,block->sceneBoundingRect().height()+20));
    copy->scrollArea=scrollWorkspace;
    resetFloatTreeZValue(copy);
    setInsertedOperatorInteractivity(copy);
    if(outsideWorkspaceHorizontal(copy->sceneBoundingRect())){
        message::workspaceWidthLimitReached();
        deleteFloatBlock(copy);
        return;
    }
    FloatBlock* zRoot=raiseFloatTreeAboveWorkspace(copy);
    rememberFloatBlockStagePos(zRoot,scrollWorkspace);
    saveUndoCheckpoint();
}

bool isPlainNumberBlock(FloatBlock* block){
    return block!=nullptr&&block->type==0&&!block->isOperator();
}

bool isPlaceholderFloatValue(FloatBlock* block){
    return isPlainNumberBlock(block)&&block->parentItem()!=nullptr;
}

void deleteFloatContext(FloatBlock* block){
    if(block==nullptr||block->isbase){
        return;
    }
    if(block->parentItem()!=nullptr){
        replaceFloatExpressionWithValue(block);
        refreshAllControlLayouts();
        saveUndoCheckpoint();
        return;
    }
    if(isPlainNumberBlock(block)){
        block->setData(0);
        saveUndoCheckpoint();
        return;
    }
    deleteFloatBlock(block);
    saveUndoCheckpoint();
}

void deleteCustomCallReference(CustomCallBlock* block){
    if(block==nullptr){
        return;
    }
    if(block->isbase){
        baseCodeBlocks.erase(
            std::remove(baseCodeBlocks.begin(),baseCodeBlocks.end(),block),
            baseCodeBlocks.end()
        );
        customCallBaseBlocks.erase(
            std::remove(customCallBaseBlocks.begin(),customCallBaseBlocks.end(),block),
            customCallBaseBlocks.end()
        );
        deleteCodeBlock(block);
        return;
    }

    QPointF replacementPos=block->pos();
    CodeBlock* previous=block->pre;
    CodeBlock* following=block->next;
    ControlCodeBlock* owner=findInsideOwner(block);

    if(previous!=nullptr){
        previous->next=following;
    }
    if(owner!=nullptr&&owner->inside==block){
        owner->inside=following;
    }
    if(following!=nullptr){
        following->pre=previous;
        following->insideParent=owner;
    }

    block->pre=nullptr;
    block->next=nullptr;
    block->insideParent=nullptr;
    deleteCodeBlock(block);

    if(previous!=nullptr){
        syncCodeChainFrom(previous,previous->calculatePos());
    }
    else if(owner!=nullptr){
        refreshControlFromInside(owner);
    }
    else if(following!=nullptr){
        syncCodeChainFrom(following,replacementPos);
    }
}

bool codeTreeContains(CodeBlock* root,CodeBlock* target){
    CodeBlock* curr=root;
    while(curr!=nullptr){
        if(curr==target){
            return true;
        }
        ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(curr);
        if(control!=nullptr&&codeTreeContains(control->inside,target)){
            return true;
        }
        curr=curr->next;
    }
    return false;
}

CodeBlock* topCodeTreeRoot(CodeBlock* block){
    CodeBlock* root=block;
    while(root!=nullptr){
        while(root->pre!=nullptr){
            root=root->pre;
        }
        if(root->insideParent==nullptr){
            return root;
        }
        root=root->insideParent;
    }
    return nullptr;
}

qreal workspaceTopZExcludingCodeTree(CodeBlock* excludedRoot){
    qreal top=10;
    auto considerCode=[&](CodeBlock* block){
        if(block==nullptr||block->isbase||block->scrollArea!=scrollWorkspace||
           block->dragging||block->ismoving||codeTreeContains(excludedRoot,block)){
            return;
        }
        top=std::max(top,block->zValue());
    };
    for(CodeBlock* block:codeBlocks){
        considerCode(block);
    }
    considerCode(currentStartBlock);
    for(FloatBlock* block:floatBlocks){
        if(block==nullptr||block->isbase||block->parentItem()!=nullptr||
           block->scrollArea!=scrollWorkspace||block->dragging){
            continue;
        }
        top=std::max(top,block->zValue());
    }
    return top;
}

CodeBlock* raiseCodeTreeAboveWorkspace(CodeBlock* block){
    CodeBlock* root=topCodeTreeRoot(block);
    if(root==nullptr){
        return block;
    }
    qreal z=std::min(workspaceTopZExcludingCodeTree(root)+20,panelMaskZ-1000);
    setCodeTreeZValue(root,z);
    return root;
}

bool floatTreeContains(FloatBlock* root,FloatBlock* target){
    if(root==nullptr||target==nullptr){
        return false;
    }
    if(root==target){
        return true;
    }
    UnaryOpBlock* unary=dynamic_cast<UnaryOpBlock*>(root);
    if(unary!=nullptr){
        return floatTreeContains(unary->slot,target);
    }
    BinaryOpBlock* binary=dynamic_cast<BinaryOpBlock*>(root);
    if(binary!=nullptr){
        return floatTreeContains(binary->left,target)||floatTreeContains(binary->right,target);
    }
    ListGetBlock* listGet=dynamic_cast<ListGetBlock*>(root);
    if(listGet!=nullptr){
        return floatTreeContains(listGet->index,target);
    }
    return false;
}

qreal workspaceTopZExcludingFloatTree(FloatBlock* excludedRoot){
    qreal top=10;
    auto considerCode=[&](CodeBlock* block){
        if(block==nullptr||block->isbase||block->scrollArea!=scrollWorkspace||
           block->dragging||block->ismoving){
            return;
        }
        top=std::max(top,block->zValue());
    };
    for(CodeBlock* block:codeBlocks){
        considerCode(block);
    }
    considerCode(currentStartBlock);
    for(FloatBlock* block:floatBlocks){
        if(block==nullptr||block->isbase||block->parentItem()!=nullptr||
           block->scrollArea!=scrollWorkspace||block->dragging||
           floatTreeContains(excludedRoot,block)){
            continue;
        }
        top=std::max(top,block->zValue());
    }
    return top;
}

FloatBlock* raiseFloatTreeAboveWorkspace(FloatBlock* block){
    FloatBlock* root=rootFloatBlock(block);
    if(root==nullptr){
        return block;
    }
    qreal z=std::min(workspaceTopZExcludingFloatTree(root)+20,panelMaskZ-1000);
    setFloatTreeZValue(root,z);
    return root;
}

void removeCustomBlockReferences(const QString& customName,CodeBlock* excludedTree){
    vector<CustomCallBlock*> callsToDelete;
    auto addCall=[&](CustomCallBlock* block){
        if(block!=nullptr&&std::find(callsToDelete.begin(),callsToDelete.end(),block)==callsToDelete.end()){
            callsToDelete.push_back(block);
        }
    };
    for(CustomCallBlock* block:customCallBaseBlocks){
        if(block!=nullptr&&block->customName==customName){
            addCall(block);
        }
    }
    for(CodeBlock* block:codeBlocks){
        CustomCallBlock* customCall=dynamic_cast<CustomCallBlock*>(block);
        if(customCall!=nullptr&&customCall->customName==customName&&
           !codeTreeContains(excludedTree,customCall)){
            addCall(customCall);
        }
    }
    for(CustomCallBlock* block:callsToDelete){
        deleteCustomCallReference(block);
    }
    refreshVariableToolbox();
    refreshAllControlLayouts();
}

void rebuildCustomCallToolbox(){
    for(CustomCallBlock* block:customCallBaseBlocks){
        if(block==nullptr){
            continue;
        }
        baseCodeBlocks.erase(
            std::remove(baseCodeBlocks.begin(),baseCodeBlocks.end(),block),
            baseCodeBlocks.end()
        );
        if(block->scene()!=nullptr){
            block->scene()->removeItem(block);
        }
        delete block;
    }
    customCallBaseBlocks.clear();

    if(appScene==nullptr){
        return;
    }
    for(const auto& item:customHatBlocks){
        CustomHatBlock* hat=item.second;
        if(hat==nullptr){
            continue;
        }
        CustomCallBlock* callBlock=new CustomCallBlock(
            hat->customName,
            hat->parameterName,
            nullptr,
            true
        );
        callBlock->setZValue(10);
        appScene->addItem(callBlock);
        baseCodeBlocks.push_back(callBlock);
        customCallBaseBlocks.push_back(callBlock);
    }
}

QJsonObject serializeFloatBlock(FloatBlock* block){
    QJsonObject object;
    if(block==nullptr){
        return object;
    }
    object["type"]=block->type;
    object["data"]=block->data;
    VariableBlock* variable=dynamic_cast<VariableBlock*>(block);
    if(variable!=nullptr){
        object["kind"]="variable";
        object["name"]=variable->variableName;
        return object;
    }
    CustomParamBlock* customParam=dynamic_cast<CustomParamBlock*>(block);
    if(customParam!=nullptr){
        object["kind"]="customParam";
        object["customName"]=customParam->customName;
        object["name"]=customParam->parameterName;
        return object;
    }
    RobotCoordBlock* robotCoord=dynamic_cast<RobotCoordBlock*>(block);
    if(robotCoord!=nullptr){
        object["kind"]="robotCoord";
        object["coordType"]=robotCoord->coordType;
        return object;
    }
    RobotFrontMapBlock* frontMap=dynamic_cast<RobotFrontMapBlock*>(block);
    if(frontMap!=nullptr){
        object["kind"]="robotFrontMap";
        return object;
    }
    UnaryOpBlock* unary=dynamic_cast<UnaryOpBlock*>(block);
    if(unary!=nullptr){
        object["kind"]="unary";
        object["text"]=unary->s;
        object["slot"]=serializeFloatBlock(unary->slot);
        return object;
    }
    BinaryOpBlock* binary=dynamic_cast<BinaryOpBlock*>(block);
    if(binary!=nullptr){
        object["kind"]="binary";
        object["text"]=binary->s;
        object["left"]=serializeFloatBlock(binary->left);
        object["right"]=serializeFloatBlock(binary->right);
        return object;
    }
    ListGetBlock* listGet=dynamic_cast<ListGetBlock*>(block);
    if(listGet!=nullptr){
        object["kind"]="listGet";
        object["name"]=listGet->listName;
        object["index"]=serializeFloatBlock(listGet->index);
        return object;
    }
    ListSizeBlock* listSize=dynamic_cast<ListSizeBlock*>(block);
    if(listSize!=nullptr){
        object["kind"]="listSize";
        object["name"]=listSize->listName;
        return object;
    }
    object["kind"]="number";
    return object;
}

FloatBlock* deserializeFloatBlock(const QJsonObject& object,QGraphicsItem* parent){
    QString kind=object["kind"].toString("number");
    if(kind=="variable"){
        return new VariableBlock(object["name"].toString("x"),false,parent);
    }
    if(kind=="customParam"){
        return new CustomParamBlock(object["customName"].toString(),object["name"].toString("x"),false,parent);
    }
    if(kind=="robotCoord"){
        return new RobotCoordBlock(object["coordType"].toInt(),false,parent);
    }
    if(kind=="robotFrontMap"){
        return new RobotFrontMapBlock(false,parent);
    }
    if(kind=="unary"){
        QJsonObject slotObject=object["slot"].toObject();
        FloatBlock* slot=deserializeFloatBlock(slotObject,nullptr);
        UnaryOpBlock* block=new UnaryOpBlock(object["type"].toInt(),object["text"].toString(),slot,false,parent);
        deleteFloatBlock(slot);
        block->slot->setMovable(true);
        block->refreshSize();
        return block;
    }
    if(kind=="binary"){
        FloatBlock* left=deserializeFloatBlock(object["left"].toObject(),nullptr);
        FloatBlock* right=deserializeFloatBlock(object["right"].toObject(),nullptr);
        BinaryOpBlock* block=new BinaryOpBlock(object["type"].toInt(),object["text"].toString(),left,right,false,parent);
        deleteFloatBlock(left);
        deleteFloatBlock(right);
        block->left->setMovable(true);
        block->right->setMovable(true);
        block->refreshSize();
        return block;
    }
    if(kind=="listGet"){
        FloatBlock* index=deserializeFloatBlock(object["index"].toObject(),nullptr);
        ListGetBlock* block=new ListGetBlock(object["name"].toString("x"),index,false,parent);
        deleteFloatBlock(index);
        block->index->setMovable(true);
        block->refreshSize();
        return block;
    }
    if(kind=="listSize"){
        return new ListSizeBlock(object["name"].toString("x"),false,parent);
    }
    FloatBlock* block=new FloatBlock(object["type"].toInt(),false,parent);
    block->setData(object["data"].toDouble(0.0));
    return block;
}

QJsonObject serializeCodeBlock(CodeBlock* block){
    QJsonObject object;
    if(block==nullptr){
        return object;
    }
    object["type"]=block->type;
    object["text"]=block->s;
    QPointF stage=sceneToStagePos(scrollWorkspace,block->pos());
    object["x"]=stage.x();
    object["y"]=stage.y();
    if(dynamic_cast<StartBlock*>(block)!=nullptr){
        object["kind"]="start";
    }
    else if(dynamic_cast<EndBlock*>(block)!=nullptr){
        object["kind"]="end";
    }
    else if(CustomHatBlock* customHat=dynamic_cast<CustomHatBlock*>(block)){
        object["kind"]="customHat";
        object["name"]=customHat->customName;
        object["parameter"]=customHat->parameterName;
    }
    else if(CustomCallBlock* customCall=dynamic_cast<CustomCallBlock*>(block)){
        object["kind"]="customCall";
        object["name"]=customCall->customName;
        object["parameter"]=customCall->parameterName;
        if(customCall->value!=nullptr){
            object["value"]=serializeFloatBlock(customCall->value);
        }
    }
    else if(FloatCodeBlock* floatCode=dynamic_cast<FloatCodeBlock*>(block)){
        object["kind"]="floatCode";
        object["value"]=serializeFloatBlock(floatCode->value);
    }
    else if(OutputBlock* output=dynamic_cast<OutputBlock*>(block)){
        object["kind"]="output";
        object["message"]=output->messageText;
        object["value"]=serializeFloatBlock(output->value);
    }
    else if(SetVariableBlock* setVariable=dynamic_cast<SetVariableBlock*>(block)){
        object["kind"]=setVariable->type==14?"increaseVariable":"setVariable";
        object["name"]=setVariable->variableName;
        object["value"]=serializeFloatBlock(setVariable->value);
    }
    else if(PushListBlock* pushList=dynamic_cast<PushListBlock*>(block)){
        object["kind"]="pushList";
        object["name"]=pushList->listName;
        object["value"]=serializeFloatBlock(pushList->value);
    }
    else if(SetListBlock* setList=dynamic_cast<SetListBlock*>(block)){
        object["kind"]="setList";
        object["name"]=setList->listName;
        object["index"]=serializeFloatBlock(setList->index);
        object["value"]=serializeFloatBlock(setList->value);
    }
    else if(RemoveListItemBlock* removeItem=dynamic_cast<RemoveListItemBlock*>(block)){
        object["kind"]="removeListItem";
        object["name"]=removeItem->listName;
        object["index"]=serializeFloatBlock(removeItem->index);
    }
    else if(ClearListBlock* clearList=dynamic_cast<ClearListBlock*>(block)){
        object["kind"]="clearList";
        object["name"]=clearList->listName;
    }
    else if(ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(block)){
        object["kind"]="control";
        object["condition"]=serializeFloatBlock(control->condition);
        object["inside"]=serializeCodeBlock(control->inside);
    }
    else{
        object["kind"]="code";
    }
    object["next"]=serializeCodeBlock(block->next);
    return object;
}

CodeBlock* deserializeCodeBlock(const QJsonObject& object){
    if(object.isEmpty()){
        return nullptr;
    }
    QString kind=object["kind"].toString("code");
    CodeBlock* block=nullptr;
    if(kind=="start"){
        block=new StartBlock(false);
    }
    else if(kind=="end"){
        block=new EndBlock(false);
    }
    else if(kind=="customHat"){
        block=new CustomHatBlock(object["name"].toString("custom"),
            object["parameter"].toString("x"),false);
    }
    else if(kind=="customCall"){
        FloatBlock* value=deserializeFloatBlock(object["value"].toObject(),nullptr);
        block=new CustomCallBlock(object["name"].toString("custom"),
            object["parameter"].toString(),value,false);
        deleteFloatBlock(value);
    }
    else if(kind=="floatCode"){
        FloatBlock* value=deserializeFloatBlock(object["value"].toObject(),nullptr);
        block=new FloatCodeBlock(object["type"].toInt(),object["text"].toString(),
            value,false);
        deleteFloatBlock(value);
    }
    else if(kind=="output"){
        FloatBlock* value=deserializeFloatBlock(object["value"].toObject(),nullptr);
        block=new OutputBlock(object["message"].toString("x"),value,false);
        deleteFloatBlock(value);
    }
    else if(kind=="setVariable"){
        FloatBlock* value=deserializeFloatBlock(object["value"].toObject(),nullptr);
        block=new SetVariableBlock(object["name"].toString("x"),value,false);
        deleteFloatBlock(value);
    }
    else if(kind=="increaseVariable"){
        FloatBlock* value=deserializeFloatBlock(object["value"].toObject(),nullptr);
        block=new IncreaseVariableBlock(object["name"].toString("x"),value,false);
        deleteFloatBlock(value);
    }
    else if(kind=="pushList"){
        FloatBlock* value=deserializeFloatBlock(object["value"].toObject(),nullptr);
        block=new PushListBlock(object["name"].toString("x"),value,false);
        deleteFloatBlock(value);
    }
    else if(kind=="setList"){
        FloatBlock* index=deserializeFloatBlock(object["index"].toObject(),nullptr);
        FloatBlock* value=deserializeFloatBlock(object["value"].toObject(),nullptr);
        block=new SetListBlock(object["name"].toString("x"),index,value,false);
        deleteFloatBlock(index);
        deleteFloatBlock(value);
    }
    else if(kind=="removeListItem"){
        FloatBlock* index=deserializeFloatBlock(object["index"].toObject(),nullptr);
        block=new RemoveListItemBlock(object["name"].toString("x"),index,false);
        deleteFloatBlock(index);
    }
    else if(kind=="clearList"){
        block=new ClearListBlock(object["name"].toString("x"),false);
    }
    else if(kind=="control"){
        FloatBlock* condition=deserializeFloatBlock(object["condition"].toObject(),nullptr);
        ControlCodeBlock* control=new ControlCodeBlock(object["type"].toInt(),object["text"].toString(),
            condition,false);
        deleteFloatBlock(condition);
        control->inside=deserializeCodeBlock(object["inside"].toObject());
        CodeBlock* inside=control->inside;
        while(inside!=nullptr){
            inside->insideParent=control;
            inside=inside->next;
        }
        refreshControlFromInside(control);
        block=control;
    }
    else{
        int type=object["type"].toInt();
        if(type==3){
            FloatBlock* value=new FloatBlock(0,false);
            value->setData(1);
            block=new FloatCodeBlock(3,object["text"].toString("向前移动"),value,false);
            deleteFloatBlock(value);
        }
        else{
            block=new CodeBlock(type,object["text"].toString(),false);
        }
    }
    CodeBlock* next=deserializeCodeBlock(object["next"].toObject());
    block->next=next;
    if(next!=nullptr){
        next->pre=block;
    }
    return block;
}

void collectRuntimeCodeSnapshot(CodeBlock* block){
    CodeBlock* curr=block;
    while(curr!=nullptr){
        if(StartBlock* start=dynamic_cast<StartBlock*>(curr)){
            runtimeStartBlock=start;
        }
        if(CustomHatBlock* customHat=dynamic_cast<CustomHatBlock*>(curr)){
            runtimeCustomHatBlocks[customHat->customName]=customHat;
        }
        if(ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(curr)){
            collectRuntimeCodeSnapshot(control->inside);
        }
        curr=curr->next;
    }
}

void deleteRuntimeCodeTree(CodeBlock* block){
    CodeBlock* curr=block;
    while(curr!=nullptr){
        CodeBlock* next=curr->next;
        if(ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(curr)){
            deleteRuntimeCodeTree(control->inside);
            control->inside=nullptr;
        }
        curr->next=nullptr;
        if(curr->shadow!=nullptr){
            delete curr->shadow;
            curr->shadow=nullptr;
        }
        delete curr;
        curr=next;
    }
}

void clearRuntimeCodeSnapshot(){
    for(CodeBlock* root:runtimeCodeRoots){
        deleteRuntimeCodeTree(root);
    }
    runtimeCodeRoots.clear();
    runtimeStartBlock=nullptr;
    runtimeCustomHatBlocks.clear();
    runtimeCodeSnapshotActive=false;
    runtimeSnapshotError.clear();
    runtimeEndReached=false;
}

bool codeTreeContainsEndBlock(CodeBlock* block){
    CodeBlock* curr=block;
    while(curr!=nullptr){
        if(isEndCodeBlock(curr)){
            return true;
        }
        if(ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(curr)){
            if(codeTreeContainsEndBlock(control->inside)){
                return true;
            }
        }
        curr=curr->next;
    }
    return false;
}

bool validateEndBlockForRun(){
    int endCount=0;
    EndBlock* endBlock=nullptr;
    for(CodeBlock* block:workspaceCodeItemsFromScene()){
        if(EndBlock* candidate=dynamic_cast<EndBlock*>(block)){
            endCount++;
            endBlock=candidate;
        }
    }
    if(endCount==0){
        runtimeSnapshotError=QString::fromUtf8("缺少结束积木");
        return false;
    }
    if(endCount>1){
        runtimeSnapshotError=QString::fromUtf8("只能放置一个结束积木");
        return false;
    }
    if(endBlock!=nullptr&&endBlock->next!=nullptr){
        runtimeSnapshotError=QString::fromUtf8("结束积木下面不能连接其他积木");
        return false;
    }
    if(currentStartBlock==nullptr||!codeTreeContainsEndBlock(currentStartBlock)){
        runtimeSnapshotError=QString::fromUtf8("结束积木必须连接在开始运行链上");
        return false;
    }
    return true;
}

bool buildRuntimeCodeSnapshot(){
    clearRuntimeCodeSnapshot();
    runtimeSnapshotError.clear();
    if(!validateEndBlockForRun()){
        return false;
    }
    for(CodeBlock* root:workspaceCodeRootsFromScene()){
        CodeBlock* clone=deserializeCodeBlock(serializeCodeBlock(root));
        if(clone==nullptr){
            continue;
        }
        runtimeCodeRoots.push_back(clone);
        collectRuntimeCodeSnapshot(clone);
    }
    runtimeCodeSnapshotActive=true;
    runtimeEndReached=false;
    return runtimeStartBlock!=nullptr;
}

vector<CodeBlock*> workspaceCodeRootsFromScene(){
    vector<CodeBlock*> roots;
    if(appScene==nullptr){
        return roots;
    }
    for(QGraphicsItem* item:appScene->items()){
        CodeBlock* block=dynamic_cast<CodeBlock*>(item);
        if(block==nullptr||block->isbase||block->scrollArea!=scrollWorkspace){
            continue;
        }
        if(block->pre!=nullptr||block->insideParent!=nullptr){
            continue;
        }
        if(std::find(roots.begin(),roots.end(),block)==roots.end()){
            roots.push_back(block);
        }
    }
    return roots;
}

vector<CodeBlock*> workspaceCodeItemsFromScene(){
    vector<CodeBlock*> items;
    if(appScene==nullptr){
        return items;
    }
    for(QGraphicsItem* item:appScene->items()){
        CodeBlock* block=dynamic_cast<CodeBlock*>(item);
        if(block==nullptr||block->isbase||block->scrollArea!=scrollWorkspace){
            continue;
        }
        if(std::find(items.begin(),items.end(),block)==items.end()){
            items.push_back(block);
        }
    }
    return items;
}

vector<FloatBlock*> workspaceFloatRootsFromScene(){
    vector<FloatBlock*> roots;
    if(appScene==nullptr){
        return roots;
    }
    for(QGraphicsItem* item:appScene->items()){
        FloatBlock* block=dynamic_cast<FloatBlock*>(item);
        if(block==nullptr||block->isbase||block->parentItem()!=nullptr||
           block->scrollArea!=scrollWorkspace){
            continue;
        }
        if(std::find(roots.begin(),roots.end(),block)==roots.end()){
            roots.push_back(block);
        }
    }
    return roots;
}

void rebuildCodeRegistryFromScene(){
    codeBlocks.clear();
    currentStartBlock=nullptr;
    customHatBlocks.clear();
    if(appScene==nullptr){
        return;
    }
    for(QGraphicsItem* item:appScene->items()){
        CodeBlock* block=dynamic_cast<CodeBlock*>(item);
        if(block==nullptr||block->isbase||block->scrollArea!=scrollWorkspace){
            continue;
        }
        codeBlocks.push_back(block);
        StartBlock* start=dynamic_cast<StartBlock*>(block);
        if(start!=nullptr){
            currentStartBlock=start;
        }
        CustomHatBlock* customHat=dynamic_cast<CustomHatBlock*>(block);
        if(customHat!=nullptr){
            customHatBlocks[customHat->customName]=customHat;
        }
    }
}

void rebuildFloatBlockRegistryFromScene(){
    floatBlocks.clear();
    if(appScene==nullptr){
        return;
    }
    for(QGraphicsItem* item:appScene->items()){
        FloatBlock* block=dynamic_cast<FloatBlock*>(item);
        if(block==nullptr||block->parentItem()!=nullptr){//floatBlocks存放:floatBase,freeFloat
            continue;
        }
        floatBlocks.push_back(block);
    }
}

QJsonObject serializeWorkspace(){
    QJsonObject root;
    root["levelNumber"]=level::activeLevelNumber();
    QJsonArray codeRoots;
    for(CodeBlock* block:workspaceCodeRootsFromScene()){
        codeRoots.append(serializeCodeBlock(block));
    }
    QJsonArray freeFloats;
    for(FloatBlock* block:workspaceFloatRootsFromScene()){
        QJsonObject object;
        object["value"]=serializeFloatBlock(block);
        QPointF stage=sceneToStagePos(scrollWorkspace,block->pos());
        object["x"]=stage.x();
        object["y"]=stage.y();
        freeFloats.append(object);
    }
    QJsonArray variables;
    for(const auto&item:runtimeState.variables())
    {
        QJsonObject object;
        object["name"]=QString::fromStdString(item.first);
        object["value"]=item.second;
        variables.append(object);
    }
    QJsonArray lists;
    for(const auto&item:runtimeState.lists())
    {
        QJsonObject object;
        object["name"]=QString::fromStdString(item.first);
        QJsonArray values;
        for(const double& it:item.second)
        {
            values.append(it);
        }
        object["values"]=values;
        lists.append(object);
    }
    QJsonArray readOnlyVariables;
    for(const auto&item:runtimeState.constVariables())
    {
        readOnlyVariables.append(QString::fromStdString(item));
    }
    QJsonArray readOnlyLists;
    for(const auto&item:runtimeState.constLists())
    {
        readOnlyLists.append(QString::fromStdString(item));
    }
    root["codeRoots"]=codeRoots;
    root["freeFloats"]=freeFloats;
    root["variables"]=variables;
    root["lists"]=lists;
    root["readOnlyVariables"]=readOnlyVariables;
    root["readOnlyLists"]=readOnlyLists;
    return root;
}

void clearWorkspaceForUndo(){
    clearContextMenu();
    vector<CodeBlock*> codeItems=workspaceCodeItemsFromScene();
    currentStartBlock=nullptr;
    for(CodeBlock* block:codeItems){
        if(block==nullptr){
            continue;
        }
        if(block->shadow!=nullptr&&block->shadow->scene()!=nullptr){
            block->shadow->scene()->removeItem(block->shadow);
        }
        if(block->scene()!=nullptr){
            block->scene()->removeItem(block);
        }
    }
    vector<FloatBlock*> freeFloats=workspaceFloatRootsFromScene();
    for(FloatBlock* block:freeFloats){
        if(block==nullptr){
            continue;
        }
        clearAbsorbShadow(block);
        if(block->scene()!=nullptr){
            block->scene()->removeItem(block);
        }
    }
    rebuildCodeRegistryFromScene();
    rebuildFloatBlockRegistryFromScene();
}

void clearWorkspaceAndCacheOnExit(){
    stopProgram();
    runtimeState.clearAll();
    clearDataLockedNames();
    if(appScene!=nullptr){
        if(player!=nullptr&&player->scene()==appScene){
            appScene->removeItem(player);
        }
        appScene->clear();
    }
    clearUndoCache();
    resetSceneGlobals();
}

void refreshVariableToolbox();

void restoreWorkspace(const QJsonObject& root){//load workplace by Json file
    restoringUndo=true;
    clearWorkspaceForUndo();
    QJsonArray codeRoots=root["codeRoots"].toArray();
    for(const QJsonValue& value:codeRoots){
        QJsonObject object=value.toObject();
        CodeBlock* block=deserializeCodeBlock(object);
        if(block==nullptr){
            continue;
        }
        QPointF stagePos(object["x"].toDouble(),object["y"].toDouble());
        setCodeBlockStagePos(block,scrollWorkspace,stagePos);
        if(dynamic_cast<StartBlock*>(block)!=nullptr){
            currentStartBlock=static_cast<StartBlock*>(block);
            if(appScene!=nullptr&&block->scene()==nullptr){
                appScene->addItem(block);
            }
            addCodeTreeToScene(block->next);
        }
        else{
            addCodeTreeToScene(block);
        }
        syncCodeChainFrom(block,block->pos());
        setCodeTreeZValue(block,10);
        rememberCodeTreeStagePos(block,scrollWorkspace);
    }
    QJsonArray freeFloats=root["freeFloats"].toArray();
    for(const QJsonValue& value:freeFloats){
        QJsonObject object=value.toObject();
        FloatBlock* block=deserializeFloatBlock(object["value"].toObject(),nullptr);
        block->scrollArea=scrollWorkspace;
        floatBlocks.push_back(block);
        if(appScene!=nullptr){
            appScene->addItem(block);
        }
        setFloatBlockStagePos(block,scrollWorkspace,QPointF(object["x"].toDouble(),object["y"].toDouble()));
        resetFloatTreeZValue(block);
        setInsertedOperatorInteractivity(block);
    }
    runtimeState.clearAll();
    QJsonArray variables=root["variables"].toArray();
    for(const QJsonValue& variable:variables){
        QJsonObject object=variable.toObject();
        std::string name=object["name"].toString().toStdString();
        runtimeState.forceSetVariable(name,object["value"].toDouble(),runtimeState.variableReadOnly(name));
    }
    QJsonArray lists=root["lists"].toArray();
    for(const QJsonValue& list:lists){
        QJsonObject object=list.toObject();
        std::string name=object["name"].toString().toStdString();
        QJsonArray values=object["values"].toArray();
        vector<double>vValues;
        vValues.clear();
        for(const QJsonValue& value:values)
        {
            vValues.push_back(value.toDouble());
        }
        runtimeState.forceSetList(name,vValues,runtimeState.variableReadOnly(name));
    }
    std::set<std::string>constVariables;
    QJsonArray readOnlyVariables=root["readOnlyVariables"].toArray();
    for(const QJsonValue& readOnlyVariable:readOnlyVariables){
        constVariables.insert(readOnlyVariable.toString().toStdString());
    }
    std::set<std::string>constLists;
    QJsonArray readOnlyLists=root["readOnlyLists"].toArray();
    for(const QJsonValue& readOnlyList:readOnlyLists){
        constLists.insert(readOnlyList.toString().toStdString());
    }
    runtimeState.forceSetReadOnly(constVariables,constLists);
    rebuildCustomCallToolbox();
    refreshVariableToolbox();

    refreshAllControlLayouts();
    restoringUndo=false;
}

QString writeUndoFile(const QJsonObject& snapshot){
    QDir dir(QDir::current());
    dir.mkpath("cache");
    QString path=dir.filePath(QString("cache/undo_%1.json").arg(undoCheckpointId++));
    QFile file(path);
    if(file.open(QIODevice::WriteOnly|QIODevice::Truncate)){
        file.write(QJsonDocument(snapshot).toJson(QJsonDocument::Compact));
        file.close();
    }
    return path;
}

void clearUndoCache(){
    undoCheckpoints.clear();
    undoCheckpointId=0;
    QDir dir(QDir::current().filePath("cache"));
    if(!dir.exists()){
        return;
    }
    QStringList filters;
    filters<<"undo_*.json";
    QFileInfoList files=dir.entryInfoList(filters,QDir::Files);
    for(const QFileInfo& file:files){
        QFile::remove(file.absoluteFilePath());
    }
}

void saveUndoCheckpoint(){
    if(restoringUndo){
        return;
    }
    undoCheckpoints.push_back(writeUndoFile(serializeWorkspace()));
}

void undoLastCheckpoint(){
    if(undoCheckpoints.size()<2){
        return;
    }
    QFile::remove(undoCheckpoints.back());
    undoCheckpoints.pop_back();
    QFile file(undoCheckpoints.back());
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }
    QJsonDocument document=QJsonDocument::fromJson(file.readAll());
    file.close();
    if(!document.isObject()){
        return;
    }
    restoreWorkspace(document.object());
}

void clearContextMenu(){
    for(ContextMenuButton* button:contextMenuButtons){
        if(button->shadow!=nullptr&&button->shadow->scene()!=nullptr){
            button->shadow->scene()->removeItem(button->shadow);
        }
        delete button->shadow;
        button->shadow=nullptr;
        if(button->scene()!=nullptr){
            button->scene()->removeItem(button);
        }
        delete button;
    }
    contextMenuButtons.clear();
}

bool isContextMenuItem(QGraphicsItem* item){
    QGraphicsItem* curr=item;
    while(curr!=nullptr){
        if(dynamic_cast<ContextMenuButton*>(curr)!=nullptr){
            return true;
        }
        curr=curr->parentItem();
    }
    return false;
}

bool isWorkspaceContentItem(QGraphicsItem* item){
    QGraphicsItem* curr=item;
    while(curr!=nullptr){
        if(dynamic_cast<CodeBlock*>(curr)!=nullptr||
           dynamic_cast<FloatBlock*>(curr)!=nullptr||
           dynamic_cast<ScrollSlider*>(curr)!=nullptr||
           dynamic_cast<Button*>(curr)!=nullptr||
           dynamic_cast<TextButton*>(curr)!=nullptr){
            return true;
        }
        curr=curr->parentItem();
    }
    return false;
}

void addContextMenuButton(QString label,QPointF pos,function<void()> action){
    if(appScene==nullptr){
        return;
    }
    ContextMenuButton* button=new ContextMenuButton(label);
    button->setPos(pos);
    button->setZValue(absorbHighlightZ+1000);
    button->onClick=[action](){
        contextMenuButtonPressed=true;
        for(ContextMenuButton* menuButton:contextMenuButtons){
            menuButton->hide();
            menuButton->setAcceptedMouseButtons(Qt::NoButton);
            if(menuButton->shadow!=nullptr){
                menuButton->shadow->hide();
            }
        }
        QTimer::singleShot(0,[action](){
            clearContextMenu();
            if(action){
                action();
            }
            contextMenuButtonPressed=false;
        });
    };
    appScene->addItem(button);
    contextMenuButtons.push_back(button);
}

void showUndoContextMenu(QPointF scenePos){
    clearContextMenu();
    addContextMenuButton("撤销",scenePos,[](){
        undoLastCheckpoint();
    });
}

void refreshEditedFloatName(FloatBlock* block){
    if(block==nullptr){
        return;
    }
    block->refreshSize();
    QGraphicsItem* parent=block->parentItem();
    FloatBlock* floatParent=dynamic_cast<FloatBlock*>(parent);
    if(floatParent!=nullptr){
        refreshFloatAncestors(floatParent);
    }
    CodeBlock* codeParent=dynamic_cast<CodeBlock*>(parent);
    if(codeParent!=nullptr){
        codeParent->refreshSize();
        refreshAllControlLayouts();
        checkEditedCodeWorkspaceWidth(codeParent);
    }
    if(parent==nullptr){
        checkEditedFloatWorkspaceWidth(block);
    }
}

void refreshEditedCodeName(CodeBlock* block){
    if(block==nullptr){
        return;
    }
    block->refreshSize();
    refreshAllControlLayouts();
    checkEditedCodeWorkspaceWidth(block);
}

bool renameRuntimeVariable(const QString& oldName,const QString& newName){
    if(!validVariableName(newName)){
        message::invalidVariableName();
        return false;
    }
    if(newName==oldName){
        return true;
    }
    std::string oldStd=oldName.toStdString();
    std::string newStd=newName.toStdString();
    if(!canRenameVariableName(oldName)){
        message::invalidVariableName();
        return false;
    }
    if(runtimeState.hasVariable(newStd)){
        message::invalidVariableName();
        return false;
    }
    double value=0.0;
    bool exists=runtimeState.getVariable(oldStd,&value);
    if(exists&&runtimeState.variableReadOnly(oldStd)){
        message::invalidVariableName();
        return false;
    }
    if(exists&&!runtimeState.removeVariable(oldStd)){
        message::invalidVariableName();
        return false;
    }
    runtimeState.forceSetVariable(newStd,value,false);
    return true;
}

bool renameRuntimeList(const QString& oldName,const QString& newName){
    if(!validVariableName(newName)){
        message::invalidVariableName();
        return false;
    }
    if(newName==oldName){
        return true;
    }
    std::string oldStd=oldName.toStdString();
    std::string newStd=newName.toStdString();
    if(!canRenameListName(oldName)){
        message::invalidVariableName();
        return false;
    }
    if(runtimeState.hasList(newStd)){
        message::invalidVariableName();
        return false;
    }
    std::vector<double> values;
    auto oldIt=runtimeState.lists().find(oldStd);
    bool exists=oldIt!=runtimeState.lists().end();
    if(exists){
        values=oldIt->second;
    }
    if(exists&&runtimeState.listReadOnly(oldStd)){
        message::invalidVariableName();
        return false;
    }
    if(exists&&!runtimeState.removeList(oldStd)){
        message::invalidVariableName();
        return false;
    }
    runtimeState.forceSetList(newStd,values,false);
    return true;
}

void renameVariableReferencesInFloat(FloatBlock* block,const QString& oldName,const QString& newName){
    if(block==nullptr){
        return;
    }
    VariableBlock* variable=dynamic_cast<VariableBlock*>(block);
    if(variable!=nullptr&&variable->variableName==oldName){
        variable->setVariableName(newName);
        refreshEditedFloatName(variable);
    }
    UnaryOpBlock* unary=dynamic_cast<UnaryOpBlock*>(block);
    if(unary!=nullptr){
        renameVariableReferencesInFloat(unary->slot,oldName,newName);
        return;
    }
    BinaryOpBlock* binary=dynamic_cast<BinaryOpBlock*>(block);
    if(binary!=nullptr){
        renameVariableReferencesInFloat(binary->left,oldName,newName);
        renameVariableReferencesInFloat(binary->right,oldName,newName);
        return;
    }
    ListGetBlock* listGet=dynamic_cast<ListGetBlock*>(block);
    if(listGet!=nullptr){
        renameVariableReferencesInFloat(listGet->index,oldName,newName);
    }
}

void renameListReferencesInFloat(FloatBlock* block,const QString& oldName,const QString& newName){
    if(block==nullptr){
        return;
    }
    ListGetBlock* listGet=dynamic_cast<ListGetBlock*>(block);
    if(listGet!=nullptr){
        if(listGet->listName==oldName){
            listGet->listName=newName;
            listGet->listText->setPlainText(newName);
            refreshEditedFloatName(listGet);
        }
        renameListReferencesInFloat(listGet->index,oldName,newName);
        return;
    }
    ListSizeBlock* listSize=dynamic_cast<ListSizeBlock*>(block);
    if(listSize!=nullptr&&listSize->listName==oldName){
        listSize->listName=newName;
        listSize->listText->setPlainText(newName);
        refreshEditedFloatName(listSize);
    }
    UnaryOpBlock* unary=dynamic_cast<UnaryOpBlock*>(block);
    if(unary!=nullptr){
        renameListReferencesInFloat(unary->slot,oldName,newName);
        return;
    }
    BinaryOpBlock* binary=dynamic_cast<BinaryOpBlock*>(block);
    if(binary!=nullptr){
        renameListReferencesInFloat(binary->left,oldName,newName);
        renameListReferencesInFloat(binary->right,oldName,newName);
    }
}

void renameVariableReferencesInCode(CodeBlock* head,const QString& oldName,const QString& newName){
    CodeBlock* curr=head;
    while(curr!=nullptr){
        SetVariableBlock* setVariable=dynamic_cast<SetVariableBlock*>(curr);
        if(setVariable!=nullptr){
            if(setVariable->variableName==oldName){
                setVariable->variableName=newName;
                setVariable->variableText->setPlainText(newName);
                refreshEditedCodeName(setVariable);
            }
            renameVariableReferencesInFloat(setVariable->value,oldName,newName);
        }
        FloatCodeBlock* floatCode=dynamic_cast<FloatCodeBlock*>(curr);
        if(floatCode!=nullptr){
            renameVariableReferencesInFloat(floatCode->value,oldName,newName);
        }
        OutputBlock* output=dynamic_cast<OutputBlock*>(curr);
        if(output!=nullptr){
            renameVariableReferencesInFloat(output->value,oldName,newName);
        }
        PushListBlock* pushList=dynamic_cast<PushListBlock*>(curr);
        if(pushList!=nullptr){
            renameVariableReferencesInFloat(pushList->value,oldName,newName);
        }
        SetListBlock* setList=dynamic_cast<SetListBlock*>(curr);
        if(setList!=nullptr){
            renameVariableReferencesInFloat(setList->index,oldName,newName);
            renameVariableReferencesInFloat(setList->value,oldName,newName);
        }
        RemoveListItemBlock* removeItem=dynamic_cast<RemoveListItemBlock*>(curr);
        if(removeItem!=nullptr){
            renameVariableReferencesInFloat(removeItem->index,oldName,newName);
        }
        ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(curr);
        if(control!=nullptr){
            renameVariableReferencesInFloat(control->condition,oldName,newName);
            renameVariableReferencesInCode(control->inside,oldName,newName);
        }
        CustomCallBlock* customCall=dynamic_cast<CustomCallBlock*>(curr);
        if(customCall!=nullptr){
            renameVariableReferencesInFloat(customCall->value,oldName,newName);
        }
        curr=curr->next;
    }
}

void renameListReferencesInCode(CodeBlock* head,const QString& oldName,const QString& newName){
    CodeBlock* curr=head;
    while(curr!=nullptr){
        FloatCodeBlock* floatCode=dynamic_cast<FloatCodeBlock*>(curr);
        if(floatCode!=nullptr){
            renameListReferencesInFloat(floatCode->value,oldName,newName);
        }
        OutputBlock* output=dynamic_cast<OutputBlock*>(curr);
        if(output!=nullptr){
            renameListReferencesInFloat(output->value,oldName,newName);
        }
        SetVariableBlock* setVariable=dynamic_cast<SetVariableBlock*>(curr);
        if(setVariable!=nullptr){
            renameListReferencesInFloat(setVariable->value,oldName,newName);
        }
        PushListBlock* pushList=dynamic_cast<PushListBlock*>(curr);
        if(pushList!=nullptr){
            if(pushList->listName==oldName){
                pushList->listName=newName;
                pushList->listText->setPlainText(newName);
                refreshEditedCodeName(pushList);
            }
            renameListReferencesInFloat(pushList->value,oldName,newName);
        }
        SetListBlock* setList=dynamic_cast<SetListBlock*>(curr);
        if(setList!=nullptr){
            if(setList->listName==oldName){
                setList->listName=newName;
                setList->listText->setPlainText(newName);
                refreshEditedCodeName(setList);
            }
            renameListReferencesInFloat(setList->index,oldName,newName);
            renameListReferencesInFloat(setList->value,oldName,newName);
        }
        RemoveListItemBlock* removeItem=dynamic_cast<RemoveListItemBlock*>(curr);
        if(removeItem!=nullptr){
            if(removeItem->listName==oldName){
                removeItem->listName=newName;
                removeItem->listText->setPlainText(newName);
                refreshEditedCodeName(removeItem);
            }
            renameListReferencesInFloat(removeItem->index,oldName,newName);
        }
        ClearListBlock* clearList=dynamic_cast<ClearListBlock*>(curr);
        if(clearList!=nullptr&&clearList->listName==oldName){
            clearList->listName=newName;
            clearList->listText->setPlainText(newName);
            refreshEditedCodeName(clearList);
        }
        ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(curr);
        if(control!=nullptr){
            renameListReferencesInFloat(control->condition,oldName,newName);
            renameListReferencesInCode(control->inside,oldName,newName);
        }
        CustomCallBlock* customCall=dynamic_cast<CustomCallBlock*>(curr);
        if(customCall!=nullptr){
            renameListReferencesInFloat(customCall->value,oldName,newName);
        }
        curr=curr->next;
    }
}

void renameVariableEverywhere(const QString& oldName,const QString& newName){
    for(CodeBlock* root:workspaceCodeRootsFromScene()){
        renameVariableReferencesInCode(root,oldName,newName);
    }
    for(FloatBlock* root:workspaceFloatRootsFromScene()){
        renameVariableReferencesInFloat(root,oldName,newName);
    }
    refreshAllControlLayouts();
    refreshVariableToolbox();
    saveUndoCheckpoint();
}

void renameListEverywhere(const QString& oldName,const QString& newName){
    for(CodeBlock* root:workspaceCodeRootsFromScene()){
        renameListReferencesInCode(root,oldName,newName);
    }
    for(FloatBlock* root:workspaceFloatRootsFromScene()){
        renameListReferencesInFloat(root,oldName,newName);
    }
    refreshAllControlLayouts();
    refreshVariableToolbox();
    saveUndoCheckpoint();
}

void requestRenameVariable(const QString& oldName){
    bool ok=false;
    QString newName=QInputDialog::getText(nullptr,"改变名称","输入名称",
        QLineEdit::Normal,oldName,&ok);
    if(!ok){
        return;
    }
    if(!renameRuntimeVariable(oldName,newName)){
        return;
    }
    if(newName==oldName){
        return;
    }
    renameVariableEverywhere(oldName,newName);
}

void requestRenameList(const QString& oldName){
    bool ok=false;
    QString newName=QInputDialog::getText(nullptr,"改变名称","输入名称",
        QLineEdit::Normal,oldName,&ok);
    if(!ok){
        return;
    }
    if(!renameRuntimeList(oldName,newName)){
        return;
    }
    if(newName==oldName){
        return;
    }
    renameListEverywhere(oldName,newName);
}

void showCodeContextMenu(CodeBlock* block,QPointF scenePos){
    clearContextMenu();
    if(block==nullptr||block->isbase){
        return;
    }
    addContextMenuButton("复制",scenePos,[block](){
        copyCodeContext(block);
    });
    addContextMenuButton("删除",scenePos+QPointF(0,28),[block](){
        deleteCodeContext(block);
    });
}

void showFloatContextMenu(FloatBlock* block,QPointF scenePos){
    clearContextMenu();
    if(block==nullptr){
        return;
    }
    if(block->isbase){
        VariableBlock* variable=dynamic_cast<VariableBlock*>(block);
        if(variable!=nullptr&&canRenameVariableName(variable->variableName)){
            QString oldName=variable->variableName;
            addContextMenuButton("改名",scenePos,[oldName](){
                requestRenameVariable(oldName);
            });
            return;
        }
        if(block->type==-2){
            QString listName=block->text->toPlainText();
            if(canRenameListName(listName)){
                addContextMenuButton("改名",scenePos,[listName](){
                    requestRenameList(listName);
                });
            }
        }
        return;
    }
    QPointF deletePos=scenePos;
    if(!isPlaceholderFloatValue(block)){
        addContextMenuButton("复制",scenePos,[block](){
            copyFloatContext(block);
        });
        deletePos+=QPointF(0,28);
    }
    addContextMenuButton("删除",deletePos,[block](){
        deleteFloatContext(block);
    });
}

void scheduleDeleteCodeChain(CodeBlock* head){
    if(head==nullptr){
        return;
    }
    QTimer::singleShot(0,[head](){
        deleteCodeChain(head);
        refreshAllControlLayouts();
    });
}

void replaceFloatExpressionWithValue(FloatBlock* root){
    if(root==nullptr){
        return;
    }
    QGraphicsItem* parent=root->parentItem();
    if(parent==nullptr){
        deleteFloatBlock(root);
        return;
    }
    if(isPlainNumberBlock(root)){
        root->setData(0);
        root->setMovable(true);
        return;
    }

    QPointF localPos=root->pos();
    FloatBlock* parentBlock=dynamic_cast<FloatBlock*>(parent);
    FloatCodeBlock* codeParent=dynamic_cast<FloatCodeBlock*>(parent);
    OutputBlock* outputParent=dynamic_cast<OutputBlock*>(parent);
    SetVariableBlock* setParent=dynamic_cast<SetVariableBlock*>(parent);
    ControlCodeBlock* controlParent=dynamic_cast<ControlCodeBlock*>(parent);
    CustomCallBlock* customCallParent=dynamic_cast<CustomCallBlock*>(parent);
    FloatBlock* replacementValue=new FloatBlock(0,false,parent);
    replacementValue->setData(0);
    replacementValue->setMovable(true);
    replacementValue->setPos(localPos);

    UnaryOpBlock* unaryParent=dynamic_cast<UnaryOpBlock*>(parentBlock);
    if(unaryParent!=nullptr&&unaryParent->slot==root){
        unaryParent->slot=replacementValue;
    }
    BinaryOpBlock* binaryParent=dynamic_cast<BinaryOpBlock*>(parentBlock);
    if(binaryParent!=nullptr){
        if(binaryParent->left==root){
            binaryParent->left=replacementValue;
        }
        if(binaryParent->right==root){
            binaryParent->right=replacementValue;
        }
    }
    if(codeParent!=nullptr&&codeParent->value==root){
        codeParent->value=replacementValue;
    }
    if(outputParent!=nullptr&&outputParent->value==root){
        outputParent->value=replacementValue;
    }
    if(setParent!=nullptr&&setParent->value==root){
        setParent->value=replacementValue;
    }
    PushListBlock* pushParent=dynamic_cast<PushListBlock*>(parent);
    SetListBlock* setListParent=dynamic_cast<SetListBlock*>(parent);
    ListGetBlock* getParent=dynamic_cast<ListGetBlock*>(parent);
    if(pushParent!=nullptr&&pushParent->value==root){
        pushParent->value=replacementValue;
    }
    if(setListParent!=nullptr){
        if(setListParent->index==root){
            setListParent->index=replacementValue;
        }
        if(setListParent->value==root){
            setListParent->value=replacementValue;
        }
    }
    if(getParent!=nullptr&&getParent->index==root){
        getParent->index=replacementValue;
    }
    if(controlParent!=nullptr&&controlParent->condition==root){
        controlParent->condition=replacementValue;
    }
    if(customCallParent!=nullptr&&customCallParent->value==root){
        customCallParent->value=replacementValue;
    }

    deleteFloatBlock(root);

    if(parentBlock!=nullptr){
        refreshFloatAncestors(parentBlock);
    }
    if(codeParent!=nullptr){
        codeParent->refreshSize();
        refreshAllControlLayouts();
    }
    if(outputParent!=nullptr){
        outputParent->refreshSize();
        refreshAllControlLayouts();
    }
    if(setParent!=nullptr){
        setParent->refreshSize();
        refreshAllControlLayouts();
    }
    if(pushParent!=nullptr){
        pushParent->refreshSize();
        refreshAllControlLayouts();
    }
    if(setListParent!=nullptr){
        setListParent->refreshSize();
        refreshAllControlLayouts();
    }
    if(getParent!=nullptr){
        getParent->refreshSize();
        refreshFloatAncestors(getParent);
    }
    if(controlParent!=nullptr){
        controlParent->refreshSize();
        refreshAllControlLayouts();
    }
    if(customCallParent!=nullptr){
        customCallParent->refreshSize();
        refreshAllControlLayouts();
    }
}

void scheduleRemoveFloatExpression(FloatBlock* block){
    if(block==nullptr){
        return;
    }
    FloatBlock* root=rootFloatBlock(block);
    QTimer::singleShot(0,[root](){
        replaceFloatExpressionWithValue(root);
        refreshAllControlLayouts();
    });
}

void checkEditedCodeWorkspaceWidth(CodeBlock* block){
    if(block==nullptr){
        return;
    }
    CodeBlock* head=codeChainHead(block);
    if(head!=nullptr&&head->scrollArea==scrollWorkspace&&outsideWorkspaceHorizontal(codeTreeSceneRect(head))){
        message::workspaceWidthLimitReached();
    }
}

void checkEditedFloatWorkspaceWidth(FloatBlock* block){
    if(block==nullptr){
        return;
    }
    CodeBlock* owner=containingCodeBlock(block);
    if(owner!=nullptr){
        CodeBlock* head=codeChainHead(owner);
        if(head!=nullptr&&head->scrollArea==scrollWorkspace&&outsideWorkspaceHorizontal(codeTreeSceneRect(head))){
            message::workspaceWidthLimitReached();
            scheduleRemoveFloatExpression(block);
        }
        return;
    }
    if(block->parentItem()==nullptr&&block->scrollArea==scrollWorkspace&&
       outsideWorkspaceHorizontal(block->sceneBoundingRect())){
        message::workspaceWidthLimitReached();
        QTimer::singleShot(0,[block](){
            deleteFloatBlock(block);
        });
    }
}

double codeBlockFloatValue(CodeBlock* block){
    FloatCodeBlock* floatBlock=dynamic_cast<FloatCodeBlock*>(block);
    if(floatBlock!=nullptr&&floatBlock->value!=nullptr){
        return floatBlock->value->getValue();
    }
    OutputBlock* outputBlock=dynamic_cast<OutputBlock*>(block);
    if(outputBlock!=nullptr&&outputBlock->value!=nullptr){
        return outputBlock->value->getValue();
    }
    SetVariableBlock* setBlock=dynamic_cast<SetVariableBlock*>(block);
    if(setBlock!=nullptr&&setBlock->value!=nullptr){
        return setBlock->value->getValue();
    }
    PushListBlock* pushBlock=dynamic_cast<PushListBlock*>(block);
    if(pushBlock!=nullptr&&pushBlock->value!=nullptr){
        return pushBlock->value->getValue();
    }
    SetListBlock* setListBlock=dynamic_cast<SetListBlock*>(block);
    if(setListBlock!=nullptr&&setListBlock->value!=nullptr){
        return setListBlock->value->getValue();
    }
    CustomCallBlock* customCall=dynamic_cast<CustomCallBlock*>(block);
    if(customCall!=nullptr&&customCall->value!=nullptr){
        return customCall->value->getValue();
    }
    ControlCodeBlock* controlBlock=dynamic_cast<ControlCodeBlock*>(block);
    if(controlBlock!=nullptr&&controlBlock->condition!=nullptr){
        return controlBlock->condition->getValue();
    }
    return 0.0;
}

core::BlockExecutor::BlockSnapshot readRuntimeBlock(core::BlockExecutor::Node node){
    core::BlockExecutor::BlockSnapshot snapshot;
    CodeBlock* block=static_cast<CodeBlock*>(node);
    if(block==nullptr){
        return snapshot;
    }
    recordRuntimeStepUse();
    snapshot.type=block->type;
    runtimeSkipCurrentBlock=false;
    bool waitingFrame=(block->type==3||block->type==4)&&
        executorPtr!=nullptr&&executorPtr->waitingOn(node);
    if(isEndCodeBlock(block)){
        runtimeEndReached=true;
    }
    if(!waitingFrame){
        snapshot.value=codeBlockFloatValue(block);
        if(runtimeSkipCurrentBlock){
            snapshot.type=-999;
        }
    }
    snapshot.next=block->next;
    SetVariableBlock* setBlock=dynamic_cast<SetVariableBlock*>(block);
    if(setBlock!=nullptr){
        snapshot.variableName=setBlock->variableName.toStdString();
    }
    PushListBlock* pushBlock=dynamic_cast<PushListBlock*>(block);
    if(pushBlock!=nullptr){
        snapshot.listName=pushBlock->listName.toStdString();
    }
    SetListBlock* setListBlock=dynamic_cast<SetListBlock*>(block);
    if(setListBlock!=nullptr){
        snapshot.listName=setListBlock->listName.toStdString();
        if(setListBlock->index!=nullptr){
            snapshot.indexValue=setListBlock->index->getValue();
        }
    }
    RemoveListItemBlock* removeItemBlock=dynamic_cast<RemoveListItemBlock*>(block);
    if(removeItemBlock!=nullptr){
        snapshot.listName=removeItemBlock->listName.toStdString();
        if(removeItemBlock->index!=nullptr){
            snapshot.indexValue=removeItemBlock->index->getValue();
        }
        int userIndex=static_cast<int>(std::floor(snapshot.indexValue));
        int listLength=runtimeState.listSize(snapshot.listName);
        if(listLength>=0){
            recordRuntimeStepUse(std::max(0,listLength-userIndex));
        }
    }
    ClearListBlock* clearListBlock=dynamic_cast<ClearListBlock*>(block);
    if(clearListBlock!=nullptr){
        snapshot.listName=clearListBlock->listName.toStdString();
    }
    ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(block);
    if(control!=nullptr){
        snapshot.inside=control->inside;
    }
    CustomCallBlock* customCall=dynamic_cast<CustomCallBlock*>(block);
    if(customCall!=nullptr){
        snapshot.customName=customCall->customName.toStdString();
        const auto& hats=runtimeCodeSnapshotActive?runtimeCustomHatBlocks:customHatBlocks;
        auto it=hats.find(customCall->customName);
        if(it!=hats.end()&&it->second!=nullptr){
            snapshot.callTarget=it->second;
        }
    }
    OutputBlock* outputBlock=dynamic_cast<OutputBlock*>(block);
    if(outputBlock!=nullptr){
        snapshot.messageText=outputBlock->messageText.toStdString();
    }
    return snapshot;
}

QPointF scrollAreaOrigin(int area){
    if(area==scrollToolbox){
        return QPointF(toolboxX,toolboxY);
    }
    if(area==scrollWorkspace){
        return QPointF(workspaceX,workspaceY);
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

int codeChainHeight(CodeBlock* head){
    int height=0;
    CodeBlock* curr=head;
    while(curr!=nullptr){
        height+=curr->wid;
        curr=curr->next;
    }
    return height;
}

void setCodeChainMoving(CodeBlock* head,bool moving){
    CodeBlock* curr=head;
    while(curr!=nullptr){
        curr->ismoving=moving;
        ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(curr);
        if(control!=nullptr){
            setCodeChainMoving(control->inside,moving);
        }
        curr=curr->next;
    }
}

void syncCodeChainFrom(CodeBlock* head,QPointF startPos){
    CodeBlock* curr=head;
    QPointF currPos=startPos;
    while(curr!=nullptr){
        curr->setPos(currPos);
        ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(curr);
        if(control!=nullptr){
            syncControlInside(control);
        }
        currPos+=QPointF(0,curr->wid);
        curr=curr->next;
    }
}

void syncInsideCodeChain(ControlCodeBlock* owner){
    if(owner==nullptr){
        return;
    }
    CodeBlock* curr=owner->inside;
    QPointF currPos=owner->pos()+QPointF(owner->leftWidth,owner->topHeight);
    while(curr!=nullptr){
        curr->insideParent=owner;
        curr->setPos(currPos);
        ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(curr);
        if(control!=nullptr){
            syncControlInside(control);
        }
        currPos+=QPointF(0,curr->wid);
        curr=curr->next;
    }
}

void offsetCodeChain(CodeBlock* head,QPointF offset){
    CodeBlock* curr=head;
    while(curr!=nullptr){
        curr->setPos(curr->pos()+offset);
        ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(curr);
        if(control!=nullptr){
            offsetCodeChain(control->inside,offset);
        }
        curr=curr->next;
    }
}

ControlCodeBlock* findInsideOwner(CodeBlock* block){
    for(CodeBlock* candidate:codeBlocks){
        ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(candidate);
        if(control==nullptr){
            continue;
        }
        CodeBlock* curr=control->inside;
        while(curr!=nullptr){
            if(curr==block){
                return control;
            }
            curr=curr->next;
        }
    }
    return nullptr;
}

void refreshControlFromInside(ControlCodeBlock* control){
    if(control==nullptr){
        return;
    }
    CodeBlock* curr=control->inside;
    while(curr!=nullptr){
        ControlCodeBlock* child=dynamic_cast<ControlCodeBlock*>(curr);
        if(child!=nullptr&&!child->dragging&&!child->ismoving){
            refreshControlFromInside(child);
        }
        curr=curr->next;
    }
    control->innerHeight=control->inside==nullptr?30:codeChainHeight(control->inside);
    control->refreshSize();
    syncControlInside(control);
}

void syncControlInside(ControlCodeBlock* control){
    if(control==nullptr||control->inside==nullptr){
        return;
    }
    syncInsideCodeChain(control);
}

void previewControlAncestorGrowth(ControlCodeBlock* control){
    ControlCodeBlock* curr=findInsideOwner(control);
    while(curr!=nullptr){
        int oldWid=curr->wid;
        curr->innerHeight=curr->inside==nullptr?30:codeChainHeight(curr->inside);
        curr->refreshSize();
        int delta=curr->wid-oldWid;
        if(delta!=0&&curr->next!=nullptr){
            offsetCodeChain(curr->next,QPointF(0,delta));
        }
        curr=findInsideOwner(curr);
    }
}

void previewControlInsideInsert(ControlCodeBlock* control,int insertHeight){
    if(control==nullptr){
        return;
    }
    int oldWid=control->wid;
    control->innerHeight=std::max(30,codeChainHeight(control->inside)+insertHeight);
    control->refreshSize();
    if(control->next!=nullptr){
        offsetCodeChain(control->next,QPointF(0,control->wid-oldWid));
    }
    previewControlAncestorGrowth(control);
}

void refreshAllControlLayouts(){
    for(CodeBlock* block:codeBlocks){
        ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(block);
        if(control!=nullptr&&!control->dragging&&!control->ismoving){
            refreshControlFromInside(control);
        }
    }
    for(CodeBlock* block:codeBlocks){
        if(block!=nullptr&&block->pre==nullptr&&block->insideParent==nullptr&&
           !block->dragging&&!block->ismoving){
            syncCodeChainFrom(block,block->pos());
        }
    }
    if(currentStartBlock!=nullptr&&!currentStartBlock->dragging&&!currentStartBlock->ismoving){
        syncCodeChainFrom(currentStartBlock,currentStartBlock->pos());
    }
}

ControlCodeBlock* findInsideAbsorbTarget(CodeBlock* moving){
    if(moving==nullptr){
        return nullptr;
    }
    ControlCodeBlock* bestTarget=nullptr;
    qreal bestDistance=20;
    for(CodeBlock* block:codeBlocks){
        ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(block);
        if(control==nullptr||control->isbase||control->ismoving||control==moving){
            continue;
        }
        QPointF slotPos=control->pos()+QPointF(control->leftWidth,control->topHeight);
        qreal distance=QLineF(moving->pos(),slotPos).length();
        if(distance<bestDistance){
            bestDistance=distance;
            bestTarget=control;
        }
    }
    return bestTarget;
}

bool isFirstInsideBlock(CodeBlock* block){
    if(block==nullptr||block->insideParent==nullptr){
        return false;
    }
    return block->insideParent->inside==block;
}

bool canTopOnlyBlockAttachBefore(CodeBlock* block){
    if(block==nullptr){
        return false;
    }
    return block->pre==nullptr&&block->insideParent==nullptr;
}

void rememberCodeTreeStagePos(CodeBlock* head,int area){
    CodeBlock* curr=head;
    while(curr!=nullptr){
        rememberCodeBlockStagePos(curr,area);
        ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(curr);
        if(control!=nullptr){
            rememberCodeTreeStagePos(control->inside,area);
        }
        curr=curr->next;
    }
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
    refreshAllControlLayouts();
}

void refreshVariableToolbox(){
    for(VariableBlock* block:variableBaseBlocks){
        deleteFloatBlock(block);
    }
    variableBaseBlocks.clear();
    for(FloatBlock* block:listLabelBaseBlocks){
        deleteFloatBlock(block);
    }
    listLabelBaseBlocks.clear();
    if(appScene==nullptr){
        return;
    }
    int y=variableToolboxStartY;
    for(const auto& item:runtimeState.variables()){
        QString name=QString::fromStdString(item.first);
        VariableBlock* block=new VariableBlock(name,true);
        setFloatBlockStagePos(block,scrollToolbox,QPointF(20,y));
        block->setZValue(10);
        block->setAcceptedMouseButtons(canRenameVariableName(name)?
            (Qt::LeftButton|Qt::RightButton):Qt::LeftButton);
        appScene->addItem(block);
        floatBlocks.push_back(block);
        variableBaseBlocks.push_back(block);
        y+=block->wid+toolboxFloatBlockGap;
    }
    if(variableSetBaseBlock!=nullptr){
        setCodeBlockStagePos(variableSetBaseBlock,scrollToolbox,QPointF(20,y));
        y+=variableSetBaseBlock->wid+toolboxCodeBlockGap;
    }
    if(variableIncreaseBaseBlock!=nullptr){
        setCodeBlockStagePos(variableIncreaseBaseBlock,scrollToolbox,QPointF(20,y));
        y+=variableIncreaseBaseBlock->wid+toolboxCodeBlockGap;
    }
    for(const auto& item:runtimeState.lists()){
        QString name=QString::fromStdString(item.first);
        FloatBlock* labelBlock=new FloatBlock(-2,true);
        labelBlock->text->setPlainText(name);
        labelBlock->refreshSize();
        labelBlock->setBrush(listColor());
        labelBlock->setMovable(false);
        labelBlock->setAcceptedMouseButtons(canRenameListName(name)?
            Qt::RightButton:Qt::NoButton);
        setFloatBlockStagePos(labelBlock,scrollToolbox,QPointF(20,y));
        labelBlock->setZValue(10);
        appScene->addItem(labelBlock);
        floatBlocks.push_back(labelBlock);
        listLabelBaseBlocks.push_back(labelBlock);
        y+=labelBlock->wid+toolboxFloatBlockGap;
    }
    for(FloatBlock* block:listFloatBaseBlocks){
        if(block==nullptr){
            continue;
        }
        setFloatBlockStagePos(block,scrollToolbox,QPointF(20,y));
        y+=block->wid+toolboxFloatBlockGap;
    }
    for(CodeBlock* block:listCodeBaseBlocks){
        if(block==nullptr){
            continue;
        }
        setCodeBlockStagePos(block,scrollToolbox,QPointF(20,y));
        y+=block->wid+toolboxCodeBlockGap;
    }
    for(CustomCallBlock* block:customCallBaseBlocks){
        if(block==nullptr){
            continue;
        }
        setCodeBlockStagePos(block,scrollToolbox,QPointF(20,y));
        y+=block->wid+toolboxCodeBlockGap;
    }
    updateToolboxScrollRange();
    syncScrollArea(scrollToolbox);
}

void updateToolboxScrollRange(){
    qreal bottom=0;
    for(CodeBlock* block:baseCodeBlocks){
        if(block!=nullptr&&block->scrollArea==scrollToolbox){
            bottom=std::max<qreal>(bottom,block->stagePos.y()+block->wid);
        }
    }
    for(FloatBlock* block:floatBlocks){
        if(block!=nullptr&&block->parentItem()==nullptr&&block->scrollArea==scrollToolbox){
            bottom=std::max<qreal>(bottom,block->stagePos.y()+block->wid);
        }
    }
    toolboxMaxScrollY=std::max<qreal>(0,bottom+20-toolboxHeight);
    if(toolboxSlider!=nullptr){
        toolboxSlider->setValue(toolboxSlider->value());
    }
}

void deleteCodeBlock(CodeBlock* block){
    if(block==nullptr){
        return;
    }
    if(block==currentStartBlock){
        currentStartBlock=nullptr;
    }
    CustomHatBlock* customHat=dynamic_cast<CustomHatBlock*>(block);
    if(customHat!=nullptr&&!suppressCustomReferenceRemoval){
        removeCustomBlockReferences(customHat->customName,customHat);
    }
    if(customHat!=nullptr){
        auto it=customHatBlocks.find(customHat->customName);
        if(it!=customHatBlocks.end()&&it->second==customHat){
            customHatBlocks.erase(it);
        }
    }
    if(block->insideParent!=nullptr&&block->insideParent->inside==block){
        ControlCodeBlock* owner=block->insideParent;
        owner->inside=nullptr;
        block->insideParent=nullptr;
        refreshControlFromInside(owner);
    }
    ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(block);
    if(control!=nullptr){
        CodeBlock* curr=control->inside;
        while(curr!=nullptr){
            CodeBlock* next=curr->next;
            deleteCodeBlock(curr);
            curr=next;
        }
        control->inside=nullptr;
    }
    codeBlocks.erase(
        std::remove(codeBlocks.begin(),codeBlocks.end(),block),
        codeBlocks.end()
    );
    eraseCodeEmbeddedFloatRecords(block);
    if(block->shadow!=nullptr){
        if(block->shadow->scene()!=nullptr){
            block->shadow->scene()->removeItem(block->shadow);
        }
        delete block->shadow;
        block->shadow=nullptr;
    }
    if(block->scene()!=nullptr){
        block->scene()->removeItem(block);
    }
    delete block;
}

void deleteFloatBlock(FloatBlock* block){
    if(block==nullptr){
        return;
    }
    clearAbsorbShadow(block);
    eraseFloatTreeRecords(block);
    UnaryOpBlock* unary=dynamic_cast<UnaryOpBlock*>(block);
    if(unary!=nullptr){
        deleteFloatBlock(unary->slot);
        unary->slot=nullptr;
    }
    BinaryOpBlock* binary=dynamic_cast<BinaryOpBlock*>(block);
    if(binary!=nullptr){
        deleteFloatBlock(binary->left);
        deleteFloatBlock(binary->right);
        binary->left=nullptr;
        binary->right=nullptr;
    }
    ListGetBlock* listGet=dynamic_cast<ListGetBlock*>(block);
    if(listGet!=nullptr){
        deleteFloatBlock(listGet->index);
        listGet->index=nullptr;
    }
    if(block->parentItem()!=nullptr){
        block->setParentItem(nullptr);
    }
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

void eraseFloatTreeRecords(FloatBlock* block){
    if(block==nullptr){
        return;
    }
    UnaryOpBlock* unary=dynamic_cast<UnaryOpBlock*>(block);
    if(unary!=nullptr){
        eraseFloatTreeRecords(unary->slot);
    }
    BinaryOpBlock* binary=dynamic_cast<BinaryOpBlock*>(block);
    if(binary!=nullptr){
        eraseFloatTreeRecords(binary->left);
        eraseFloatTreeRecords(binary->right);
    }
    ListGetBlock* listGet=dynamic_cast<ListGetBlock*>(block);
    if(listGet!=nullptr){
        eraseFloatTreeRecords(listGet->index);
    }
    eraseFloatBlockRecord(block);
}

void eraseCodeEmbeddedFloatRecords(CodeBlock* block){
    if(block==nullptr){
        return;
    }
    FloatCodeBlock* floatCode=dynamic_cast<FloatCodeBlock*>(block);
    if(floatCode!=nullptr){
        eraseFloatTreeRecords(floatCode->value);
    }
    OutputBlock* outputBlock=dynamic_cast<OutputBlock*>(block);
    if(outputBlock!=nullptr){
        eraseFloatTreeRecords(outputBlock->value);
    }
    SetVariableBlock* setCode=dynamic_cast<SetVariableBlock*>(block);
    if(setCode!=nullptr){
        eraseFloatTreeRecords(setCode->value);
    }
    PushListBlock* pushList=dynamic_cast<PushListBlock*>(block);
    if(pushList!=nullptr){
        eraseFloatTreeRecords(pushList->value);
    }
    SetListBlock* setList=dynamic_cast<SetListBlock*>(block);
    if(setList!=nullptr){
        eraseFloatTreeRecords(setList->index);
        eraseFloatTreeRecords(setList->value);
    }
    RemoveListItemBlock* removeItem=dynamic_cast<RemoveListItemBlock*>(block);
    if(removeItem!=nullptr){
        eraseFloatTreeRecords(removeItem->index);
    }
    ControlCodeBlock* control=dynamic_cast<ControlCodeBlock*>(block);
    if(control!=nullptr){
        eraseFloatTreeRecords(control->condition);
    }
    CustomHatBlock* customHat=dynamic_cast<CustomHatBlock*>(block);
    if(customHat!=nullptr){
        eraseFloatTreeRecords(customHat->parameterBlock);
    }
    CustomCallBlock* customCall=dynamic_cast<CustomCallBlock*>(block);
    if(customCall!=nullptr){
        eraseFloatTreeRecords(customCall->value);
    }
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

void CustomParamBlock::mousePressEvent(QGraphicsSceneMouseEvent* event){
    if(parentItem()==nullptr||dynamic_cast<CustomHatBlock*>(parentItem())==nullptr){
        FloatBlock::mousePressEvent(event);
        return;
    }
    if(event->button()==Qt::RightButton){
        event->accept();
        return;
    }
    if(event->button()!=Qt::LeftButton||scene()==nullptr){
        event->ignore();
        return;
    }
    qreal dragZ=nextDragZ();
    FloatBlock* tempBlock=copy();
    floatBlocks.push_back(tempBlock);
    scene()->addItem(tempBlock);
    tempBlock->setPos(scenePos());
    tempBlock->setZValue(dragZ);
    setFloatTreeZValue(tempBlock,dragZ);
    tempBlock->dragging=true;
    tempBlock->scrollArea=scrollNone;
    tempBlock->moved=false;
    tempBlock->mouseOffset=event->pos();
    tempBlock->grabMouse();
    event->accept();
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
        refreshAllControlLayouts();
    }
}

void setInsertedOperatorInteractivity(FloatBlock* block){
    if(block==nullptr){
        return;
    }
    block->setMovable(true);
    UnaryOpBlock* unary=dynamic_cast<UnaryOpBlock*>(block);
    if(unary!=nullptr){
        setInsertedOperatorInteractivity(unary->slot);
        return;
    }
    BinaryOpBlock* binary=dynamic_cast<BinaryOpBlock*>(block);
    if(binary!=nullptr){
        setInsertedOperatorInteractivity(binary->left);
        setInsertedOperatorInteractivity(binary->right);
        return;
    }
    ListGetBlock* listGet=dynamic_cast<ListGetBlock*>(block);
    if(listGet!=nullptr){
        setInsertedOperatorInteractivity(listGet->index);
    }
}

void setFloatTreeZValue(FloatBlock* block,qreal z){
    if(block==nullptr){
        return;
    }
    block->setZValue(z);
    UnaryOpBlock* unary=dynamic_cast<UnaryOpBlock*>(block);
    if(unary!=nullptr){
        setFloatTreeZValue(unary->slot,z+1);
        return;
    }
    BinaryOpBlock* binary=dynamic_cast<BinaryOpBlock*>(block);
    if(binary!=nullptr){
        setFloatTreeZValue(binary->left,z+1);
        setFloatTreeZValue(binary->right,z+1);
        return;
    }
    ListGetBlock* listGet=dynamic_cast<ListGetBlock*>(block);
    if(listGet!=nullptr){
        setFloatTreeZValue(listGet->index,z+1);
    }
}

void resetFloatTreeZValue(FloatBlock* block){
    setFloatTreeZValue(block,10);
}

void setCodeTreeZValue(CodeBlock* block,qreal z){
    CodeBlock* curr=block;
    while(curr!=nullptr){
        curr->setZValue(z);
        FloatCodeBlock* floatCode=dynamic_cast<FloatCodeBlock*>(curr);
        if(floatCode!=nullptr){
            setFloatTreeZValue(floatCode->value,z+1);
        }
        OutputBlock* outputBlock=dynamic_cast<OutputBlock*>(curr);
        if(outputBlock!=nullptr){
            setFloatTreeZValue(outputBlock->value,z+1);
        }
        SetVariableBlock* setCode=dynamic_cast<SetVariableBlock*>(curr);
        if(setCode!=nullptr){
            setFloatTreeZValue(setCode->value,z+1);
        }
        PushListBlock* pushList=dynamic_cast<PushListBlock*>(curr);
        if(pushList!=nullptr){
            setFloatTreeZValue(pushList->value,z+1);
        }
        SetListBlock* setList=dynamic_cast<SetListBlock*>(curr);
        if(setList!=nullptr){
            setFloatTreeZValue(setList->index,z+1);
            setFloatTreeZValue(setList->value,z+1);
        }
        RemoveListItemBlock* removeItem=dynamic_cast<RemoveListItemBlock*>(curr);
        if(removeItem!=nullptr){
            setFloatTreeZValue(removeItem->index,z+1);
        }
        ControlCodeBlock* controlCode=dynamic_cast<ControlCodeBlock*>(curr);
        if(controlCode!=nullptr){
            setFloatTreeZValue(controlCode->condition,z+1);
            setCodeTreeZValue(controlCode->inside,z+1);
        }
        CustomHatBlock* customHat=dynamic_cast<CustomHatBlock*>(curr);
        if(customHat!=nullptr){
            setFloatTreeZValue(customHat->parameterBlock,z+1);
        }
        CustomCallBlock* customCall=dynamic_cast<CustomCallBlock*>(curr);
        if(customCall!=nullptr){
            setFloatTreeZValue(customCall->value,z+1);
        }
        curr=curr->next;
    }
}

FloatBlock* detachOperatorFromParent(FloatBlock* moving){
    if(moving==nullptr){
        return nullptr;
    }
    FloatBlock* parentBlock=dynamic_cast<FloatBlock*>(moving->parentItem());
    FloatCodeBlock* codeParent=dynamic_cast<FloatCodeBlock*>(moving->parentItem());
    OutputBlock* outputParent=dynamic_cast<OutputBlock*>(moving->parentItem());
    SetVariableBlock* setParent=dynamic_cast<SetVariableBlock*>(moving->parentItem());
    PushListBlock* pushParent=dynamic_cast<PushListBlock*>(moving->parentItem());
    SetListBlock* setListParent=dynamic_cast<SetListBlock*>(moving->parentItem());
    RemoveListItemBlock* removeItemParent=dynamic_cast<RemoveListItemBlock*>(moving->parentItem());
    ControlCodeBlock* controlParent=dynamic_cast<ControlCodeBlock*>(moving->parentItem());
    CustomCallBlock* customCallParent=dynamic_cast<CustomCallBlock*>(moving->parentItem());
    if(parentBlock==nullptr&&codeParent==nullptr&&outputParent==nullptr&&setParent==nullptr&&
       pushParent==nullptr&&setListParent==nullptr&&removeItemParent==nullptr&&controlParent==nullptr&&
       customCallParent==nullptr){
        return nullptr;
    }

    QPointF oldScenePos=moving->scenePos();
    QGraphicsItem* parentItem=nullptr;
    if(parentBlock!=nullptr){
        parentItem=parentBlock;
    }
    else if(codeParent!=nullptr){
        parentItem=codeParent;
    }
    else if(outputParent!=nullptr){
        parentItem=outputParent;
    }
    else if(setParent!=nullptr){
        parentItem=setParent;
    }
    else if(pushParent!=nullptr){
        parentItem=pushParent;
    }
    else if(setListParent!=nullptr){
        parentItem=setListParent;
    }
    else if(removeItemParent!=nullptr){
        parentItem=removeItemParent;
    }
    else if(customCallParent!=nullptr){
        parentItem=customCallParent;
    }
    else{
        parentItem=controlParent;
    }
    FloatBlock* replacementValue=new FloatBlock(0,false,parentItem);
    replacementValue->setData(0);
    replacementValue->setMovable(true);

    UnaryOpBlock* unaryParent=dynamic_cast<UnaryOpBlock*>(parentBlock);
    if(unaryParent!=nullptr&&unaryParent->slot==moving){
        unaryParent->slot=replacementValue;
    }
    BinaryOpBlock* binaryParent=dynamic_cast<BinaryOpBlock*>(parentBlock);
    if(binaryParent!=nullptr){
        if(binaryParent->left==moving){
            binaryParent->left=replacementValue;
        }
        if(binaryParent->right==moving){
            binaryParent->right=replacementValue;
        }
    }
    ListGetBlock* getParent=dynamic_cast<ListGetBlock*>(parentBlock);
    if(getParent!=nullptr&&getParent->index==moving){
        getParent->index=replacementValue;
    }
    if(codeParent!=nullptr&&codeParent->value==moving){
        codeParent->value=replacementValue;
    }
    if(outputParent!=nullptr&&outputParent->value==moving){
        outputParent->value=replacementValue;
    }
    if(setParent!=nullptr&&setParent->value==moving){
        setParent->value=replacementValue;
    }
    if(pushParent!=nullptr&&pushParent->value==moving){
        pushParent->value=replacementValue;
    }
    if(setListParent!=nullptr){
        if(setListParent->index==moving){
            setListParent->index=replacementValue;
        }
        if(setListParent->value==moving){
            setListParent->value=replacementValue;
        }
    }
    if(removeItemParent!=nullptr&&removeItemParent->index==moving){
        removeItemParent->index=replacementValue;
    }
    if(controlParent!=nullptr&&controlParent->condition==moving){
        controlParent->condition=replacementValue;
    }
    if(customCallParent!=nullptr&&customCallParent->value==moving){
        customCallParent->value=replacementValue;
    }

    moving->setParentItem(nullptr);
    moving->setPos(oldScenePos);
    moving->setMovable(true);
    eraseFloatTreeRecords(moving);
    floatBlocks.push_back(moving);
    if(parentBlock!=nullptr){
        refreshFloatAncestors(parentBlock);
    }
    if(codeParent!=nullptr){
        codeParent->refreshSize();
        refreshAllControlLayouts();
    }
    if(outputParent!=nullptr){
        outputParent->refreshSize();
        refreshAllControlLayouts();
    }
    if(setParent!=nullptr){
        setParent->refreshSize();
        refreshAllControlLayouts();
    }
    if(pushParent!=nullptr){
        pushParent->refreshSize();
        refreshAllControlLayouts();
    }
    if(setListParent!=nullptr){
        setListParent->refreshSize();
        refreshAllControlLayouts();
    }
    if(removeItemParent!=nullptr){
        removeItemParent->refreshSize();
        refreshAllControlLayouts();
    }
    if(controlParent!=nullptr){
        controlParent->refreshSize();
        refreshAllControlLayouts();
    }
    if(customCallParent!=nullptr){
        customCallParent->refreshSize();
        refreshAllControlLayouts();
    }
    showAbsorbShadow(moving,replacementValue);
    moving->absorbTarget=replacementValue;
    return replacementValue;
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
        collectFloatValueTargets(unary->slot,moving,targets);
        return;
    }
    BinaryOpBlock* binary=dynamic_cast<BinaryOpBlock*>(root);
    if(binary!=nullptr){
        collectFloatValueTargets(binary->left,moving,targets);
        collectFloatValueTargets(binary->right,moving,targets);
        return;
    }
    ListGetBlock* listGet=dynamic_cast<ListGetBlock*>(root);
    if(listGet!=nullptr){
        collectFloatValueTargets(listGet->index,moving,targets);
    }
}

FloatBlock* findAbsorbTarget(FloatBlock* moving){
    if(moving==nullptr){
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
        OutputBlock* outputBlock=dynamic_cast<OutputBlock*>(block);
        if(outputBlock!=nullptr){
            collectFloatValueTargets(outputBlock->value,moving,targets);
        }
        SetVariableBlock* setCode=dynamic_cast<SetVariableBlock*>(block);
        if(setCode!=nullptr){
            collectFloatValueTargets(setCode->value,moving,targets);
        }
        PushListBlock* pushCode=dynamic_cast<PushListBlock*>(block);
        if(pushCode!=nullptr){
            collectFloatValueTargets(pushCode->value,moving,targets);
        }
        SetListBlock* setListCode=dynamic_cast<SetListBlock*>(block);
        if(setListCode!=nullptr){
            collectFloatValueTargets(setListCode->index,moving,targets);
            collectFloatValueTargets(setListCode->value,moving,targets);
        }
        RemoveListItemBlock* removeItemCode=dynamic_cast<RemoveListItemBlock*>(block);
        if(removeItemCode!=nullptr){
            collectFloatValueTargets(removeItemCode->index,moving,targets);
        }
        ControlCodeBlock* controlCode=dynamic_cast<ControlCodeBlock*>(block);
        if(controlCode!=nullptr){
            collectFloatValueTargets(controlCode->condition,moving,targets);
        }
        CustomCallBlock* customCall=dynamic_cast<CustomCallBlock*>(block);
        if(customCall!=nullptr){
            collectFloatValueTargets(customCall->value,moving,targets);
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
    qreal shadowZ=std::max(absorbHighlightZ,moving->zValue()+100);
    moving->absorbShadow->setZValue(shadowZ);
    if(moving->absorbShadow->scene()==nullptr){
        moving->scene()->addItem(moving->absorbShadow);
    }
    moving->absorbShadow->setZValue(shadowZ);
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
    if(moving==nullptr||target==nullptr){
        return;
    }
    clearAbsorbShadow(moving);
    FloatBlock* parentBlock=dynamic_cast<FloatBlock*>(target->parentItem());
    FloatCodeBlock* codeParent=dynamic_cast<FloatCodeBlock*>(target->parentItem());
    OutputBlock* outputParent=dynamic_cast<OutputBlock*>(target->parentItem());
    SetVariableBlock* setParent=dynamic_cast<SetVariableBlock*>(target->parentItem());
    PushListBlock* pushParent=dynamic_cast<PushListBlock*>(target->parentItem());
    SetListBlock* setListParent=dynamic_cast<SetListBlock*>(target->parentItem());
    RemoveListItemBlock* removeItemParent=dynamic_cast<RemoveListItemBlock*>(target->parentItem());
    ControlCodeBlock* controlParent=dynamic_cast<ControlCodeBlock*>(target->parentItem());
    CustomCallBlock* customCallParent=dynamic_cast<CustomCallBlock*>(target->parentItem());
    if(customCallParent!=nullptr&&customCallParent->value==target){
        QPointF targetLocalPos=target->pos();
        customCallParent->value=moving;
        eraseFloatTreeRecords(moving);
        moving->setParentItem(customCallParent);
        moving->setPos(targetLocalPos);
        moving->dragging=false;
        moving->moved=false;
        resetFloatTreeZValue(moving);
        setInsertedOperatorInteractivity(moving);
        deleteFloatBlock(target);
        customCallParent->refreshSize();
        refreshAllControlLayouts();
        checkEditedFloatWorkspaceWidth(moving);
        return;
    }
    if(controlParent!=nullptr&&controlParent->condition==target){
        QPointF targetLocalPos=target->pos();
        controlParent->condition=moving;
        eraseFloatTreeRecords(moving);
        moving->setParentItem(controlParent);
        moving->setPos(targetLocalPos);
        moving->dragging=false;
        moving->moved=false;
        resetFloatTreeZValue(moving);
        setInsertedOperatorInteractivity(moving);
        deleteFloatBlock(target);
        controlParent->refreshSize();
        refreshAllControlLayouts();
        checkEditedFloatWorkspaceWidth(moving);
        return;
    }
    if(setParent!=nullptr&&setParent->value==target){
        QPointF targetLocalPos=target->pos();
        setParent->value=moving;
        eraseFloatTreeRecords(moving);
        moving->setParentItem(setParent);
        moving->setPos(targetLocalPos);
        moving->dragging=false;
        moving->moved=false;
        resetFloatTreeZValue(moving);
        setInsertedOperatorInteractivity(moving);
        deleteFloatBlock(target);
        setParent->refreshSize();
        refreshAllControlLayouts();
        checkEditedFloatWorkspaceWidth(moving);
        return;
    }
    if(codeParent!=nullptr&&codeParent->value==target){
        QPointF targetLocalPos=target->pos();
        codeParent->value=moving;
        eraseFloatTreeRecords(moving);
        moving->setParentItem(codeParent);
        moving->setPos(targetLocalPos);
        moving->dragging=false;
        moving->moved=false;
        resetFloatTreeZValue(moving);
        setInsertedOperatorInteractivity(moving);
        deleteFloatBlock(target);
        codeParent->refreshSize();
        refreshAllControlLayouts();
        checkEditedFloatWorkspaceWidth(moving);
        return;
    }
    if(outputParent!=nullptr&&outputParent->value==target){
        QPointF targetLocalPos=target->pos();
        outputParent->value=moving;
        eraseFloatTreeRecords(moving);
        moving->setParentItem(outputParent);
        moving->setPos(targetLocalPos);
        moving->dragging=false;
        moving->moved=false;
        resetFloatTreeZValue(moving);
        setInsertedOperatorInteractivity(moving);
        deleteFloatBlock(target);
        outputParent->refreshSize();
        refreshAllControlLayouts();
        checkEditedFloatWorkspaceWidth(moving);
        return;
    }
    if(pushParent!=nullptr&&pushParent->value==target){
        QPointF targetLocalPos=target->pos();
        pushParent->value=moving;
        eraseFloatTreeRecords(moving);
        moving->setParentItem(pushParent);
        moving->setPos(targetLocalPos);
        moving->dragging=false;
        moving->moved=false;
        resetFloatTreeZValue(moving);
        setInsertedOperatorInteractivity(moving);
        deleteFloatBlock(target);
        pushParent->refreshSize();
        refreshAllControlLayouts();
        checkEditedFloatWorkspaceWidth(moving);
        return;
    }
    if(setListParent!=nullptr&&(setListParent->index==target||setListParent->value==target)){
        QPointF targetLocalPos=target->pos();
        if(setListParent->index==target){
            setListParent->index=moving;
        }
        if(setListParent->value==target){
            setListParent->value=moving;
        }
        eraseFloatTreeRecords(moving);
        moving->setParentItem(setListParent);
        moving->setPos(targetLocalPos);
        moving->dragging=false;
        moving->moved=false;
        resetFloatTreeZValue(moving);
        setInsertedOperatorInteractivity(moving);
        deleteFloatBlock(target);
        setListParent->refreshSize();
        refreshAllControlLayouts();
        checkEditedFloatWorkspaceWidth(moving);
        return;
    }
    if(removeItemParent!=nullptr&&removeItemParent->index==target){
        QPointF targetLocalPos=target->pos();
        removeItemParent->index=moving;
        eraseFloatTreeRecords(moving);
        moving->setParentItem(removeItemParent);
        moving->setPos(targetLocalPos);
        moving->dragging=false;
        moving->moved=false;
        resetFloatTreeZValue(moving);
        setInsertedOperatorInteractivity(moving);
        deleteFloatBlock(target);
        removeItemParent->refreshSize();
        refreshAllControlLayouts();
        checkEditedFloatWorkspaceWidth(moving);
        return;
    }
    if(parentBlock==nullptr){
        QPointF targetPos=target->pos();
        deleteFloatBlock(target);
        moving->setPos(targetPos);
        resetFloatTreeZValue(moving);
        rememberFloatBlockStagePos(moving,scrollWorkspace);
        checkEditedFloatWorkspaceWidth(moving);
        return;
    }

    QPointF targetLocalPos=target->pos();
    UnaryOpBlock* unaryParent=dynamic_cast<UnaryOpBlock*>(parentBlock);
    if(unaryParent!=nullptr&&unaryParent->slot==target){
        unaryParent->slot=moving;
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
    ListGetBlock* getParent=dynamic_cast<ListGetBlock*>(parentBlock);
    if(getParent!=nullptr&&getParent->index==target){
        getParent->index=moving;
    }

    eraseFloatTreeRecords(moving);
    moving->setParentItem(parentBlock);
    moving->setPos(targetLocalPos);
    moving->dragging=false;
    moving->moved=false;
    resetFloatTreeZValue(moving);
    setInsertedOperatorInteractivity(moving);
    deleteFloatBlock(target);
    refreshFloatAncestors(parentBlock);
    checkEditedFloatWorkspaceWidth(moving);
}

void FloatBlock::mousePressEvent(QGraphicsSceneMouseEvent* event){
    if(event->button()==Qt::RightButton){
        if(contextMenuButtonPressed){
            event->accept();
            return;
        }
        showFloatContextMenu(this,event->scenePos());
        event->accept();
        return;
    }
    if(!contextMenuButtonPressed){
        clearContextMenu();
    }
    if(event->button()!=Qt::LeftButton){
        event->ignore();
        return;
    }
    if(!movable){
        event->ignore();
        return;
    }
    if(parentItem()!=nullptr){
        if(!isPlainNumberBlock(this)){
            qreal dragZ=nextDragZ();
            detachOperatorFromParent(this);
            setZValue(dragZ);
            setFloatTreeZValue(this,dragZ);
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
        qreal dragZ=nextDragZ();
        FloatBlock* tempBlock=copy();
        floatBlocks.push_back(tempBlock);
        scene()->addItem(tempBlock);
        tempBlock->setZValue(dragZ);
        setFloatTreeZValue(tempBlock,dragZ);
        tempBlock->dragging=true;
        tempBlock->scrollArea=scrollNone;
        tempBlock->moved=false;
        tempBlock->mouseOffset=event->pos();
        tempBlock->grabMouse();
        event->accept();
        return;
    }
    qreal dragZ=nextDragZ();
    setZValue(dragZ);
    setFloatTreeZValue(this,dragZ);
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
    if(event->button()!=Qt::LeftButton){
        event->accept();
        return;
    }
    if(parentItem()!=nullptr){
        if(!moved&&!isOperator()){
            editValue();
            checkEditedFloatWorkspaceWidth(this);
        }
        event->accept();
        return;
    }
    if(dragging){
        dragging=false;
        ungrabMouse();
        if(outsideWorkspaceHorizontal(sceneBoundingRect())){
            clearAbsorbShadow(this);
            deleteFloatBlock(this);
            saveUndoCheckpoint();
            return;
        }
        if(absorbTarget!=nullptr){
            FloatBlock* target=absorbTarget;
            attachOperatorToTarget(this,target);
            saveUndoCheckpoint();
            event->accept();
            return;
        }
        clearAbsorbShadow(this);
        if(outsideWorkspaceVertical(sceneBoundingRect())){
            setPos(pos()+clampVerticalDeltaToWorkspace(sceneBoundingRect()));
        }
        if(!inWorkspace(this)){
            deleteFloatBlock(this);
            saveUndoCheckpoint();
            return;
        }
        FloatBlock* zRoot=raiseFloatTreeAboveWorkspace(this);
        rememberFloatBlockStagePos(zRoot,scrollWorkspace);
        if(!moved&&!isbase&&!isOperator()){
            editValue();
            checkEditedFloatWorkspaceWidth(this);
        }
        saveUndoCheckpoint();
        event->accept();
        return;
    }
    QGraphicsPolygonItem::mouseReleaseEvent(event);
}

void CodeBlock::mousePressEvent(QGraphicsSceneMouseEvent* event){
    if(event->button()==Qt::RightButton){
        if(contextMenuButtonPressed){
            event->accept();
            return;
        }
        showCodeContextMenu(this,event->scenePos());
        event->accept();
        return;
    }
    if(!contextMenuButtonPressed){
        clearContextMenu();
    }
    if(isbase){
        qreal dragZ=nextDragZ();
        CodeBlock* tempBlock=copy();
        if(!isTopOnlyCodeBlock(tempBlock)){
            codeBlocks.push_back(tempBlock);
        }
        scene()->addItem(tempBlock);

        tempBlock->ismoving=true;
        setCodeChainMoving(tempBlock,true);
        CodeBlock* curr=tempBlock;
        if(curr->pre!=nullptr){
            curr->pre->next=nullptr;
            curr->pre=nullptr;
        }
        while(curr->next!=nullptr){
            curr=curr->next;
            curr->ismoving=true;
        }
        setCodeTreeZValue(tempBlock,dragZ);
        setCodeShadowLikeBlock(tempBlock->shadow,tempBlock);
        tempBlock->shadow->setPos(tempBlock->pos());
        tempBlock->shadow->setZValue(dragZ-1);
        scene()->addItem(tempBlock->shadow);
        tempBlock->dragging=true;
        tempBlock->scrollArea=scrollNone;
        tempBlock->mouseOffset=event->pos();
        tempBlock->grabMouse();
        event->accept();
        return;
    }
    qreal dragZ=nextDragZ();
    ismoving=true;
    ControlCodeBlock* oldInsideOwner=findInsideOwner(this);
    CodeBlock* curr=this;
    if(curr->pre!=nullptr){
        curr->pre->next=nullptr;
        curr->pre=nullptr;
    }
    if(oldInsideOwner!=nullptr&&oldInsideOwner->inside==this){
        oldInsideOwner->inside=nullptr;
    }
    insideParent=nullptr;
    refreshControlFromInside(oldInsideOwner);
    refreshAllControlLayouts();
    setCodeChainMoving(this,true);
    while(curr->next!=nullptr){
        curr=curr->next;
        curr->ismoving=true;
    }
    setCodeTreeZValue(this,dragZ);
    scrollArea=scrollNone;
    setCodeShadowLikeBlock(shadow,this);
    shadow->setPos(pos());
    shadow->setZValue(dragZ-1);
    scene()->addItem(shadow);
    dragging=true;
    mouseOffset=event->pos();
    event->accept();
}

void CodeBlock::mouseMoveEvent(QGraphicsSceneMouseEvent* event){
    int totalwid=wid;
    if(preTarget!=nullptr&&preTarget->next!=nullptr){
        syncCodeChainFrom(preTarget->next,preTarget->next->calculatePos());
    }
    CodeBlock* curr=this;
    preTarget=nullptr;
    nextTarget=nullptr;
    insideTarget=nullptr;
    while(curr->next!=nullptr){
        curr=curr->next;
        totalwid+=curr->wid;
    }
    if(dragging){
        refreshAllControlLayouts();
        setPos(event->scenePos()-mouseOffset);
        syncCodeChainFrom(this,pos());
        setCodeShadowLikeBlock(shadow,this);
        shadow->setPos(pos());
        if(!isTopOnlyCodeBlock(this)&&currentStartBlock!=nullptr&&!currentStartBlock->ismoving){
            QPointF startBottom=currentStartBlock->pos()+QPointF(0,currentStartBlock->wid);
            if((!codeChainEndsWithEndBlock(this)||currentStartBlock->next==nullptr)&&
               QLineF(pos(),startBottom).length()<20){
                preTarget=currentStartBlock;
                shadow->setPos(startBottom);
            }
        }
        qreal bestPreDistance=preTarget==nullptr?20:QLineF(pos(),preTarget->pos()+QPointF(0,preTarget->wid)).length();
        for(auto & otherBlock:codeBlocks){
            if(otherBlock==this){
                continue;
            }
            if(otherBlock->ismoving){
                otherBlock->setPos(otherBlock->calculatePos());
                continue;
            }
            if(isEndCodeBlock(otherBlock)){
                continue;
            }
            if(codeChainEndsWithEndBlock(this)&&otherBlock->next!=nullptr){
                continue;
            }
            QPointF otherPos=otherBlock->pos();
            qreal distance=QLineF(pos(),otherPos+QPointF(0,otherBlock->wid)).length();
            if(!isTopOnlyCodeBlock(this)&&nextTarget==nullptr&&distance<bestPreDistance){
                bestPreDistance=distance;
                preTarget=otherBlock;
                shadow->setPos(otherBlock->pos()+QPointF(0,otherBlock->wid));
            }
        }
        qreal bestNextDistance=20;
        if(!codeChainEndsWithEndBlock(this)){
        for(auto & otherBlock:codeBlocks){
            if(otherBlock==this){
                continue;
            }
            if(otherBlock->ismoving){
                continue;
            }
            QPointF otherPos=otherBlock->pos();
            if(isTopOnlyCodeBlock(otherBlock)){
                continue;
            }
            if(isTopOnlyCodeBlock(this)&&!canTopOnlyBlockAttachBefore(otherBlock)){
                continue;
            }
            if(isFirstInsideBlock(otherBlock)){
                continue;
            }
            qreal distance=QLineF(pos()+QPointF(0,totalwid),otherPos).length();
            if(preTarget==nullptr&&distance<bestNextDistance){
                if(otherBlock->pre!=nullptr&&findInsideOwner(otherBlock)==nullptr){
                    continue;
                }
                bestNextDistance=distance;
                nextTarget=otherBlock;
                setCodeShadowLikeBlock(shadow,curr);
                shadow->setPos(otherBlock->pos()-QPointF(0,curr->wid));
            }
        }
        }
        if(preTarget==nullptr&&!isTopOnlyCodeBlock(this)){
            insideTarget=findInsideAbsorbTarget(this);
            if(codeChainEndsWithEndBlock(this)&&insideTarget!=nullptr&&insideTarget->inside!=nullptr){
                insideTarget=nullptr;
            }
            if(insideTarget!=nullptr){
                nextTarget=nullptr;
                QPointF slotPos=insideTarget->pos()+QPointF(insideTarget->leftWidth,insideTarget->topHeight);
                previewControlInsideInsert(insideTarget,totalwid);
                shadow->setPos(slotPos);
                if(insideTarget->inside!=nullptr){
                    offsetCodeChain(insideTarget->inside,QPointF(0,totalwid));
                }
            }
        }
        ControlCodeBlock* previewInsideOwner=nullptr;
        if(preTarget!=nullptr){
            previewInsideOwner=findInsideOwner(preTarget);
        }
        else if(insideTarget==nullptr&&nextTarget!=nullptr){
            previewInsideOwner=nextTarget->insideParent;
        }
        if(previewInsideOwner!=nullptr){
            previewControlInsideInsert(previewInsideOwner,totalwid);
            if(nextTarget!=nullptr&&nextTarget->insideParent==previewInsideOwner){
                offsetCodeChain(nextTarget,QPointF(0,totalwid));
            }
        }
        if(preTarget!=nullptr){
            insideTarget=nullptr;
        }
        if(preTarget!=nullptr&&preTarget->next!=nullptr){
            offsetCodeChain(preTarget->next,QPointF(0,totalwid));
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
        QRectF releaseRect=codeTreeSceneRect(this);
        if(!isbase&&outsideWorkspaceHorizontal(releaseRect)){
            scene()->removeItem(shadow);
            deleteCodeChain(this);
            saveUndoCheckpoint();
            return;
        }
        bool hasAbsorbTarget=preTarget!=nullptr||nextTarget!=nullptr||insideTarget!=nullptr;
        if(!isbase&&!hasAbsorbTarget&&!inWorkspaceTotal(releaseRect)){
            scene()->removeItem(shadow);
            deleteCodeChain(this);
            saveUndoCheckpoint();
            return;
        }
        if(releasedStart!=nullptr&&currentStartBlock!=nullptr&&currentStartBlock!=releasedStart){
            message::multithreadingNotAllowed();
            releasedStart->ismoving=false;
            scene()->removeItem(shadow);
            deleteCodeBlock(this);
            return;
        }
        if(releasedStart!=nullptr){
            currentStartBlock=releasedStart;
            currentStartBlock->ismoving=false;
        }
        dragging=false;
        if(preTarget!=nullptr){
            insideTarget=nullptr;
            nextTarget=nullptr;
        }
        else if(insideTarget!=nullptr){
            nextTarget=nullptr;
        }
        if(insideTarget!=nullptr){
            CodeBlock* oldInside=insideTarget->inside;
            CodeBlock* tail=this;
            while(tail->next!=nullptr){
                tail=tail->next;
            }
            if(oldInside!=nullptr&&oldInside!=this){
                tail->next=oldInside;
                oldInside->pre=tail;
                oldInside->insideParent=nullptr;
            }
            insideTarget->inside=this;
            insideParent=insideTarget;
            pre=nullptr;
            setPos(insideTarget->pos()+QPointF(insideTarget->leftWidth,insideTarget->topHeight));
            refreshControlFromInside(insideTarget);
            if(insideTarget->next!=nullptr){
                syncCodeChainFrom(insideTarget->next,insideTarget->pos()+QPointF(0,insideTarget->wid));
            }
        }
        else if(preTarget!=nullptr){
            CodeBlock* b=preTarget;
            ControlCodeBlock* owner=findInsideOwner(b);
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
            insideParent=nullptr;
            refreshControlFromInside(owner);
        }
        else if(nextTarget!=nullptr){
            CodeBlock* b=nextTarget;
            ControlCodeBlock* owner=b->insideParent;
            CodeBlock* oldPrev=b->pre;
            curr=this;
            while(curr->next!=nullptr){
                curr->setPos(b->pos()-QPointF(0,totalwid));
                totalwid-=curr->wid;
                curr=curr->next;
            }
            curr->setPos(b->pos()-QPointF(0,totalwid));
            if(oldPrev!=nullptr){
                oldPrev->next=this;
                pre=oldPrev;
                insideParent=owner;
            }
            else if(owner!=nullptr){
                owner->inside=this;
                insideParent=owner;
                b->insideParent=nullptr;
                pre=nullptr;
            }
            else{
                pre=nullptr;
            }
            b->pre=curr;
            curr->next=b;
        }
        for(auto otherBlock:codeBlocks){
            otherBlock->ismoving=0;
        }
        if(currentStartBlock!=nullptr){
            currentStartBlock->ismoving=false;
        }
        refreshAllControlLayouts();
        CodeBlock* zRoot=raiseCodeTreeAboveWorkspace(this);
        rememberCodeTreeStagePos(zRoot,scrollWorkspace);
        scene()->removeItem(shadow);
        ungrabMouse();
        saveUndoCheckpoint();
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

level::TestContext currentLevelTestContext(){
    level::TestContext context;
    int mapWidth=currentMapWidth();
    int mapHeight=currentMapHeight();
    context.map.assign(mapWidth,std::vector<int>(mapHeight,0));
    for(int i=0;i<mapWidth;i++){
        for(int j=0;j<mapHeight;j++){
            context.map[i][j]=mapdata[i][j];
        }
    }
    if(player!=nullptr){
        context.robot.x=player->gridx;
        context.robot.y=player->gridy;
        context.robot.direction=player->direction;
    }
    context.runtime=&runtimeState;
    context.steps=levelTestStepCount;
    context.time=levelTestTimeCount;
    context.waited=runtimeWaitActionFrame;
    return context;
}

void updateTestStatusText(){
    if(testStatusText!=nullptr){
        testStatusText->setPlainText(QString::number(levelTestStepCount));
    }
    if(timeStatusText!=nullptr){
        timeStatusText->setPlainText(QString::number(levelTestTimeCount));
    }
}

void finishLevelTest(bool forcedFail,const QString& message){
    if(timerPtr!=nullptr){
        timerPtr->stop();
    }
    runtimeCountersActive=false;
    if(executorPtr!=nullptr){
        executorPtr->reset(nullptr);
    }
    runningBlock=nullptr;
    if(forcedFail||levelTestFailed){
        clearRuntimeCodeSnapshot();
        levelTestRunning=false;
        resetRunButtons();
        updateTestStatusText();
        QString text=message.isEmpty()?
            QString("测试样例 %1 失败: 机器人陷入循环或被陷阱困住")
                .arg(levelTestCaseIndex+1):
            message;
        QMessageBox::warning(nullptr,"测试结果",text);
        return;
    }
    level::TestResult result=level::testActiveLevelCase(levelTestCaseIndex,
        currentLevelTestContext());
    if(!result.passed){
        clearRuntimeCodeSnapshot();
        levelTestRunning=false;
        resetRunButtons();
        updateTestStatusText();
        QMessageBox::warning(nullptr,"测试结果",
            QString::fromStdString(result.message));
        return;
    }
    levelTestCaseIndex++;
    if(levelTestCaseIndex>=levelTestCaseTotal){
        clearRuntimeCodeSnapshot();
        levelTestRunning=false;
        resetRunButtons();
        updateTestStatusText();
        LevelChoosePage::upgradeLevelUnlocked(levelNumberNow+1);
        QString successText=result.message.empty()?
            QString("所有 %1 个测试样例都已通过").arg(levelTestCaseTotal):
            QString::fromStdString(result.message);
        QMessageBox::information(nullptr,"测试结果",successText);
        return;
    }
    beginLevelTestCase(levelTestCaseIndex);
}

void beginLevelTestCase(int index){
    if(!buildRuntimeCodeSnapshot()){
        if(!runtimeSnapshotError.isEmpty()){
            message::otherError(runtimeSnapshotError.toStdString());
            levelTestRunning=false;
            programRunning=false;
            resetRunButtons();
            updateTestStatusText();
            return;
        }
        finishLevelTest(true,"测试失败，缺少开始积木块");
        return;
    }
    if(runtimeStartBlock->next==nullptr){
        finishLevelTest(true,"测试失败，缺少开始积木块");
        return;
    }
    programRunning=true;
    resetActiveLevelForRun();
    resetRobot();
    runtimeState.resetAll();
    level::prepareActiveTestCase(index,runtimeState);
    ensureBuiltInRuntimeVariables();
    customParameterStacks.clear();
    runtimeStopRequested=false;
    levelTestFailed=false;
    levelTestStepCount=0;
    levelTestTimeCount=0;
    runtimeCountersActive=true;
    updateTestStatusText();
    runningBlock=runtimeStartBlock->next;
    if(executorPtr!=nullptr){
        executorPtr->reset(runtimeStartBlock->next);
    }
    if(timerPtr!=nullptr){
        timerPtr->stop();
        timerPtr->start(runtimeIntervalForCodeBlock(runtimeStartBlock->next));
    }
}

void startLevelTestRun(){
    if(activeRunButton!=nullptr||levelTestRunning||programRunning){
        stopProgram();
    }
    levelTestRunning=true;
    levelTestCaseIndex=0;
    levelTestCaseTotal=std::max(1,level::activeTestCaseCount());
    setRunTextButtonRunning();
    beginLevelTestCase(levelTestCaseIndex);
}

void ui::resetLevelConfig(){
    clearDataLockedNames();
    level::resetActiveLevel(screensize,screensize);
    level::setActiveRobotStart(5,5,0);
}

void ui::setLevelCell(int x,int y,int type){
    if(level::activeLevel().map().empty()){
        ui::resetLevelConfig();
    }
    if(x<0||x>=currentMapWidth()||y<0||y>=currentMapHeight()){
        return;
    }
    level::setActiveMapCell(x,y,type);
}

void ui::setRobotStart(int x,int y,int direction){
    if(level::activeLevel().map().empty()){
        ui::resetLevelConfig();
    }
    x=std::max(0,std::min(x,currentMapWidth()-1));
    y=std::max(0,std::min(y,currentMapHeight()-1));
    level::setActiveRobotStart(x,y,direction);
}

void ui::setReachPositionGoal(int x,int y){
    if(level::activeLevel().map().empty()){
        ui::resetLevelConfig();
    }
    level::setActiveReachPositionGoal(x,y);
}

void ui::setInputCases(const std::vector<level::DataTestCase>& cases){
    if(level::activeLevel().map().empty()){
        ui::resetLevelConfig();
    }
    level::setActiveInputCases(cases);
    lockDataCaseNames(cases);
    if(cases.empty()){
        return;
    }
    const level::DataTestCase& testCase=cases.front();
    for(const auto& item:testCase.inputVariables){
        runtimeState.forceSetVariable(item.first,item.second,true);
    }
    for(const auto& item:testCase.inputLists){
        runtimeState.forceSetList(item.first,item.second,true);
    }
    refreshVariableToolbox();
}

void ui::setDataOutputCases(const std::vector<level::DataTestCase>& cases){
    if(level::activeLevel().map().empty()){
        ui::resetLevelConfig();
    }
    level::setActiveDataOutputCases(cases);
    lockDataCaseNames(cases);
    if(cases.empty()){
        return;
    }
    for(const level::DataTestCase& testCase:cases){
        for(const auto& item:testCase.inputVariables){
            double value=runtimeState.hasVariable(item.first)?0.0:item.second;
            runtimeState.forceSetVariable(item.first,value,true);
        }
        for(const auto& item:testCase.inputLists){
            std::vector<double> value=runtimeState.hasList(item.first)?
                std::vector<double>():item.second;
            runtimeState.forceSetList(item.first,value,true);
        }
        for(const auto& item:testCase.expectedVariables){
            if(!runtimeState.hasVariable(item.first)){
                runtimeState.forceSetVariable(item.first,0.0,false);

            }
        }
        for(const auto& item:testCase.expectedLists){
            if(!runtimeState.hasList(item.first)){
                runtimeState.forceSetList(item.first,std::vector<double>(),false);
            }
        }
    }
    refreshVariableToolbox();
}

void init(){
    if(level::activeLevel().map().empty()){
        ui::resetLevelConfig();
    }
    syncMapDataFromActiveLevel();
}

void syncMapDataFromActiveLevel(){
    const level::LevelConfig& config=level::activeLevel();
    for(int i=0;i<screensize;i++){
        for(int j=0;j<screensize;j++){
            mapdata[i][j]=level::CellEmpty;
        }
    }
    int mapWidth=currentMapWidth();
    int mapHeight=currentMapHeight();
    int configWidth=static_cast<int>(config.map().size());
    for(int i=0;i<screensize;i++){
        for(int j=0;j<screensize;j++){
            if(i>=mapWidth||j>=mapHeight||i>=configWidth){
                continue;
            }
            mapdata[i][j]=config.mapCell(i,j);
        }
    }
}

void resetActiveLevelForRun(){
    level::configureActiveLevel(level::activeLevelNumber(),level::activeLevelType());
    syncMapDataFromActiveLevel();
    updateStageGeometry();
}

void resetRobot(){
    level::RobotState start=level::activeLevel().robotStart();
    player->gridx=std::max(0,std::min(start.x,currentMapWidth()-1));
    player->gridy=std::max(0,std::min(start.y,currentMapHeight()-1));
    player->direction=((start.direction%4)+4)%4;
    player->SyncCell(stageDisplayCellSize(),stageDisplayOrigin());
}

void resetRunButtons(){
    programRunning=false;
    if(runTextButton!=nullptr){
        runTextButton->text->setPlainText("运行");
        runTextButton->setTexture("run.png");
    }
    if(runButton!=nullptr){
        runButton->setPlayShape();
    }
    if(fastRunButton!=nullptr){
        fastRunButton->setLightningShape();
    }
    activeRunButton=nullptr;
}

bool isRuntimeActionBlockType(int blockType){
    return blockType==0||blockType==1||blockType==3||blockType==4;
}

bool isFirstLevelRestrictedToolbox(){
    return level::activeLevelNumber()==1;
}

bool toolboxAllowsWaitBlock(){
    return level::activeLevelNumber()>=2;
}

bool toolboxAllowsAdvancedBlocks(){
    return level::activeLevelNumber()>=3;
}

bool toolboxAllowsCustomBlocks(){
    return level::activeLevelNumber()>=4;
}

int runtimeIntervalForBlockType(int blockType){
    return isRuntimeActionBlockType(blockType)
        ? settings::RuntimeActionBlockIntervalMs
        : settings::RuntimeCodeBlockIntervalMs;
}

int runtimeIntervalForCodeBlock(CodeBlock* block){
    if(block==nullptr){
        return settings::RuntimeCodeBlockIntervalMs;
    }
    return runtimeIntervalForBlockType(block->type);
}

void setRunTextButtonRunning(){
    if(runTextButton==nullptr){
        return;
    }
    runTextButton->text->setPlainText(QString::fromUtf8("\350\277\220\350\241\214\344\270\255"));
    runTextButton->refreshShape();
    runTextButton->setFixedSize(150,scaledAssetHeight("run.png",150,40));
}

void stopProgram(){
    programRunning=false;
    runtimeStopRequested=true;
    levelTestRunning=false;
    levelTestFailed=false;
    levelTestStepCount=0;
    levelTestTimeCount=0;
    runtimeCountersActive=false;
    levelTestCaseIndex=0;
    levelTestCaseTotal=1;
    updateTestStatusText();
    runningBlock=nullptr;
    if(timerPtr!=nullptr){
        timerPtr->stop();
    }
    if(executorPtr!=nullptr){
        executorPtr->reset(nullptr);
    }
    customParameterStacks.clear();
    clearRuntimeCodeSnapshot();
    resetRunButtons();
}

void startProgram(Button* button,int intervalMs){
    if(programRunning||activeRunButton!=nullptr){
        stopProgram();
    }
    if(!buildRuntimeCodeSnapshot()){
        if(!runtimeSnapshotError.isEmpty()){
            message::otherError(runtimeSnapshotError.toStdString());
            resetRunButtons();
            return;
        }
        clearRuntimeCodeSnapshot();
        resetRunButtons();
        return;
    }
    if(runtimeStartBlock->next==nullptr){
        clearRuntimeCodeSnapshot();
        resetRunButtons();
        return;
    }
    resetActiveLevelForRun();
    resetRobot();
    runtimeState.resetAll();
    level::prepareActiveTestCase(0,runtimeState);
    ensureBuiltInRuntimeVariables();
    customParameterStacks.clear();
    runtimeStopRequested=false;
    levelTestFailed=false;
    levelTestStepCount=0;
    levelTestTimeCount=0;
    runtimeCountersActive=true;
    updateTestStatusText();
    programRunning=true;
    if(runTextButton!=nullptr){
        runTextButton->text->setPlainText("运行中");
        runTextButton->refreshShape();
        runTextButton->setFixedSize(150,scaledAssetHeight("run.png",150,40));
    }
    runningBlock=runtimeStartBlock->next;
    if(executorPtr!=nullptr){
        executorPtr->reset(runtimeStartBlock->next);
    }
    if(button!=nullptr){
        activeRunButton=button;
        button->setStopShape();
    }
    if(timerPtr!=nullptr){
        timerPtr->stop();
        timerPtr->start(runtimeIntervalForCodeBlock(runtimeStartBlock->next));
    }
}

void toggleProgram(Button* button,int intervalMs){
    if(activeRunButton==button){
        stopProgram();
        return;
    }
    startProgram(button,intervalMs);
}

void drawStage(QGraphicsScene& scene){
    addHeaderLogo(scene);

    QPoint stageOrigin=stageDisplayOrigin();
    int stageSize=stageDisplaySize();
    int mapWidth=currentMapWidth();
    int mapHeight=currentMapHeight();
    stage=scene.addRect(stageOrigin.x(),stageOrigin.y(),stageSize,stageSize);
    stage->setBrush(Qt::white);
    stage->setPen(Qt::NoPen);

    int cellSize=stageDisplayCellSize();
    std::map<int,QBrush> cellBrushes=buildCellBrushes(cellSize);
    for(int i=0;i<screensize;i++){
        for(int j=0;j<screensize;j++){
            squares[i][j]=scene.addRect(0,0,cellSize,cellSize);
            platePairImages[i][j]=nullptr;
            if(i>=mapWidth||j>=mapHeight){
                squares[i][j]->setVisible(false);
            }
            else{
                squares[i][j]->setBrush(brushForCell(mapdata[i][j],cellBrushes));
            }
            squares[i][j]->setPos(stageOrigin.x()+i*cellSize,stageOrigin.y()+j*cellSize);
        }
    }
    resetRobot();
    player->setZValue(10);
    scene.addItem(player);
    updateStageGeometry();
    runButton=nullptr;
    fastRunButton=nullptr;

    runTextButton=new TextButton("运行");
    runTextButton->setPos(15,410);
    runTextButton->setFixedSize(150,scaledAssetHeight("run.png",150,40));
    runTextButton->setBrush(QColor(44,135,82));
    runTextButton->setTexture("run.png");
    runTextButton->setZValue(20);
    runTextButton->onClick=[](){
        startLevelTestRun();
    };
    scene.addItem(runTextButton);

    TextButton* stopTextButton=new TextButton("停止");
    stopTextButton->setPos(175,410);
    stopTextButton->setFixedSize(150,scaledAssetHeight("stop.png",150,40));
    stopTextButton->setBrush(QColor(180,48,48));
    stopTextButton->setTexture("stop.png");
    stopTextButton->setZValue(20);
    stopTextButton->onClick=[](){
        stopProgram();
    };
    scene.addItem(stopTextButton);

    fullscreenButton=new TextButton("全");
    fullscreenButton->setPos(260,577);
    fullscreenButton->setFixedSize(60,60);
    fullscreenButton->setBrush(fileButtonColor());
    fullscreenButton->setTexture("larger.png");
    fullscreenButton->text->hide();
    fullscreenButton->setZValue(20);
    fullscreenButton->onClick=[](){
        setStageExpanded(true);
        if(!programRunning&&!levelTestRunning){
            startLevelTestRun();
        }
    };
    scene.addItem(fullscreenButton);

    shrinkStageButton=new TextButton("缩");
    shrinkStageButton->setFixedSize(60,60);
    shrinkStageButton->setBrush(fileButtonColor());
    shrinkStageButton->setTexture("smaller.png");
    shrinkStageButton->text->hide();
    shrinkStageButton->setVisible(false);
    shrinkStageButton->onClick=[](){
        setStageExpanded(false);
    };
    scene.addItem(shrinkStageButton);

    QPixmap informationPixmap=loadImageAsset("information.png");
    if(!informationPixmap.isNull()){
        constexpr int infoWidth=320;
        int infoHeight=int(std::round(double(informationPixmap.height())*infoWidth/informationPixmap.width()));
        informationImage=scene.addPixmap(
            informationPixmap.scaled(infoWidth,infoHeight,Qt::KeepAspectRatio,Qt::SmoothTransformation)
        );
        informationImage->setPos(10,460);
        informationImage->setZValue(20);
    }
    else{
        informationFallbackBox=scene.addRect(10,460,320,107);
        informationFallbackBox->setBrush(QColor(230,233,238));
        informationFallbackBox->setPen(QPen(QColor(90,100,112),1.5));
        informationFallbackBox->setZValue(20);
    }

    testButton=nullptr;
    testStatusText=new QGraphicsTextItem();
    testStatusText->document()->setDocumentMargin(0);
    testStatusText->setDefaultTextColor(QColor(255,255,255));
    testStatusText->setFont(QFont("Arial",20,QFont::Bold));
    testStatusText->setTextWidth(70);
    testStatusText->setPos(82,494);
    testStatusText->setZValue(21);
    testStatusText->setAcceptedMouseButtons(Qt::NoButton);
    scene.addItem(testStatusText);

    timeStatusText=new QGraphicsTextItem();
    timeStatusText->document()->setDocumentMargin(0);
    timeStatusText->setDefaultTextColor(QColor(255,255,255));
    timeStatusText->setFont(QFont("Arial",20,QFont::Bold));
    timeStatusText->setTextWidth(70);
    timeStatusText->setPos(235,494);
    timeStatusText->setZValue(21);
    timeStatusText->setAcceptedMouseButtons(Qt::NoButton);
    scene.addItem(timeStatusText);
    updateInformationGeometry();
    updateTestStatusText();

    if(toolboxAllowsAdvancedBlocks()){
        createVariableButton=new TextButton("创建新变量");
        createVariableButton->setPos(10,577);
        createVariableButton->setFixedSize(160,scaledAssetHeight("create_variable.png",160,40));
        createVariableButton->setBrush(variableColor());
        createVariableButton->setTexture("create_variable.png");
        createVariableButton->setZValue(20);
        createVariableButton->onClick=[](){
            bool ok=false;
            QString name=QInputDialog::getText(nullptr,"创建新变量","请输入变量名",
                QLineEdit::Normal,"",&ok).trimmed();
            if(!ok){
                return;
            }
            if(!validVariableName(name)){
                message::invalidVariableName();
                return;
            }
            if(!runtimeState.hasVariable(name.toStdString())){
                runtimeState.createVariable(name.toStdString());
            }
            refreshVariableToolbox();
        };
        scene.addItem(createVariableButton);

        createListButton=new TextButton("创建新列表");
        createListButton->setPos(10,617);
        createListButton->setFixedSize(160,scaledAssetHeight("create_list.png",160,40));
        createListButton->setBrush(listColor());
        createListButton->setTexture("create_list.png");
        createListButton->setZValue(20);
        createListButton->onClick=[](){
            bool ok=false;
            QString name=QInputDialog::getText(nullptr,"创建新列表","请输入列表名",
                QLineEdit::Normal,"",&ok).trimmed();
            if(!ok){
                return;
            }
            if(!validVariableName(name)){
                message::invalidVariableName();
                return;
            }
            if(!runtimeState.hasList(name.toStdString())){
                runtimeState.createList(name.toStdString());
            }
            refreshVariableToolbox();
        };
        scene.addItem(createListButton);
    }

    if(toolboxAllowsCustomBlocks()){
        createCustomBlockButton=new TextButton("创建自定义积木");
        createCustomBlockButton->setPos(10,657);
        createCustomBlockButton->setFixedSize(160,scaledAssetHeight("create_custom.png",160,40));
        createCustomBlockButton->setBrush(customBlockColor());
        createCustomBlockButton->setTexture("create_custom.png");
        createCustomBlockButton->setZValue(20);
        createCustomBlockButton->onClick=[](){
            QDialog dialog;
            dialog.setWindowTitle("创建自定义积木");
            QFormLayout layout(&dialog);
            QLineEdit nameEdit;
            QLineEdit parameterEdit;
            layout.addRow("自定义积木名",&nameEdit);
            layout.addRow("参数名(默认无参数)",&parameterEdit);
            QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
            layout.addWidget(&buttons);
            QObject::connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
            QObject::connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
            if(dialog.exec()!=QDialog::Accepted){
                return;
            }
            QString name=nameEdit.text().trimmed();
            QString parameterName=parameterEdit.text().trimmed();
            if(!validVariableName(name)){
                message::invalidVariableName();
                return;
            }
            if(!parameterName.isEmpty()&&!validVariableName(parameterName)){
                message::invalidVariableName();
                return;
            }
            if(customHatBlocks.find(name)!=customHatBlocks.end()){
                return;
            }
            CustomHatBlock* hat=new CustomHatBlock(name,parameterName,false);
            setCodeBlockStagePos(hat,scrollWorkspace,QPointF(40,40));
            hat->setZValue(10);
            if(appScene!=nullptr){
                appScene->addItem(hat);
            }
            codeBlocks.push_back(hat);
            customHatBlocks[name]=hat;

            CustomCallBlock* callBlock=new CustomCallBlock(name,parameterName,nullptr,true);
            callBlock->setZValue(10);
            if(appScene!=nullptr){
                appScene->addItem(callBlock);
            }
            baseCodeBlocks.push_back(callBlock);
            customCallBaseBlocks.push_back(callBlock);
            refreshVariableToolbox();
            saveUndoCheckpoint();
        };
        scene.addItem(createCustomBlockButton);
    }

    TextButton* settingsButton=new TextButton("Settings");
    settingsButton->setPos(990,10);
    settingsButton->setFixedSize(60,60);
    settingsButton->setBrush(fileButtonColor());
    settingsButton->setTexture("icons/settings.png");
    settingsButton->text->hide();
    settingsButton->setZValue(topUiZ);
    settingsButton->onClick=[](){
        settings::SettingsDialog dialog;
        if(dialog.exec()==QDialog::Accepted){
            workspaceScrollY=std::min(workspaceScrollY,workspaceMaxScrollY());
            syncScrollArea(scrollWorkspace);
        }
    };
    scene.addItem(settingsButton);

    TextButton* saveButton=new TextButton("保存");
    saveButton->setPos(1060,10);
    saveButton->setFixedSize(60,60);
    saveButton->setBrush(fileButtonColor());
    saveButton->setTexture("save.png");
    saveButton->text->hide();
    saveButton->setZValue(topUiZ);
    saveButton->onClick=[](){
        QString filePath=QFileDialog::getSaveFileName(
            nullptr,
            "保存文件",
            archiveDefaultFilePath(),
            "JSON 文件 (*.json)"
        );
        if (filePath.isEmpty()) {
            return;
        }
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            message::otherError("错误，无法创建或写入该文件！");
            return;
        }
        file.write(QJsonDocument(serializeWorkspace()).toJson());
        file.close();
    };
    scene.addItem(saveButton);

    TextButton* openButton=new TextButton("打开");
    openButton->setPos(1130,10);
    openButton->setFixedSize(60,60);
    openButton->setBrush(fileButtonColor());
    openButton->setTexture("open.png");
    openButton->text->hide();
    openButton->setZValue(topUiZ);
    openButton->onClick=[](){
        QString filePath=QFileDialog::getOpenFileName(
            nullptr,
            "打开文件",
            archiveDirectoryPath(),
            "JSON 文件 (*.json)"
        );
        if (filePath.isEmpty()) {
            return;
        }
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            message::otherError("错误，打开文件失败！");
            return;
        }

        QByteArray jsonData = file.readAll();
        file.close();
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
        if (parseError.error != QJsonParseError::NoError||!jsonDoc.isObject()) {
            message::otherError("当前存档格式错误");
            return;
        }
        QJsonObject checkedRoot=jsonDoc.object();
        QJsonValue levelValue=checkedRoot["levelNumber"];
        if(!levelValue.isDouble()){
            message::otherError("当前存档格式错误");
            return;
        }
        int checkedFileLevelNumber=levelValue.toInt(-1);
        if(checkedFileLevelNumber<level::MinLevelNumber||
           checkedFileLevelNumber>level::TotalLevelCount){
            message::otherError("当前存档格式错误");
            return;
        }
        int checkedCurrentLevelNumber=level::activeLevelNumber();
        if(checkedFileLevelNumber!=checkedCurrentLevelNumber){
            QMessageBox::warning(nullptr,"关卡不一致",
                QString("当前是第 %1 关，打开的文件是第 %2 关")
                    .arg(checkedCurrentLevelNumber)
                    .arg(checkedFileLevelNumber));
            return;
        }
        clearUndoCache();
        restoreWorkspace(checkedRoot);
        saveUndoCheckpoint();
        return;
        if (parseError.error != QJsonParseError::NoError) {
            message::otherError(std::string("JSON 解析失败:")+parseError.errorString().toStdString());
            return;
        }

        if (jsonDoc.isObject()) {
            QJsonObject root=jsonDoc.object();
            if(root.contains("levelNumber")){
                int fileLevelNumber=root["levelNumber"].toInt(level::activeLevelNumber());
                int currentLevelNumber=level::activeLevelNumber();
                if(fileLevelNumber!=currentLevelNumber){
                    QMessageBox::warning(nullptr,"关卡不一致",
                        QString("当前是第 %1 关，打开的文件是第 %2 关")
                            .arg(currentLevelNumber)
                            .arg(fileLevelNumber));
                }
            }
            clearUndoCache();
            restoreWorkspace(root);
            saveUndoCheckpoint();
        }
        else if (jsonDoc.isArray()) {
            message::otherError("错误：该 JSON 文件的最外层是一个数组(Array)，而不是对象(Object)！");
        }
    };
    scene.addItem(openButton);
    exitButton=new TextButton("退出");
    exitButton->setPos(10,10);
    exitButton->setFixedSize(60,60);
    exitButton->setBrush(fileButtonColor());
    exitButton->setTexture("icons/return.png");
    exitButton->text->hide();
    exitButton->setZValue(topUiZ);
    scene.addItem(exitButton);

    levelInfoPanel=new LevelHintPanel();
    levelInfoPanel->setPos((appWidth-720)/2,(appHeight-420)/2);
    levelInfoPanel->setHintText(hintTextForLevel(level::activeLevelNumber()));
    levelInfoPanel->setZValue(topUiZ+30);
    levelInfoPanel->setVisible(false);
    scene.addItem(levelInfoPanel);

    levelInfoButton=new TextButton("信息");
    levelInfoButton->setPos(80,10);
    levelInfoButton->setFixedSize(60,60);
    levelInfoButton->setBrush(fileButtonColor());
    levelInfoButton->setTexture("icons/information.png");
    levelInfoButton->text->hide();
    levelInfoButton->setZValue(topUiZ);
    levelInfoButton->onClick=[](){
        if(levelInfoPanel!=nullptr){
            levelInfoPanel->setVisible(!levelInfoPanel->isVisible());
        }
    };
    scene.addItem(levelInfoButton);
}

void addPanelMasks(QGraphicsScene& scene,QRectF panelRect,bool protectStage=false){
    QColor maskColor=appBackgroundColor();
    QVector<QRectF> masks;
    qreal leftMaskX=panelRect.left()-20;
    qreal leftMaskWidth=20;
    if(protectStage&&leftMaskX<341){
        leftMaskWidth=panelRect.left()-(stageX+stagePixelSize+1);
        leftMaskX=stageX+stagePixelSize+1;
    }
    masks<<QRectF(panelRect.left(),0,panelRect.width(),panelRect.top())
         <<QRectF(panelRect.left(),panelRect.bottom(),panelRect.width(),appHeight-panelRect.bottom())
         <<QRectF(leftMaskX,panelRect.top(),leftMaskWidth,panelRect.height())
         <<QRectF(panelRect.right(),panelRect.top(),20,panelRect.height());
    for(const QRectF& rect:masks){
        QGraphicsRectItem* mask=scene.addRect(rect);
        mask->setBrush(maskColor);
        mask->setPen(Qt::NoPen);
        mask->setZValue(panelMaskZ);
    }
}

void drawToolbox(QGraphicsScene& scene){
    QRectF panelRect(toolboxX,toolboxY,toolboxWidth,toolboxHeight);
    toolboxBackground=scene.addRect(panelRect);
    toolboxBackground->setBrush(panelBackgroundColor());
    toolboxBackground->setPen(Qt::NoPen);
    int y=20;
    auto addCode=[&](CodeBlock* block){
        setCodeBlockStagePos(block,scrollToolbox,QPointF(20,y));
        block->setZValue(10);
        scene.addItem(block);
        baseCodeBlocks.push_back(block);
        y+=block->wid+toolboxCodeBlockGap;
    };
    auto addFloat=[&](FloatBlock* block){
        setFloatBlockStagePos(block,scrollToolbox,QPointF(20,y));
        block->setZValue(10);
        scene.addItem(block);
        floatBlocks.push_back(block);
        y+=block->wid+toolboxFloatBlockGap;
    };

    addCode(new StartBlock(true));
    addCode(new EndBlock(true));
    addCode(new CodeBlock(0,"左转",true));
    addCode(new CodeBlock(1,"右转",true));
    addCode(new FloatCodeBlock(3,"向前移动",nullptr,true));
    if(toolboxAllowsWaitBlock()){
        addCode(new FloatCodeBlock(4,"等待",nullptr,true));
    }
    if(toolboxAllowsAdvancedBlocks()){
        addCode(new OutputBlock("x",nullptr,true));
        addCode(new ControlCodeBlock(5,"如果",nullptr,true));
        addCode(new ControlCodeBlock(6,"当",nullptr,true));

        const std::pair<int,QString> unaryBlocks[]={
            {8,"floor"},
            {9,"abs"},
            {10,"not"}
        };
        for(const auto& item:unaryBlocks){
            addFloat(new UnaryOpBlock(item.first,item.second,nullptr,true));
        }

        const std::pair<int,QString> binaryBlocks[]={
            {0,"+"},
            {1,"-"},
            {2,"*"},
            {3,"/"},
            {4,"pow"},
            {6,"max"},
            {7,"min"},
            {8,"=="},
            {9,"!="},
            {10,"<"},
            {11,">"},
            {12,"and"},
            {13,"or"}
        };
        for(const auto& item:binaryBlocks){
            addFloat(new BinaryOpBlock(item.first,item.second,nullptr,nullptr,true));
        }
        addFloat(new RobotCoordBlock(0,true));
        addFloat(new RobotCoordBlock(1,true));
        addFloat(new RobotFrontMapBlock(true));

        variableToolboxStartY=y;
        variableSetBaseBlock=new SetVariableBlock("x",nullptr,true);
        variableSetBaseBlock->setZValue(10);
        scene.addItem(variableSetBaseBlock);
        baseCodeBlocks.push_back(variableSetBaseBlock);
        variableIncreaseBaseBlock=new IncreaseVariableBlock("x",nullptr,true);
        variableIncreaseBaseBlock->setZValue(10);
        scene.addItem(variableIncreaseBaseBlock);
        baseCodeBlocks.push_back(variableIncreaseBaseBlock);

        auto addListFloat=[&](FloatBlock* block){
            block->setZValue(10);
            scene.addItem(block);
            floatBlocks.push_back(block);
            listFloatBaseBlocks.push_back(block);
        };
        auto addListCode=[&](CodeBlock* block){
            block->setZValue(10);
            scene.addItem(block);
            baseCodeBlocks.push_back(block);
            listCodeBaseBlocks.push_back(block);
        };
        addListFloat(new ListGetBlock("x",nullptr,true));
        addListFloat(new ListSizeBlock("x",true));
        addListCode(new PushListBlock("x",nullptr,true));
        addListCode(new SetListBlock("x",nullptr,nullptr,true));
        addListCode(new RemoveListItemBlock("x",nullptr,true));
        addListCode(new ClearListBlock("x",true));

        refreshVariableToolbox();
    }
    if(!toolboxAllowsAdvancedBlocks()){
        variableToolboxStartY=y;
    }
    updateToolboxScrollRange();
    toolboxSlider=new ScrollSlider(panelRect.right()-22,panelRect.top()+10,panelRect.height()-30);
    QPixmap sliderPixmap=loadImageAsset("slider.png");
    if(!sliderPixmap.isNull()){
        toolboxSlider->setBrush(QBrush(sliderPixmap.scaled(
            int(scrollSliderWidth),
            int(scrollSliderHeight),
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation
        )));
        toolboxSlider->setPen(Qt::NoPen);
    }
    toolboxSlider->onChanged=[](qreal value){
        toolboxScrollY=value*toolboxMaxScrollY;
        syncScrollArea(scrollToolbox);
    };
    scene.addItem(toolboxSlider);
    addPanelMasks(scene,panelRect,true);
}

void drawWorkspace(QGraphicsScene& scene){
    QRectF panelRect=workspaceRect();
    workspaceBackground=scene.addRect(panelRect);
    workspaceBackground->setBrush(panelBackgroundColor());
    workspaceBackground->setPen(Qt::NoPen);
    workspaceSlider=new ScrollSlider(panelRect.right()-22,panelRect.top()+10,panelRect.height()-30);
    QPixmap sliderPixmap=loadImageAsset("slider.png");
    if(!sliderPixmap.isNull()){
        workspaceSlider->setBrush(QBrush(sliderPixmap.scaled(
            int(scrollSliderWidth),
            int(scrollSliderHeight),
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation
        )));
        workspaceSlider->setPen(Qt::NoPen);
    }
    workspaceSlider->onChanged=[](qreal value){
        workspaceScrollY=value*workspaceMaxScrollY();
        syncScrollArea(scrollWorkspace);
    };
    scene.addItem(workspaceSlider);
    addPanelMasks(scene,panelRect);
}
void LevelChoosePage::onStartButtonClicked()
{
    QObject* Slender=sender();
    int levelNumber=-1;
    if(Slender)
    {
        levelNumber=Slender->property("levelNumber").toInt();
    }
    levelNumberNow=levelNumber;
    startLevel(levelNumber);
}

void LevelChoosePage::startLevel(int levelNumber)
{
    if(levelNumber<level::MinLevelNumber||levelNumber>level::TotalLevelCount){
        return;
    }
    editorExitToDesktopRequested=false;
    level::LevelType levelType=level::defaultLevelTypeForNumber(levelNumber);
    level::configureActiveLevel(levelNumber,levelType);
    runtimeState.clearAll();
    level::prepareActiveTestCase(0,runtimeState);
    ensureBuiltInRuntimeVariables();
    lockCurrentDataRuntimeNames();
    ::init();
    stageExpanded=false;
    if(view!=nullptr){
        view->onClosed=nullptr;
        QGraphicsScene* oldScene=scene;
        view->close();
        delete view;
        view=nullptr;
        if(oldScene!=nullptr){
            delete oldScene;
        }
        scene=nullptr;
    }
    scene = new QGraphicsScene(this);
    view = new AppGraphicsView(scene);
    view->onClosed=[this]()
    {
        AppGraphicsView* closedView=view;
        QGraphicsScene* closedScene=scene;
        view=nullptr;
        scene=nullptr;
        if(closedView!=nullptr){
            closedView->deleteLater();
        }
        if(closedScene!=nullptr){
            closedScene->deleteLater();
        }
    };
    close();
    view->show();
}
void MainWindow::onStartButtonClicked()
{
    bool ok=false;
    int levelNumber=QInputDialog::getInt(
        this,
        QString::fromUtf8("选择关卡"),
        QString::fromUtf8("请输入关卡编号："),
        level::MinLevelNumber,
        level::MinLevelNumber,
        level::TotalLevelCount,
        1,
        &ok
    );
    if(!ok){
        return;
    }
    editorExitToDesktopRequested=false;
    level::LevelType levelType=level::defaultLevelTypeForNumber(levelNumber);
    level::configureActiveLevel(levelNumber,levelType);
    runtimeState.clearAll();
    level::prepareActiveTestCase(0,runtimeState);
    ensureBuiltInRuntimeVariables();
    init();
    stageExpanded=false;
    if(view!=nullptr){
        view->onClosed=nullptr;
        QGraphicsScene* oldScene=scene;
        view->close();
        view->deleteLater();
        view=nullptr;
        if(oldScene!=nullptr){
            oldScene->deleteLater();
        }
        scene=nullptr;
    }
    scene = new QGraphicsScene(this);
    view = new AppGraphicsView(scene);
    view->onClosed=[this]()
    {
        AppGraphicsView* closedView=view;
        QGraphicsScene* closedScene=scene;
        view=nullptr;
        scene=nullptr;
        if(closedView!=nullptr){
            closedView->deleteLater();
        }
        if(closedScene!=nullptr){
            closedScene->deleteLater();
        }
        this->show();
    };
    this->hide();
    view->show();
}

int ui::runApp(int argc,char* argv[]){
    init();
    QApplication app(argc,argv);
    player=new Robot();
    MainWindow* mainWindow=new MainWindow();
    mainWindow->show();
    //QGraphicsScene scene;
    //AppGraphicsView* view=new AppGraphicsView(&scene);
   // view->show();

    return app.exec();
}
