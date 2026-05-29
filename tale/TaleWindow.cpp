#include "TaleWindow.h"
#include "../ui/AppGraphicsView.h"

#include <QApplication>
#include <QBrush>
#include <QFont>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPen>
#include <QPixmap>
#include <QDebug>
#include <QTextDocument>

#include <vector>

namespace tale{
namespace{

constexpr int TaleWidth=1200;
constexpr int TaleHeight=800;
constexpr qreal PortraitTop=105;
constexpr int PortraitHeight=420;
constexpr qreal TextBoxX=110;
constexpr qreal TextBoxY=550;
constexpr qreal TextBoxWidth=980;
constexpr qreal TextBoxHeight=150;
constexpr qreal TextPadding=28;

const std::vector<QString> StoryTexts={
    QString::fromUtf8("这里写第一段文字。点击下方文字框进入下一段。"),
    QString::fromUtf8("这里写第二段文字。"),
    QString::fromUtf8("这里写第三段文字。")
};

class DraggablePortrait:public QGraphicsPixmapItem{
public:
    DraggablePortrait(const QPixmap& pixmap):QGraphicsPixmapItem(pixmap){
        setAcceptedMouseButtons(Qt::LeftButton);
        setCursor(Qt::OpenHandCursor);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override{
        dragging=true;
        dragOffset=event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override{
        if(dragging){
            setPos(event->scenePos()-dragOffset);
        }
        event->accept();
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override{
        dragging=false;
        setCursor(Qt::OpenHandCursor);
        qDebug()<<"Robot position:"<<pos();
        event->accept();
    }

private:
    bool dragging=false;
    QPointF dragOffset;
};

class TaleView:public QGraphicsView{
public:
    TaleView(){
        setFixedSize(TaleWidth,TaleHeight);
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setRenderHint(QPainter::Antialiasing,true);
        setScene(&scene);
        scene.setSceneRect(0,0,TaleWidth,TaleHeight);
        drawScene();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override{
        QGraphicsItem* item=itemAt(event->pos());
        if(dynamic_cast<DraggablePortrait*>(item)==nullptr){
            advanceText();
            event->accept();
            return;
        }
        QGraphicsView::mousePressEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override{
        if(event->key()==Qt::Key_Space||event->key()==Qt::Key_Return||
           event->key()==Qt::Key_Enter){
            advanceText();
            event->accept();
            return;
        }
        QGraphicsView::keyPressEvent(event);
    }

private:
    QGraphicsScene scene;
    QGraphicsRectItem* textBox=nullptr;
    QGraphicsTextItem* textItem=nullptr;
    int textIndex=0;

    void drawScene(){
        QPixmap background(loadAsset("images/background/level1.png"));
        if(!background.isNull()){
            scene.addPixmap(background.scaled(TaleWidth,TaleHeight,Qt::IgnoreAspectRatio,
                Qt::SmoothTransformation));
        }
        else{
            scene.setBackgroundBrush(QColor(30,34,40));
        }

        QPixmap robot(loadAsset("images/painting/robot.png"));
        if(!robot.isNull()){
            QPixmap scaledRobot=robot.scaledToHeight(PortraitHeight,Qt::SmoothTransformation);
            DraggablePortrait* portrait=new DraggablePortrait(scaledRobot);
            scene.addItem(portrait);
            portrait->setPos((TaleWidth-scaledRobot.width())/2.0,PortraitTop);
            portrait->setZValue(2);
        }

        textBox=scene.addRect(TextBoxX,TextBoxY,TextBoxWidth,TextBoxHeight,
            QPen(QColor(235,238,242),2),
            QBrush(QColor(24,28,34,220)));
        textBox->setZValue(5);

        textItem=scene.addText(QString(),QFont("Microsoft YaHei",22,QFont::Bold));
        textItem->document()->setDocumentMargin(0);
        textItem->setDefaultTextColor(Qt::white);
        textItem->setTextWidth(TextBoxWidth-TextPadding*2);
        textItem->setPos(TextBoxX+TextPadding,TextBoxY+TextPadding);
        textItem->setZValue(6);
        textItem->setAcceptedMouseButtons(Qt::NoButton);
        updateText();
    }

    void advanceText(){
        if(StoryTexts.empty()){
            return;
        }
        textIndex=(textIndex+1)%static_cast<int>(StoryTexts.size());
        updateText();
    }

    void updateText(){
        if(textItem==nullptr||StoryTexts.empty()){
            return;
        }
        textItem->setPlainText(StoryTexts[textIndex]);
    }
};

}

int runTale(int argc,char* argv[]){
    QApplication app(argc,argv);
    TaleView view;
    view.show();
    return app.exec();
}

}
