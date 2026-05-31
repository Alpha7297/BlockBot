#include "SettingsDialog.h"
#include "parameter.h"
#include "AppGraphicsView.h"
#include "UiConstants.h"

#include <QBrush>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QTextDocument>
#include <QTransform>

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>

namespace settings{
namespace{

constexpr qreal BackButtonX=24;
constexpr qreal BackButtonY=24;
constexpr qreal BackButtonSize=60;
constexpr qreal LogoTopY=40;
constexpr int LogoHeight=100;
constexpr qreal LengthLabelX=200;
constexpr qreal LengthLabelY=250;
constexpr qreal LengthTrackX=400;
constexpr qreal LengthTrackY=265;
constexpr qreal LengthTrackWidth=400;
constexpr qreal LengthTrackHeight=20;
constexpr qreal LengthValueX=900;
constexpr qreal LengthValueY=300;
constexpr qreal RowGap=105;
constexpr qreal SliderKnobWidth=60;
constexpr qreal SliderKnobHeight=30;

constexpr std::array<int,7> LengthValues={1,2,5,10,20,50,100};
constexpr std::array<int,8> SpeedValues={1,5,10,20,50,100,200,500};

template<size_t N>
int nearestValueIndex(const std::array<int,N>& values,int value){
    int bestIndex=0;
    int bestDistance=std::abs(value-values[0]);
    for(int i=1;i<static_cast<int>(values.size());i++){
        int distance=std::abs(value-values[i]);
        if(distance<bestDistance){
            bestDistance=distance;
            bestIndex=i;
        }
    }
    return bestIndex;
}

class IconButtonItem:public QGraphicsPixmapItem{
public:
    std::function<void()> onClick;

