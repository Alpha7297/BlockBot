#include "ArchivePage.h"
#include "AppGraphicsView.h"
#include "UiConstants.h"

#include <QCloseEvent>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>

#include <algorithm>
#include <utility>
#include <vector>

namespace{

constexpr int PageWidth=1200;
constexpr int PageHeight=800;
constexpr int LogoHeight=150;
constexpr int LogoTopMargin=80;
constexpr int ArchiveLeftCardWidth=150;
constexpr int ArchiveRightCardWidth=850;
constexpr int ArchiveCardHeight=500;
constexpr int ArchiveCardTopMargin=200;
constexpr int ArchiveRightCardOpacity=200;
constexpr int ArchiveMenuItemCount=9;
constexpr int ArchiveMenuItemWidth=150;
constexpr int ArchiveMenuItemHeight=50;
constexpr int ArchiveMenuItemGap=0;
constexpr int ArchiveMenuTopPadding=0;
constexpr int ArchiveContentPadding=28;
constexpr int ArchiveTitleHeight=44;
constexpr int ArchiveTextWidth=420;
constexpr int ArchiveTextHeight=185;
constexpr int ArchiveProgramPreviewX=490;
constexpr int ArchiveProgramPreviewY=30;
constexpr int ArchiveProgramPreviewWidth=300;
constexpr int ArchiveProgramPreviewHeight=220;
constexpr int ArchiveMovePreviewX=28;
constexpr int ArchiveMovePreviewY=300;
constexpr int ArchiveMovePreviewWidth=650;
constexpr int ArchiveMovePreviewHeight=150;
constexpr int ArchiveBlockHeight=40;
constexpr int ArchiveCostIconSize=33;
constexpr int ArchiveCostGap=8;
constexpr int ArchiveCostTextWidth=46;
constexpr int ArchiveCostItemGap=6;

enum class ArchiveBlockKind{
    Simple,
    FloatCode,
    FloatValue,
    BinaryOp,
    PrefixBinaryOp,
    UnaryOp
};

struct ArchiveBlock{
    QString text;
    QColor color;
    bool topSocket=true;
    bool bottomTab=true;
    ArchiveBlockKind kind=ArchiveBlockKind::Simple;
    QString valueText;
    QString suffixText;
    QString timeCost;
    QString stepCost;
};

QPolygonF archiveBlockPolygon(qreal width,qreal height,bool topSocket,bool bottomTab){
    const qreal connectorRight=codeConnectorX+codeConnectorWidth;
    QPolygonF shape;
    shape<<QPointF(0,0);
    if(topSocket){
        shape<<QPointF(codeConnectorX,0)
             <<QPointF(codeConnectorX,codeConnectorHeight)
             <<QPointF(connectorRight,codeConnectorHeight)
             <<QPointF(connectorRight,0);
    }
    shape<<QPointF(width,0)<<QPointF(width,height);
    if(bottomTab){
        shape<<QPointF(connectorRight,height)
             <<QPointF(connectorRight,height+codeConnectorHeight)
             <<QPointF(codeConnectorX,height+codeConnectorHeight)
             <<QPointF(codeConnectorX,height);
    }
    shape<<QPointF(0,height);
    return shape;
}

QPolygonF archiveFloatPolygon(qreal width,qreal height){
    QPolygonF shape;
    shape<<QPointF(0,0)<<QPointF(width,0)<<QPointF(width,height)<<QPointF(0,height);
    return shape;
}

QFont archiveBlockFont(){
    return QFont("Microsoft YaHei",12);
}

int archiveCostWidth(const ArchiveBlock& block){
    int width=0;
    auto addCost=[&width](){
        if(width>0){
            width+=ArchiveCostItemGap;
        }
        width+=ArchiveCostIconSize+ArchiveCostTextWidth;
    };
    if(!block.timeCost.isEmpty()){
        addCost();
    }
    if(!block.stepCost.isEmpty()){
        addCost();
    }
    return width;
}

class ArchiveBlockPreview:public QWidget{
public:
    enum Layout{
        Vertical,
        Horizontal,
        TwoThenOne
    };

