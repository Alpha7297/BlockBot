#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include "UiConstants.h"

#include <QBrush>
#include <QGraphicsPolygonItem>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPen>
#include <QPolygonF>

#include <cmath>
#include <functional>

class Button:public QGraphicsPolygonItem{
public:
    int type;
    QString s;
    QGraphicsPolygonItem* shadow;
    std::function<void()> onClick;

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
        maxY=y+height-40;
        dragOffsetY=0;
        dragging=false;
        QPolygonF shape;
        shape<<QPointF(0,0)<<QPointF(12,0)<<QPointF(12,40)<<QPointF(0,40);
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
