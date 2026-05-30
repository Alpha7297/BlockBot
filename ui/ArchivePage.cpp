#include "ArchivePage.h"
#include "AppGraphicsView.h"

#include <QCloseEvent>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>

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
            if(contentLabel!=nullptr){
                contentLabel->setText(QString::number(i+1));
            }
        });
    }

    contentLabel=new QLabel(QString::number(1),this);
    contentLabel->setAlignment(Qt::AlignCenter);
    contentLabel->setGeometry(
        archiveContentX+ArchiveLeftCardWidth,
        ArchiveCardTopMargin,
        ArchiveRightCardWidth,
        ArchiveCardHeight
    );
    contentLabel->setStyleSheet(
        "QLabel { color: white; font-family: 'Microsoft YaHei'; font-size: 72px; font-weight: bold; }");
}

void ArchivePage::closeEvent(QCloseEvent* event){
    emit pageClosed();
    QDialog::closeEvent(event);
}