    ArchiveBlockPreview(std::vector<ArchiveBlock> blocks,Layout layout,QWidget* parent=nullptr):
        QWidget(parent),
        blocks(std::move(blocks)),
        layout(layout),
        timePixmap(loadAsset("images/icons/time.png")),
        stepPixmap(loadAsset("images/icons/step.png")){
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
    }

protected:
    void paintEvent(QPaintEvent*) override{
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing,true);
        painter.setFont(archiveBlockFont());

        const int gap=layout==Vertical?0:16;
        int x=0;
        int y=0;
        for(size_t i=0;i<blocks.size();i++){
            const ArchiveBlock& block=blocks[i];
            const int textWidth=painter.fontMetrics().horizontalAdvance(block.text);
            int blockWidth=textWidth+30;
            const bool hasCost=!block.timeCost.isEmpty()||!block.stepCost.isEmpty();
            const int costWidth=hasCost?archiveCostWidth(block)+ArchiveCostGap:0;
            int valueWidth=0;
            int suffixWidth=0;
            int leftWidth=0;
            int rightWidth=0;
            int itemHeight=ArchiveBlockHeight;
            if(block.kind==ArchiveBlockKind::FloatCode){
                valueWidth=std::max(floatBlockWidth,
                    painter.fontMetrics().horizontalAdvance(block.valueText));
                suffixWidth=painter.fontMetrics().horizontalAdvance(block.suffixText);
                blockWidth=20+textWidth+10+valueWidth+10+suffixWidth+10;
            }
            if(block.kind==ArchiveBlockKind::FloatValue){
                blockWidth=std::max(floatBlockWidth,textWidth+12);
                itemHeight=floatBlockWidth;
            }
            if(block.kind==ArchiveBlockKind::BinaryOp||
               block.kind==ArchiveBlockKind::PrefixBinaryOp){
                leftWidth=std::max(floatBlockWidth,
                    painter.fontMetrics().horizontalAdvance(block.valueText)+12);
                rightWidth=std::max(floatBlockWidth,
                    painter.fontMetrics().horizontalAdvance(block.suffixText)+12);
                if(block.kind==ArchiveBlockKind::PrefixBinaryOp){
                    blockWidth=opHorizontalPadding+textWidth+opHorizontalPadding+
                        leftWidth+opHorizontalPadding+rightWidth+opHorizontalPadding;
                }
                else{
                    blockWidth=opHorizontalPadding+leftWidth+opHorizontalPadding+
                        textWidth+opHorizontalPadding+rightWidth+opHorizontalPadding;
                }
                itemHeight=floatBlockWidth+opVerticalPadding*2;
            }
            if(block.kind==ArchiveBlockKind::UnaryOp){
                valueWidth=std::max(floatBlockWidth,
                    painter.fontMetrics().horizontalAdvance(block.valueText)+12);
                blockWidth=opHorizontalPadding+textWidth+opHorizontalPadding+
                    valueWidth+opHorizontalPadding;
                itemHeight=floatBlockWidth+opVerticalPadding*2;
            }
            const int rowHeight=hasCost?std::max(itemHeight,ArchiveCostIconSize):itemHeight;
            const bool floatLike=block.kind==ArchiveBlockKind::FloatValue||
                block.kind==ArchiveBlockKind::BinaryOp||
                block.kind==ArchiveBlockKind::PrefixBinaryOp||
                block.kind==ArchiveBlockKind::UnaryOp;
            QPolygonF shape=floatLike?
                archiveFloatPolygon(blockWidth,floatBlockWidth):
                archiveBlockPolygon(blockWidth,ArchiveBlockHeight,
                    block.topSocket,block.bottomTab);
            if(block.kind==ArchiveBlockKind::BinaryOp||
               block.kind==ArchiveBlockKind::PrefixBinaryOp||
               block.kind==ArchiveBlockKind::UnaryOp){
                shape=archiveFloatPolygon(blockWidth,itemHeight);
            }
            shape.translate(x,y);

            painter.setPen(QPen(Qt::black,2));
            painter.setBrush(block.color);
            painter.drawPolygon(shape);

            painter.setPen(Qt::white);
            if(block.kind==ArchiveBlockKind::FloatValue){
                QRect textRect(x,y,blockWidth,floatBlockWidth);
                painter.drawText(textRect,Qt::AlignCenter,block.text);
            }
            else if(block.kind==ArchiveBlockKind::BinaryOp||
                    block.kind==ArchiveBlockKind::PrefixBinaryOp){
                int boxY=y+opVerticalPadding;
                int leftX=x+opHorizontalPadding;
                int textX=leftX+leftWidth+opHorizontalPadding;
                int rightX=textX+textWidth+opHorizontalPadding;
                if(block.kind==ArchiveBlockKind::PrefixBinaryOp){
                    textX=x+opHorizontalPadding;
                    leftX=textX+textWidth+opHorizontalPadding;
                    rightX=leftX+leftWidth+opHorizontalPadding;
                }
                drawFloatSlot(painter,leftX,boxY,leftWidth,block.valueText);
                painter.setPen(Qt::white);
                painter.drawText(QRect(textX,y,textWidth,itemHeight),
                    Qt::AlignCenter,block.text);
                drawFloatSlot(painter,rightX,boxY,rightWidth,block.suffixText);
            }
            else if(block.kind==ArchiveBlockKind::UnaryOp){
                int boxY=y+opVerticalPadding;
                int textX=x+opHorizontalPadding;
                int valueX=textX+textWidth+opHorizontalPadding;
                painter.drawText(QRect(textX,y,textWidth,itemHeight),
                    Qt::AlignCenter,block.text);
                drawFloatSlot(painter,valueX,boxY,valueWidth,block.valueText);
            }
            else if(block.kind==ArchiveBlockKind::FloatCode){
                QRect textRect(x+10,y,textWidth+4,ArchiveBlockHeight);
                painter.drawText(textRect,Qt::AlignVCenter|Qt::AlignLeft,block.text);

                const int valueX=x+20+textWidth;
                const int valueY=y+(ArchiveBlockHeight-floatBlockWidth)/2;
                QPolygonF valueShape=archiveFloatPolygon(valueWidth,floatBlockWidth);
                valueShape.translate(valueX,valueY);
                painter.setBrush(QColor(92,102,116));
                painter.setPen(QPen(Qt::black,2));
                painter.drawPolygon(valueShape);

                painter.setPen(Qt::white);
                painter.drawText(QRect(valueX,valueY,valueWidth,floatBlockWidth),
                    Qt::AlignCenter,block.valueText);
                painter.drawText(
                    QRect(valueX+valueWidth+10,y,suffixWidth+4,ArchiveBlockHeight),
                    Qt::AlignVCenter|Qt::AlignLeft,
                    block.suffixText
                );
            }
            else{
                QRect textRect(x+10,y,blockWidth-20,ArchiveBlockHeight);
                painter.drawText(textRect,Qt::AlignCenter,block.text);
            }
            if(hasCost){
                const int costX=x+blockWidth+ArchiveCostGap;
                const int costY=y+(rowHeight-ArchiveCostIconSize)/2;
                drawCosts(painter,costX,costY,block);
            }

            if(layout==Vertical){
                y+=rowHeight;
            }
            else if(layout==TwoThenOne&&i==1){
                x=0;
                y+=ArchiveBlockHeight+18;
            }
            else{
                x+=blockWidth+costWidth+gap;
            }
        }
    }

private:
    void drawFloatSlot(QPainter& painter,int x,int y,int width,const QString& text){
        QPolygonF valueShape=archiveFloatPolygon(width,floatBlockWidth);
        valueShape.translate(x,y);
        painter.setBrush(QColor(92,102,116));
        painter.setPen(QPen(Qt::black,2));
        painter.drawPolygon(valueShape);
        painter.setPen(Qt::white);
        painter.drawText(QRect(x,y,width,floatBlockWidth),Qt::AlignCenter,text);
    }