    IconButtonItem(const QPixmap& pixmap,qreal size,QGraphicsItem* parent=nullptr):
        QGraphicsPixmapItem(parent){
        setPixmap(pixmap.scaled(size,size,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        setAcceptedMouseButtons(Qt::LeftButton);
        setCursor(Qt::PointingHandCursor);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override{
        if(onClick){
            onClick();
        }
        event->accept();
    }
};

template<size_t N>
class DiscreteSlider:public QGraphicsPixmapItem{
public:
    QGraphicsRectItem* progress=nullptr;
    QGraphicsTextItem* valueText=nullptr;
    std::function<void(int)> onChanged;
    std::function<QString(int)> displayText;

    DiscreteSlider(const QPixmap& sliderPixmap,const std::array<int,N>& values,
                   int initialValue,qreal trackX,qreal trackY,qreal trackWidth,
                   QGraphicsItem* parent=nullptr):
        QGraphicsPixmapItem(parent),
        values(values),
        trackX(trackX),
        trackY(trackY),
        trackWidth(trackWidth){
        QPixmap rotated=sliderPixmap.transformed(QTransform().rotate(90),
            Qt::SmoothTransformation);
        setPixmap(rotated.scaled(SliderKnobWidth,SliderKnobHeight,
            Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
        setAcceptedMouseButtons(Qt::LeftButton);
        setCursor(Qt::PointingHandCursor);
        setIndex(nearestValueIndex(values,initialValue));
    }

    void setIndex(int newIndex){
        index=std::clamp(newIndex,0,static_cast<int>(values.size())-1);
        int selectedValue=values[index];
        qreal x=trackX+stepWidth()*index-SliderKnobWidth/2.0;
        qreal y=trackY+LengthTrackHeight/2.0-SliderKnobHeight/2.0;
        setPos(x,y);
        if(progress!=nullptr){
            progress->setRect(trackX,trackY,stepWidth()*index,LengthTrackHeight);
        }
        if(valueText!=nullptr){
            valueText->setPlainText(displayText?displayText(selectedValue):
                QString::number(selectedValue));
        }
        if(onChanged){
            onChanged(selectedValue);
        }
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override{
        setIndexFromSceneX(event->scenePos().x());
        event->accept();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override{
        setIndexFromSceneX(event->scenePos().x());
        event->accept();
    }

private:
    int index=0;
    std::array<int,N> values;
    qreal trackX;
    qreal trackY;
    qreal trackWidth;

    qreal stepWidth() const{
        return trackWidth/(values.size()-1);
    }

    void setIndexFromSceneX(qreal sceneX){
        qreal ratio=(sceneX-trackX)/trackWidth;
        int newIndex=int(std::round(std::clamp(ratio,0.0,1.0)*
            (values.size()-1)));
        setIndex(newIndex);
    }
};

QGraphicsTextItem* addText(QGraphicsScene* scene,const QString& text,QPointF pos,
                           int pixelSize,QColor color=Qt::white){
    QGraphicsTextItem* item=scene->addText(text,QFont("Microsoft YaHei",pixelSize,QFont::Bold));
    item->setDefaultTextColor(color);
    item->document()->setDocumentMargin(0);
    item->setPos(pos);
    return item;
}

template<size_t N>
void addDiscreteSlider(QGraphicsScene* scene,const QString& label,
                       const std::array<int,N>& values,int initialValue,
                       qreal rowY,std::function<void(int)> onChanged,
                       std::function<QString(int)> displayText=nullptr){
    addText(scene,label,QPointF(LengthLabelX,rowY),24);
    QGraphicsTextItem* valueText=addText(scene,QString(),
        QPointF(LengthValueX,rowY),24);
    valueText->setTextWidth(120);

    qreal trackY=rowY+(LengthTrackY-LengthLabelY);
    scene->addRect(LengthTrackX,trackY,LengthTrackWidth,LengthTrackHeight,
        QPen(Qt::NoPen),QBrush(QColor(110,110,110,190)));
    QGraphicsRectItem* progress=scene->addRect(LengthTrackX,trackY,0,
        LengthTrackHeight,QPen(Qt::NoPen),QBrush(QColor(225,225,225,210)));

    QPixmap sliderPixmap(loadAsset("images/icons/slider2.png"));
    auto* slider=new DiscreteSlider<N>(sliderPixmap,values,initialValue,
        LengthTrackX,trackY,LengthTrackWidth);
    slider->progress=progress;
    slider->valueText=valueText;
    slider->onChanged=onChanged;
    slider->displayText=displayText;
    scene->addItem(slider);
    slider->setIndex(nearestValueIndex(values,initialValue));
}

QString speedText(int value){
    QString prefix=QString("%1ms").arg(value);
    return prefix;
}

}

SettingsDialog::SettingsDialog(QWidget* parent):QDialog(parent){
    setWindowTitle("Settings");
    setFixedSize(appWidth,appHeight);

    scene=new QGraphicsScene(this);
    scene->setSceneRect(0,0,appWidth,appHeight);

    view=new QGraphicsView(scene,this);
    view->setGeometry(0,0,appWidth,appHeight);
    view->setFrameShape(QFrame::NoFrame);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setRenderHint(QPainter::Antialiasing,true);

    QPixmap background(loadAsset("images/background/background.png"));
    if(!background.isNull()){
        scene->addPixmap(background.scaled(appWidth,appHeight,Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation));
    }
    else{
        scene->setBackgroundBrush(QColor(34,38,44));
    }

    QPixmap returnPixmap(loadAsset("images/icons/return.png"));
    IconButtonItem* backButton=new IconButtonItem(returnPixmap,BackButtonSize);
    backButton->setPos(BackButtonX,BackButtonY);
    backButton->onClick=[this](){
        accept();
    };
    scene->addItem(backButton);

    QPixmap logoPixmap(loadAsset("images/bars/settings.png"));
    if(!logoPixmap.isNull()){
        QPixmap scaledLogo=logoPixmap.scaledToHeight(LogoHeight,Qt::SmoothTransformation);
        QGraphicsPixmapItem* logo=scene->addPixmap(scaledLogo);
        logo->setPos((appWidth-scaledLogo.width())/2.0,LogoTopY);
    }

    addDiscreteSlider(scene,QString::fromUtf8("工作区长度"),LengthValues,
        WorkspaceEditLength,LengthLabelY,[](int value){
            WorkspaceEditLength=value;
        });
    addDiscreteSlider(scene,QString::fromUtf8("移动速度"),SpeedValues,RuntimeActionBlockIntervalMs,
        LengthLabelY+RowGap,[](int value){
            RuntimeActionBlockIntervalMs=value;
        },speedText);
    addDiscreteSlider(scene,QString::fromUtf8("运行速度"),SpeedValues,RuntimeCodeBlockIntervalMs,
        LengthLabelY+RowGap*2,[](int value){
            RuntimeCodeBlockIntervalMs=value;
        },speedText);
}

}
