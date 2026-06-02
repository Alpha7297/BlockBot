#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include "UiConstants.h"
#include "AudioManager.h"

#include <QBrush>
#include <QGraphicsPolygonItem>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPen>
#include <QPolygonF>

#include <cmath>
#include <functional>

inline constexpr qreal scrollSliderWidth=18;
inline constexpr qreal scrollSliderHeight=60;

class Button:public QGraphicsPolygonItem{
public:
    int type;
    QString s;
    QGraphicsPolygonItem* shadow;
    std::function<void()> onClick;

    Button(int _type,QString _s,QGraphicsItem* parent=nullptr):QGraphicsPolygonItem(parent){
        s=_s,type=_type;
        setCircleShape();
        setBrush(QColor(80,140,235));
        setPen(Qt::NoPen);

        QPolygonF shadowShape;
        shadowShape<<QPointF(0,0)<<QPointF(24,0)<<QPointF(24,24)<<QPointF(0,24);
        shadow=new QGraphicsPolygonItem();
        shadow->setPolygon(shadowShape);
        shadow->setBrush(QColor(180,180,180,120));
        shadow->setPen(Qt::NoPen);
        shadow->hide();

        setAcceptHoverEvents(true);
        setAcceptedMouseButtons(Qt::LeftButton);
    }

    void setCircleShape(){
        QPolygonF shape;
        double radius=10.0;
        for(int i=0;i<32;i++){
            double angle=2.0*PI*i/32.0;
            shape<<QPointF(radius+radius*std::cos(angle),radius+radius*std::sin(angle));
        }
        setPolygon(shape);
    }

    void setPlayShape(){
        QPolygonF shape;
        shape<<QPointF(3,2)<<QPointF(21,12)<<QPointF(3,22);
        setPolygon(shape);
        setBrush(QColor(44,135,82));
    }

    void setLightningShape(){
        QPolygonF shape;
        shape<<QPointF(14,1)<<QPointF(5,13)<<QPointF(12,13)
             <<QPointF(9,23)<<QPointF(20,10)<<QPointF(13,10);
        setPolygon(shape);
        setBrush(QColor(202,150,36));
    }

    void setStopShape(){
        QPolygonF shape;
        double radius=11.0;
        QPointF center(12,12);
        for(int i=0;i<8;i++){
            double angle=PI/8.0+2.0*PI*i/8.0;
            shape<<QPointF(center.x()+radius*std::cos(angle),center.y()+radius*std::sin(angle));
        }
        setPolygon(shape);
        setBrush(QColor(180,48,48));
    }

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override{
        setCursor(Qt::PointingHandCursor);
        if(scene()!=nullptr&&shadow->scene()==nullptr){
            scene()->addItem(shadow);
        }
        shadow->setPos(pos());
        shadow->setZValue(zValue()-1);
        shadow->show();
        event->accept();
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override{
        unsetCursor();
        shadow->hide();
        event->accept();
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override{
        audio::playSelectSound();
        if(onClick){
            onClick();
        }
        event->accept();
    }
};

class ScrollSlider:public QGraphicsPolygonItem{
public:
    qreal minY;
    qreal maxY;
    qreal dragOffsetY;
    bool dragging;
    std::function<void(qreal)> onChanged;

    ScrollSlider(qreal x,qreal y,qreal height,QGraphicsItem* parent=nullptr):
        QGraphicsPolygonItem(parent){
        minY=y;
        maxY=y+height-scrollSliderHeight;
        dragOffsetY=0;
        dragging=false;
        QPolygonF shape;
        shape<<QPointF(0,0)<<QPointF(scrollSliderWidth,0)
             <<QPointF(scrollSliderWidth,scrollSliderHeight)
             <<QPointF(0,scrollSliderHeight);
        setPolygon(shape);
        setBrush(QColor(75,85,99));
        setPen(QPen(Qt::black,1.5));
        setPos(x,y);
        setAcceptedMouseButtons(Qt::LeftButton);
        setZValue(sliderZ);
    }

    void setBySceneY(qreal sceneY){
        qreal newY=sceneY-dragOffsetY;
        if(newY<minY){
            newY=minY;
        }
        if(newY>maxY){
            newY=maxY;
        }
        setY(newY);
        qreal value=(maxY==minY)?0:(newY-minY)/(maxY-minY);
        if(onChanged){
            onChanged(value);
        }
    }

    qreal value() const{
        return (maxY==minY)?0:(y()-minY)/(maxY-minY);
    }

    void setValue(qreal newValue){
        if(newValue<0){
            newValue=0;
        }
        if(newValue>1){
            newValue=1;
        }
        setY(minY+(maxY-minY)*newValue);
        if(onChanged){
            onChanged(newValue);
        }
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override{
        dragging=true;
        dragOffsetY=event->scenePos().y()-y();
        event->accept();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override{
        if(dragging){
            setBySceneY(event->scenePos().y());
            event->accept();
            return;
        }
        QGraphicsPolygonItem::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override{
        dragging=false;
        event->accept();
    }
};

#endif
