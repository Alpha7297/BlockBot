#include "levelchoosepage.h"

#include "UiConstants.h"
#include "../level/LevelConstants.h"
#include "../message/Message.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QFrame>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QPalette>
#include <QPixmap>
#include <QPoint>
#include <QStandardPaths>
#include <QTransform>
#include <QBrush>
#include <QFont>
#include <QPen>
#include <algorithm>
#include <functional>

namespace{

constexpr int levelButtonWidth=150;
constexpr int levelButtonHeight=100;
constexpr int levelHorizontalGap=75;
constexpr int levelVerticalGap=50;
constexpr int levelTopMargin=100;

QString assetPath(const QString& relativePath){
    QStringList roots;
    roots<<QDir::currentPath()
         <<QCoreApplication::applicationDirPath()
         <<QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("..")
         <<QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../..");
    for(const QString& root:roots){
        QString path=QDir(root).filePath(relativePath);
        if(QFileInfo::exists(path)){
            return QDir::fromNativeSeparators(path);
        }
    }
    return relativePath;
}

QString levelSaveFilePath(){
    QString dir=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if(dir.isEmpty()){
        dir=QCoreApplication::applicationDirPath();
    }
    QDir().mkpath(dir);
    return QDir(dir).filePath("level.json");
}

QRect levelFrameRectFor(const QPoint& levelPos){
    return QRect(
        levelPos.x()-17,
        levelPos.y()-10,
        183,
        122
    );
}

QColor levelBackgroundColor(int levelNum,int currentLevel){
    if(levelNum<currentLevel){
        return QColor(62,158,92,220);
    }
    if(levelNum==currentLevel){
        return QColor(194,66,62,225);
    }
    return QColor(92,96,102,210);
}

QColor levelTextColor(int levelNum,int currentLevel){
    return levelNum>currentLevel?QColor(137,146,154):QColor(247,251,255);
}

void addFixedImage(QGraphicsScene* scene,const QString& relativePath,
                   const QRect& rect,bool flipHorizontal=false){
    QPixmap pixmap(assetPath(relativePath));
    if(flipHorizontal&&!pixmap.isNull()){
        pixmap=pixmap.transformed(QTransform().scale(-1,1));
    }
    if(!pixmap.isNull()){
        pixmap=pixmap.scaled(rect.size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    }
    QGraphicsPixmapItem* item=scene->addPixmap(pixmap);
    item->setPos(rect.x(),rect.y());
}

bool writeLevelSave(int currentLevel){
    QFile file(levelSaveFilePath());
    if(!file.open(QIODevice::WriteOnly|QIODevice::Text)){
        return false;
    }
    QJsonObject object;
    object["level"]=currentLevel;
    file.write(QJsonDocument(object).toJson(QJsonDocument::Compact));
    return true;
}

class ClickablePixmapItem final:public QGraphicsPixmapItem{
public:
    std::function<void()> onClick;

    explicit ClickablePixmapItem(const QPixmap& pixmap,
                                 std::function<void()> clickHandler,
                                 QGraphicsItem* parent=nullptr)
        :QGraphicsPixmapItem(pixmap,parent),onClick(std::move(clickHandler)){
        setAcceptedMouseButtons(Qt::LeftButton);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override{
        if(event->button()==Qt::LeftButton&&onClick){
            onClick();
            event->accept();
            return;
        }
        QGraphicsPixmapItem::mousePressEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override{
        if(event->button()==Qt::LeftButton){
            event->accept();
            return;
        }
        QGraphicsPixmapItem::mouseReleaseEvent(event);
    }

private:
};

class LevelClickItem final:public QGraphicsRectItem{
public:
    std::function<void()> onClick;

    LevelClickItem(LevelChoosePage* page,int levelNum,int currentLevel,
                   const QRect& rect,QGraphicsItem* parent=nullptr)
        :QGraphicsRectItem(QRectF(rect),parent),
          owner(page),
          levelNumber(levelNum),
          currentUnlockedLevel(currentLevel){
        setBrush(Qt::NoBrush);
        setPen(Qt::NoPen);
        setAcceptedMouseButtons(Qt::LeftButton);

        QFont font("Microsoft YaHei");
        font.setPixelSize(52);
        font.setWeight(QFont::Black);
        textItem=new QGraphicsTextItem(QString::number(levelNumber),this);
        textItem->setDefaultTextColor(levelTextColor(levelNumber,currentUnlockedLevel));
        textItem->setFont(font);
        textItem->setAcceptedMouseButtons(Qt::NoButton);
        const QRectF textRect=textItem->boundingRect();
        textItem->setPos(
            rect.x()+(rect.width()-textRect.width())/2,
            rect.y()+(rect.height()-textRect.height())/2-5
        );
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override{
        if(event->button()==Qt::LeftButton){
            if(onClick){
                onClick();
            }else if(levelNumber<=currentUnlockedLevel&&owner){
                owner->startLevel(levelNumber);
            }
            event->accept();
            return;
        }
        QGraphicsRectItem::mousePressEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override{
        if(event->button()==Qt::LeftButton){
            event->accept();
            return;
        }
        QGraphicsRectItem::mouseReleaseEvent(event);
    }

private:
    LevelChoosePage* owner=nullptr;
    int levelNumber=0;
    int currentUnlockedLevel=0;
    QGraphicsTextItem* textItem=nullptr;
};

void applyChooseBackground(QWidget* widget){
    QPixmap background(assetPath("images/background/background.png"));
    if(background.isNull()){
        widget->setStyleSheet("background-color: #22262d;");
        return;
    }
    QPalette palette;
    palette.setBrush(QPalette::Window,background.scaled(
        widget->size(),
        Qt::IgnoreAspectRatio,
        Qt::SmoothTransformation
    ));
    widget->setAutoFillBackground(true);
    widget->setPalette(palette);
}

}

LevelChoosePage::LevelChoosePage(QWidget *parent):QDialog(parent){
}

void LevelChoosePage::init(){
    if(chooseView){
        delete chooseView;
        chooseView=nullptr;
    }
    if(chooseScene){
        delete chooseScene;
        chooseScene=nullptr;
    }

    setFixedSize(appWidth,appHeight);
    applyChooseBackground(this);

    chooseScene=new QGraphicsScene(this);
    chooseScene->setSceneRect(0,0,appWidth,appHeight);
    chooseView=new QGraphicsView(chooseScene,this);
    chooseView->setGeometry(0,0,appWidth,appHeight);
    chooseView->setFrameShape(QFrame::NoFrame);
    chooseView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    chooseView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    chooseView->setAlignment(Qt::AlignLeft|Qt::AlignTop);
    chooseView->setStyleSheet("background: transparent;");
    chooseView->show();

    QPixmap backgroundPixmap(assetPath("images/background/background.png"));
    if(!backgroundPixmap.isNull()){
        chooseScene->addPixmap(backgroundPixmap.scaled(
            QSize(appWidth,appHeight),
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation
        ));
    }else{
        chooseScene->setBackgroundBrush(QColor(34,38,44));
    }

    const int totalGridWidth=3*levelButtonWidth+2*levelHorizontalGap;
    const int leftMargin=(appWidth-totalGridWidth)/2;
    const QVector<QPoint> positions={
        QPoint(leftMargin,levelTopMargin),
        QPoint(leftMargin+levelButtonWidth+levelHorizontalGap,levelTopMargin),
        QPoint(leftMargin+2*(levelButtonWidth+levelHorizontalGap),levelTopMargin),
        QPoint(leftMargin+2*(levelButtonWidth+levelHorizontalGap),
            levelTopMargin+levelButtonHeight+levelVerticalGap),
        QPoint(leftMargin+levelButtonWidth+levelHorizontalGap,
            levelTopMargin+levelButtonHeight+levelVerticalGap),
        QPoint(leftMargin,levelTopMargin+levelButtonHeight+levelVerticalGap),
        QPoint(leftMargin,levelTopMargin+2*(levelButtonHeight+levelVerticalGap)),
        QPoint(leftMargin+levelButtonWidth+levelHorizontalGap,
            levelTopMargin+2*(levelButtonHeight+levelVerticalGap)),
        QPoint(leftMargin+2*(levelButtonWidth+levelHorizontalGap),
            levelTopMargin+2*(levelButtonHeight+levelVerticalGap))
    };

    for(int i=0;i<level::TotalLevelCount;i++){
        int levelNum=i+1;
        const QRect levelRect(
            positions[i].x(),
            positions[i].y(),
            levelButtonWidth,
            levelButtonHeight
        );
        const QRect frameRect=levelFrameRectFor(positions[i]);
        chooseScene->addRect(
            QRectF(levelRect),
            Qt::NoPen,
            QBrush(levelBackgroundColor(levelNum,unlockedLevel))
        );
        addFixedImage(chooseScene,"images/icons/level.png",frameRect);

        auto* levelItem=new LevelClickItem(this,levelNum,unlockedLevel,levelRect);
        levelItem->onClick=[this,levelNum](){
            if(levelNum<=unlockedLevel){
                startLevel(levelNum);
            }
        };
        chooseScene->addItem(levelItem);
    }

    addFixedImage(chooseScene,"images/icons/pipe1.png",QRect(454,139,69,23));
    addFixedImage(chooseScene,"images/icons/pipe1.png",QRect(679,139,69,23));
    addFixedImage(chooseScene,"images/icons/pipe1.png",QRect(454,289,69,23));
    addFixedImage(chooseScene,"images/icons/pipe1.png",QRect(679,289,69,23));
    addFixedImage(chooseScene,"images/icons/pipe1.png",QRect(454,439,69,23));
    addFixedImage(chooseScene,"images/icons/pipe1.png",QRect(679,439,69,23));
    addFixedImage(chooseScene,"images/icons/pipeC.png",QRect(904,136,64,179));
    addFixedImage(chooseScene,"images/icons/pipeC.png",QRect(232,286,64,179),true);

    QPixmap returnPixmap(assetPath("images/icons/return.png"));
    if(!returnPixmap.isNull()){
        returnPixmap=returnPixmap.scaled(70,70,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    }
    auto* backItem=new ClickablePixmapItem(returnPixmap,[this](){ close(); });
    backItem->setPos(30,30);
    chooseScene->addItem(backItem);
}

void LevelChoosePage::loadProcess(){
    unlockedLevel=1;
    QFile file(levelSaveFilePath());
    bool needsRewrite=!file.exists();
    if(file.open(QIODevice::ReadOnly|QIODevice::Text)){
        QJsonParseError error;
        const QJsonDocument document=QJsonDocument::fromJson(file.readAll(),&error);
        if(error.error==QJsonParseError::NoError&&document.isObject()){
            unlockedLevel=document.object().value("level").toInt(1);
        }else{
            needsRewrite=true;
        }
    }else{
        needsRewrite=true;
    }
    unlockedLevel=std::max(level::MinLevelNumber,
        std::min(unlockedLevel,level::TotalLevelCount));
    if(needsRewrite){
        writeLevelSave(unlockedLevel);
    }
    init();
}

void LevelChoosePage::saveProcess(){
    unlockedLevel=std::max(level::MinLevelNumber,
        std::min(unlockedLevel,level::TotalLevelCount));
    if(!writeLevelSave(unlockedLevel)){
        message::otherError("进度文件无法打开");
    }
}