    void drawCosts(QPainter& painter,int x,int y,const ArchiveBlock& block){
        int currX=x;
        auto drawOne=[&](const QPixmap& pixmap,const QString& value){
            if(value.isEmpty()){
                return;
            }
            drawCost(painter,pixmap,currX,y,value);
            currX+=ArchiveCostIconSize+ArchiveCostTextWidth+ArchiveCostItemGap;
        };
        drawOne(timePixmap,block.timeCost);
        drawOne(stepPixmap,block.stepCost);
    }

    void drawCost(QPainter& painter,const QPixmap& pixmap,int x,int y,const QString& value){
        if(value.isEmpty()){
            return;
        }
        QRect iconRect(x,y,ArchiveCostIconSize,ArchiveCostIconSize);
        if(!pixmap.isNull()){
            painter.drawPixmap(iconRect,pixmap.scaled(
                ArchiveCostIconSize,
                ArchiveCostIconSize,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            ));
        }
        else{
            painter.setBrush(QColor(220,220,220));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(iconRect);
        }
        painter.setPen(Qt::white);
        painter.setFont(archiveBlockFont());
        painter.drawText(
            QRect(x+ArchiveCostIconSize+2,y,ArchiveCostTextWidth,ArchiveCostIconSize),
            Qt::AlignVCenter|Qt::AlignLeft,
            QString::fromUtf8("×%1").arg(value)
        );
    }

