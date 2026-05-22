#include "tale.h"
#include "ui/UiConstants.h"

#include <QApplication>
#include <QBrush>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPen>
#include <QPixmap>
#include <QGraphicsPixmapItem>

namespace tale{

void drawEditorBackground(QGraphicsScene& scene){
    QGraphicsRectItem* map=scene.addRect(stageX,stageY,stagePixelSize,stagePixelSize);
    map->setBrush(QBrush(Qt::white));
    map->setPen(QPen(Qt::black,1.5));

    QGraphicsRectItem* toolbox=scene.addRect(
        toolboxX,
        toolboxY,
        toolboxWidth,
        toolboxHeight);
    toolbox->setBrush(QBrush(Qt::white));
    toolbox->setPen(QPen(Qt::black,1.5));

    QGraphicsRectItem* workspace=scene.addRect(
        workspaceX,
        workspaceY,
        workspaceWidth,
        workspaceHeight);
    workspace->setBrush(QBrush(Qt::white));
    workspace->setPen(QPen(Qt::black,1.5));
}

void drawTale(QGraphicsScene& scene){
    QGraphicsRectItem* background=scene.addRect(0,0,appWidth,appHeight);
    background->setBrush(QBrush(QColor(255,255,255,128)));
    background->setPen(Qt::NoPen);
    background->setZValue(10);
    QGraphicsRectItem* textBox=scene.addRect(200,500,800,250);
    textBox->setBrush(QBrush(QColor(196,226,245,255)));
    textBox->setPen(Qt::NoPen);
    QPixmap robotPixmap("images/robot.png");
    QGraphicsPixmapItem* robotItem=scene.addPixmap(robotPixmap);
    robotItem->setPos(200,300);
    robotItem->setScale(0.1595);
    robotItem->setZValue(100);
    QGraphicsTextItem* text=scene.addText("这一关，你需要计算斐波那契数列的第n项，并将结果输出到ans变量");
    text->setPos(220,520);
    text->setDefaultTextColor(Qt::black);
    text->setFont(QFont("Microsoft YaHei",16));
}

int runEditor(int argc,char* argv[]){
    QApplication app(argc,argv);

    QGraphicsScene scene;
    scene.setSceneRect(0,0,appWidth,appHeight);
    drawEditorBackground(scene);
    drawTale(scene);

    QGraphicsView view(&scene);
    view.setAlignment(Qt::AlignLeft|Qt::AlignTop);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setFixedSize(appWidth,appHeight);
    view.setWindowFlags(view.windowFlags()&~Qt::WindowMaximizeButtonHint);
    view.show();

    return app.exec();
}

}
