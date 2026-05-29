#include "MainWindow.h"

#include "UiConstants.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QPixmap>
#include <cmath>
#include <functional>

namespace{

constexpr int menuCenterX=600;
constexpr int logoCenterY=200;
constexpr int logoHeight=200;
constexpr int menuButtonHeight=100;
constexpr int levelButtonTop=400;
constexpr int sandboxButtonTop=560;

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

int widthForHeight(const QPixmap& pixmap,int targetHeight,int fallbackWidth){
    if(pixmap.isNull()||pixmap.height()<=0){
        return fallbackWidth;
    }
    return int(std::round(double(targetHeight)*pixmap.width()/pixmap.height()));
}

class MenuButtonItem final:public QGraphicsPixmapItem{
public:
    std::function<void()> onClick;

    MenuButtonItem(const QPixmap& pixmap,const QString& text,
                   QGraphicsItem* parent=nullptr)
        :QGraphicsPixmapItem(pixmap,parent){
        setAcceptedMouseButtons(Qt::LeftButton);

        QFont font("Microsoft YaHei");
        font.setPixelSize(24);
        font.setWeight(QFont::Bold);
        textItem=new QGraphicsTextItem(text,this);
        textItem->setDefaultTextColor(QColor(244,247,251));
        textItem->setFont(font);
        textItem->setAcceptedMouseButtons(Qt::NoButton);
        const QRectF bounds=boundingRect();
        const QRectF textRect=textItem->boundingRect();
        textItem->setPos(
            (bounds.width()-textRect.width())/2,
            (bounds.height()-textRect.height())/2-2
        );
    }

    MenuButtonItem(const QPixmap& pixmap,const QString& text,
                   std::function<void()> clickHandler,
                   QGraphicsItem* parent=nullptr)
        :MenuButtonItem(pixmap,text,parent){
        onClick=std::move(clickHandler);
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
    QGraphicsTextItem* textItem=nullptr;
};

void addScenePixmap(QGraphicsScene* scene,const QString& relativePath,
                    const QRect& rect,Qt::AspectRatioMode aspectMode){
    QPixmap pixmap(assetPath(relativePath));
    if(pixmap.isNull()){
        return;
    }
    QGraphicsPixmapItem* item=scene->addPixmap(
        pixmap.scaled(rect.size(),aspectMode,Qt::SmoothTransformation)
    );
    item->setPos(rect.x(),rect.y());
}

}

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent){
    setFixedSize(appWidth,appHeight);

    menuScene=new QGraphicsScene(this);
    menuScene->setSceneRect(0,0,appWidth,appHeight);

    menuView=new QGraphicsView(menuScene,this);
    menuView->setFrameShape(QFrame::NoFrame);
    menuView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    menuView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    menuView->setAlignment(Qt::AlignLeft|Qt::AlignTop);
    menuView->setFixedSize(appWidth,appHeight);
    setCentralWidget(menuView);

    addScenePixmap(menuScene,"images/background/background.png",
        QRect(0,0,appWidth,appHeight),Qt::IgnoreAspectRatio);

    QPixmap logoPixmap(assetPath("images/bars/logo.png"));
    int logoWidth=widthForHeight(logoPixmap,logoHeight,300);
    if(!logoPixmap.isNull()){
        QGraphicsPixmapItem* logoItem=menuScene->addPixmap(logoPixmap.scaled(
            logoWidth,logoHeight,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        logoItem->setPos(menuCenterX-logoWidth/2,logoCenterY-logoHeight/2);
    }

    QPixmap levelPixmap(assetPath("images/bars/levelmode.png"));
    int levelWidth=widthForHeight(levelPixmap,menuButtonHeight,240);
    levelPixmap=levelPixmap.scaled(
        levelWidth,menuButtonHeight,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    auto* levelItem=new MenuButtonItem(levelPixmap,QString::fromUtf8("关卡模式"),
        [this](){ onLevelButtonClicked(); });
    levelItem->setPos(menuCenterX-levelWidth/2,levelButtonTop);
    menuScene->addItem(levelItem);

    QPixmap sandboxPixmap(assetPath("images/bars/sandboxmode.png"));
    int sandboxWidth=widthForHeight(sandboxPixmap,menuButtonHeight,240);
    sandboxPixmap=sandboxPixmap.scaled(
        sandboxWidth,menuButtonHeight,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    auto* sandboxItem=new MenuButtonItem(sandboxPixmap,QString::fromUtf8("沙盒模式"),
        [this](){ onStartButtonClicked(); });
    sandboxItem->setPos(menuCenterX-sandboxWidth/2,sandboxButtonTop);
    menuScene->addItem(sandboxItem);
}

void MainWindow::onLevelButtonClicked(){
    if(levelChoosePage==nullptr){
        levelChoosePage=new LevelChoosePage(this);
        connect(levelChoosePage,&LevelChoosePage::pageClosed,
                this,&MainWindow::onChooseLevelPageClosed);
    }
    levelChoosePage->loadProcess();
    hide();
    levelChoosePage->show();
}

void MainWindow::onChooseLevelPageClosed(){
    show();
}