    std::vector<ArchiveBlock> blocks;
    Layout layout;
    QPixmap timePixmap;
    QPixmap stepPixmap;
};

}

ArchivePage::ArchivePage(QWidget* parent):QDialog(parent){
}

void ArchivePage::init(){
    for(QWidget* child:findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)){
        delete child;
    }

    setFixedSize(PageWidth,PageHeight);
    setStyleSheet(QString("QDialog { border-image: url(%1) 0 0 0 0 stretch stretch; }")
        .arg(loadAsset("images/background/background.png")));

    QPushButton* backBtn=new QPushButton(this);
    backBtn->setGeometry(15,15,80,80);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(QString(
        "QPushButton { border: none; border-image: url(%1) 0 0 0 0 stretch stretch; }")
        .arg(loadAsset("images/icons/return.png")));
    connect(backBtn,&QPushButton::clicked,this,&ArchivePage::close);

    QLabel* logo=new QLabel(this);
    QPixmap logoPixmap(loadAsset("images/bars/archive_logo.png"));
    if(!logoPixmap.isNull()){
        int logoWidth=logoPixmap.width()*LogoHeight/logoPixmap.height();
        logo->setGeometry((PageWidth-logoWidth)/2,LogoTopMargin,logoWidth,LogoHeight);
        logo->setPixmap(logoPixmap.scaledToHeight(LogoHeight,Qt::SmoothTransformation));
    }
    else{
        logo->setGeometry((PageWidth-520)/2,LogoTopMargin,520,LogoHeight);
    }

    const int archiveContentWidth=ArchiveLeftCardWidth+ArchiveRightCardWidth;
    const int archiveContentX=(PageWidth-archiveContentWidth)/2;

    QLabel* rightCard=new QLabel(this);
    rightCard->setGeometry(
        archiveContentX+ArchiveLeftCardWidth,
        ArchiveCardTopMargin,
        ArchiveRightCardWidth,
        ArchiveCardHeight
    );
    rightCard->setStyleSheet(QString(
        "QLabel { background-color: rgba(120, 120, 120, %1); border: none; }")
        .arg(ArchiveRightCardOpacity));

    for(int i=0;i<ArchiveMenuItemCount;i++){
        QPushButton* itemButton=new QPushButton(QString::number(i+1),this);
        itemButton->setGeometry(
            archiveContentX+(ArchiveLeftCardWidth-ArchiveMenuItemWidth)/2,
            ArchiveCardTopMargin+ArchiveMenuTopPadding+i*(ArchiveMenuItemHeight+ArchiveMenuItemGap),
            ArchiveMenuItemWidth,
            ArchiveMenuItemHeight
        );
        itemButton->setCursor(Qt::PointingHandCursor);
        itemButton->setStyleSheet(QString(
            "QPushButton {"
            "  color: white;"
            "  font-family: 'Microsoft YaHei';"
            "  font-size: 20px;"
            "  font-weight: bold;"
            "  border: none;"
            "  border-image: url(%1) 0 0 0 0 stretch stretch;"
            "}")
            .arg(loadAsset("images/bars/archive_select.png")));
        connect(itemButton,&QPushButton::clicked,this,[this,i](){
            showArchivePage(i);
        });
    }

    contentPanel=new QWidget(this);
    contentPanel->setGeometry(
        archiveContentX+ArchiveLeftCardWidth,
        ArchiveCardTopMargin,
        ArchiveRightCardWidth,
        ArchiveCardHeight
    );
    contentPanel->setStyleSheet("QWidget { background: transparent; }");
    showArchivePage(0);
}

void ArchivePage::showArchivePage(int index){
    if(contentPanel==nullptr){
        return;
    }
    for(QWidget* child:contentPanel->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)){
        delete child;
    }
    contentLabel=nullptr;

    auto showChildren=[this](){
        for(QWidget* child:contentPanel->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)){
            child->show();
        }
    };
    auto addTitle=[this](const QString& text,int x,int y,int w=ArchiveTextWidth){
        QLabel* label=new QLabel(text,contentPanel);
        label->setGeometry(x,y,w,ArchiveTitleHeight);
        label->setStyleSheet(
            "QLabel { color: white; font-family: 'Microsoft YaHei'; font-size: 26px; font-weight: bold; }");
        return label;
    };
    auto addText=[this](const QString& text,int x,int y,int w,int h,int fontSize=18){
        QLabel* label=new QLabel(text,contentPanel);
        label->setGeometry(x,y,w,h);
        label->setWordWrap(true);
        label->setAlignment(Qt::AlignLeft|Qt::AlignTop);
        label->setStyleSheet(QString(
            "QLabel { color: white; font-family: 'Microsoft YaHei'; font-size: %1px; line-height: 145%; }")
            .arg(fontSize));
        return label;
    };

    if(index>=4){
        contentLabel=new QLabel(QString::number(index+1),contentPanel);
        contentLabel->setAlignment(Qt::AlignCenter);
        contentLabel->setGeometry(0,0,ArchiveRightCardWidth,ArchiveCardHeight);
        contentLabel->setStyleSheet(
            "QLabel { color: white; font-family: 'Microsoft YaHei'; font-size: 72px; font-weight: bold; }");
        contentLabel->show();
        return;
    }

    if(index==1){
        addTitle(QString::fromUtf8("设置界面"),ArchiveContentPadding,18);
        addText(QString::fromUtf8("打开设置界面，可以调整工作区长度、程序运行速度。"),
            ArchiveContentPadding,70,360,70);

        addTitle(QString::fromUtf8("时间"),ArchiveContentPadding,145);
        addText(QString::fromUtf8(
            "时间是游戏变化的指标，显示在舞台左下角。当时间发生变化时，地图和机器人位置都有可能变化。"),
            ArchiveContentPadding,197,390,112);

        addTitle(QString::fromUtf8("使用等待"),ArchiveContentPadding,318);
        addText(QString::fromUtf8("在运行中使用等待积木可以观察地刺移动规律。"),
            ArchiveContentPadding,370,390,60);

        auto* waitPreview=new ArchiveBlockPreview({
            {QString::fromUtf8("等待"),Qt::blue,true,true,ArchiveBlockKind::FloatCode,
                QString::fromUtf8("x"),QString::fromUtf8("帧"),
                QString::fromUtf8("x"),QString::fromUtf8("x")}
        },ArchiveBlockPreview::Horizontal,contentPanel);
        waitPreview->setGeometry(455,340,340,80);

        showChildren();
        return;
    }

    if(index==2){
        addTitle(QString::fromUtf8("学习运算"),ArchiveContentPadding,18);
        addText(QString::fromUtf8("可以使用四则运算，以及最大值、最小值运算。"),
            ArchiveContentPadding,68,370,88);
        auto* opPreview=new ArchiveBlockPreview({
            {QString::fromUtf8("+"),QColor(42,105,86),true,true,ArchiveBlockKind::BinaryOp,
                QString::fromUtf8("a"),QString::fromUtf8("b"),QString(),QString::fromUtf8("1")},
            {QString::fromUtf8("-"),QColor(42,105,86),true,true,ArchiveBlockKind::BinaryOp,
                QString::fromUtf8("a"),QString::fromUtf8("b"),QString(),QString::fromUtf8("1")}
        },ArchiveBlockPreview::Horizontal,contentPanel);
        opPreview->setGeometry(420,38,420,48);
        auto* opPreviewMulDiv=new ArchiveBlockPreview({
            {QString::fromUtf8("*"),QColor(42,105,86),true,true,ArchiveBlockKind::BinaryOp,
                QString::fromUtf8("a"),QString::fromUtf8("b"),QString(),QString::fromUtf8("1")},
            {QString::fromUtf8("/"),QColor(42,105,86),true,true,ArchiveBlockKind::BinaryOp,
                QString::fromUtf8("a"),QString::fromUtf8("b"),QString(),QString::fromUtf8("1")}
        },ArchiveBlockPreview::Horizontal,contentPanel);
        opPreviewMulDiv->setGeometry(420,86,420,48);
        auto* opPreview2=new ArchiveBlockPreview({
            {QString::fromUtf8("max"),QColor(42,105,86),true,true,ArchiveBlockKind::PrefixBinaryOp,
                QString::fromUtf8("a"),QString::fromUtf8("b"),QString(),QString::fromUtf8("1")},
            {QString::fromUtf8("min"),QColor(42,105,86),true,true,ArchiveBlockKind::PrefixBinaryOp,
                QString::fromUtf8("a"),QString::fromUtf8("b"),QString(),QString::fromUtf8("1")}
        },ArchiveBlockPreview::Horizontal,contentPanel);
        opPreview2->setGeometry(420,134,400,48);

        addTitle(QString::fromUtf8("学习变量"),ArchiveContentPadding,210);
        addText(QString::fromUtf8(
            "变量可以存储数据，在左侧可以创建新变量。变量读取块会读出变量当前内容。"),
            ArchiveContentPadding,260,360,86);
        auto* varReadPreview=new ArchiveBlockPreview({
            {QString::fromUtf8("我的变量"),QColor(194,92,0),true,true,ArchiveBlockKind::FloatValue}
        },ArchiveBlockPreview::Horizontal,contentPanel);
        varReadPreview->setGeometry(435,240,220,55);
        auto* varWritePreview=new ArchiveBlockPreview({
            {QString::fromUtf8("将我的变量设定为"),QColor(194,92,0),true,true,
                ArchiveBlockKind::FloatCode,QString::fromUtf8("0"),QString(),
                QString(),QString::fromUtf8("1")},
            {QString::fromUtf8("将我的变量增加"),QColor(194,92,0),true,true,
                ArchiveBlockKind::FloatCode,QString::fromUtf8("0"),QString(),
                QString(),QString::fromUtf8("1")}
        },ArchiveBlockPreview::Vertical,contentPanel);
        varWritePreview->setGeometry(435,310,370,100);

        showChildren();
        return;
    }

    if(index==3){
        addTitle(QString::fromUtf8("学习列表"),ArchiveContentPadding,16);
        addText(QString::fromUtf8("列表存储一系列数据，可以读取某一项，也可以读取列表长度。"),
            ArchiveContentPadding,64,360,70);
        auto* listReadPreview=new ArchiveBlockPreview({
            {QString::fromUtf8("我的列表的第 1 项"),QColor(125,28,38),true,true,
                ArchiveBlockKind::FloatValue,QString(),QString(),QString(),QString::fromUtf8("1")},
            {QString::fromUtf8("我的列表的长度"),QColor(125,28,38),true,true,
                ArchiveBlockKind::FloatValue,QString(),QString(),QString(),QString::fromUtf8("1")}
        },ArchiveBlockPreview::Vertical,contentPanel);
        listReadPreview->setGeometry(435,28,380,80);
        auto* listWritePreview=new ArchiveBlockPreview({
            {QString::fromUtf8("在我的列表末尾添加"),QColor(125,28,38),true,true,
                ArchiveBlockKind::FloatCode,QString::fromUtf8("0"),QString(),
                QString(),QString::fromUtf8("1")},
            {QString::fromUtf8("清空我的列表"),QColor(125,28,38),true,true,
                ArchiveBlockKind::Simple,QString(),QString(),QString(),QString::fromUtf8("x")}
        },ArchiveBlockPreview::Vertical,contentPanel);
        listWritePreview->setGeometry(435,132,390,95);
        addText(QString::fromUtf8("清空列表的步数为 x，x 是列表当前长度。"),
            ArchiveContentPadding,136,360,50);

        addTitle(QString::fromUtf8("学习布尔"),ArchiveContentPadding,230);
        addText(QString::fromUtf8(
            "布尔值一般用 0 和 1 表示，在本游戏中简化为 0 和非 0。"),
            ArchiveContentPadding,280,380,100);
        auto* boolPreview=new ArchiveBlockPreview({
            {QString::fromUtf8("=="),QColor(42,105,86),true,true,ArchiveBlockKind::BinaryOp,
                QString::fromUtf8("a"),QString::fromUtf8("b"),QString(),QString::fromUtf8("1")},
            {QString::fromUtf8(">"),QColor(42,105,86),true,true,ArchiveBlockKind::BinaryOp,
                QString::fromUtf8("a"),QString::fromUtf8("b"),QString(),QString::fromUtf8("1")},
            {QString::fromUtf8("<"),QColor(42,105,86),true,true,ArchiveBlockKind::BinaryOp,
                QString::fromUtf8("a"),QString::fromUtf8("b"),QString(),QString::fromUtf8("1")},
            {QString::fromUtf8("not"),QColor(42,105,86),true,true,ArchiveBlockKind::UnaryOp,
                QString::fromUtf8("x"),QString(),QString(),QString::fromUtf8("1")}
        },ArchiveBlockPreview::TwoThenOne,contentPanel);
        boolPreview->setGeometry(400,310,430,120);

        showChildren();
        return;
    }

    QLabel* title=new QLabel(QString::fromUtf8("如何写一个完整的程序"),contentPanel);
    title->setGeometry(ArchiveContentPadding,20,ArchiveTextWidth,ArchiveTitleHeight);
    title->setStyleSheet(
        "QLabel { color: white; font-family: 'Microsoft YaHei'; font-size: 28px; font-weight: bold; }");

    QLabel* text=new QLabel(contentPanel);
    text->setGeometry(ArchiveContentPadding,76,ArchiveTextWidth,ArchiveTextHeight);
    text->setWordWrap(true);
    text->setAlignment(Qt::AlignLeft|Qt::AlignTop);
    text->setText(QString::fromUtf8(
        "程序必须从“开始”积木块开始顺序运行，直到“结束”积木块结束。\n\n"
        "如右侧代码完成了向左侧移动一步："));
    text->setStyleSheet(
        "QLabel { color: white; font-family: 'Microsoft YaHei'; font-size: 20px; line-height: 150%; }");

    auto* programPreview=new ArchiveBlockPreview({
        {QString::fromUtf8("开始"),QColor(156,118,42),false,true},
        {QString::fromUtf8("左转"),Qt::blue,true,true},
        {QString::fromUtf8("向前移动"),Qt::blue,true,true,ArchiveBlockKind::FloatCode,
            QString::fromUtf8("1"),QString::fromUtf8("步")},
        {QString::fromUtf8("结束"),QColor(156,118,42),true,false}
    },ArchiveBlockPreview::Vertical,contentPanel);
    programPreview->setGeometry(
        ArchiveProgramPreviewX,
        ArchiveProgramPreviewY,
        ArchiveProgramPreviewWidth,
        ArchiveProgramPreviewHeight
    );

    QLabel* moveTitle=new QLabel(QString::fromUtf8("移动积木"),contentPanel);
    moveTitle->setGeometry(ArchiveContentPadding,250,ArchiveTextWidth,36);
    moveTitle->setStyleSheet(
        "QLabel { color: white; font-family: 'Microsoft YaHei'; font-size: 24px; font-weight: bold; }");


    auto* movePreview=new ArchiveBlockPreview({
        {QString::fromUtf8("左转"),Qt::blue,true,true,ArchiveBlockKind::Simple,
            QString(),QString(),QString::fromUtf8("1"),QString::fromUtf8("1")},
        {QString::fromUtf8("右转"),Qt::blue,true,true,ArchiveBlockKind::Simple,
            QString(),QString(),QString::fromUtf8("1"),QString::fromUtf8("1")},
        {QString::fromUtf8("向前移动"),Qt::blue,true,true,ArchiveBlockKind::FloatCode,
            QString::fromUtf8("x"),QString::fromUtf8("步"),
            QString::fromUtf8("x"),QString::fromUtf8("x")}
    },ArchiveBlockPreview::TwoThenOne,contentPanel);
    movePreview->setGeometry(
        ArchiveMovePreviewX,
        ArchiveMovePreviewY,
        ArchiveMovePreviewWidth,
        ArchiveMovePreviewHeight
    );

    for(QWidget* child:contentPanel->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)){
        child->show();
    }
}

void ArchivePage::closeEvent(QCloseEvent* event){
    emit pageClosed();
    QDialog::closeEvent(event);
}
