#include "levelchoosepage.h"
#include "fstream"
#include "../level/LevelConstants.h"
#include"../message/Message.h"
#include <QCoreApplication>
#include <QDialog>
#include <QDir>
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
#include <QLabel>
#include <algorithm>

namespace{

QString levelSaveFilePath(){
    QString dir=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if(dir.isEmpty()){
        dir=QCoreApplication::applicationDirPath();
    }
    QDir().mkpath(dir);
    return QDir(dir).filePath("level.json");
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
int loadLevel(){
    QFile file(levelSaveFilePath());
    if(!file.exists())return -1;
    if(file.open(QIODevice::ReadOnly|QIODevice::Text)){
        QJsonParseError error;
        const QJsonDocument document=QJsonDocument::fromJson(file.readAll(),&error);
        if(error.error==QJsonParseError::NoError&&document.isObject()){
            return document.object().value("level").toInt(1);
        }else{
            return -1;
        }
    }else{
        return -1;
    }
    return -1;
}
}

LevelChoosePage::LevelChoosePage(QWidget *parent):QDialog(parent){
}

void LevelChoosePage::init()
{
    for(QWidget* child:findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)){
        delete child;
    }
    levels.clear();

    const int levelButtonHeight=100,levelButtonWidth=150,pageHeight=800,pageWidth=1200,topMargin=150,widthGap=75,heightGap=50;
    this->setFixedSize(pageWidth, pageHeight); // 假设背景图是这个尺寸
    this->setStyleSheet(QString("QDialog { border-image: url(%1) 0 0 0 0 stretch stretch; }").arg(loadAsset("images/background/background.png")));

    QPoint levelPositions[] = {//精准测量的关卡按钮坐标 (X, Y)
        QPoint((-levelButtonWidth+pageWidth)/2-levelButtonWidth-widthGap,  topMargin),  // 关卡 1
        QPoint((-levelButtonWidth+pageWidth)/2,      topMargin),  // 关卡 2
        QPoint((-levelButtonWidth+pageWidth)/2+levelButtonWidth+widthGap,  topMargin),  // 关卡 3
        QPoint((-levelButtonWidth+pageWidth)/2+levelButtonWidth+widthGap,  topMargin+levelButtonHeight+heightGap),  // 关卡 4
        QPoint((-levelButtonWidth+pageWidth)/2,      topMargin+levelButtonHeight+heightGap),  // 关卡 5
        QPoint((-levelButtonWidth+pageWidth)/2-levelButtonWidth-widthGap,  topMargin+levelButtonHeight+heightGap),  // 关卡 6
        QPoint((-levelButtonWidth+pageWidth)/2-levelButtonWidth-widthGap,  topMargin+levelButtonHeight*2+heightGap*2),  // 关卡 7
        QPoint((-levelButtonWidth+pageWidth)/2,      topMargin+levelButtonHeight*2+heightGap*2),  // 关卡 8
        QPoint((-levelButtonWidth+pageWidth)/2+levelButtonWidth+widthGap,  topMargin+levelButtonHeight*2+heightGap*2)   // 关卡 9
    };
    totalLevels = 9; //共有9关
    levels.resize(totalLevels);
    // 批量生成并配置按钮
    for (int i = 0; i < totalLevels; ++i) {
        int levelNum = i + 1;

        if(levels[i]==nullptr)levels[i] = new QPushButton(this);//QString::number(levelNum),this);
        levels[i]->setProperty("levelNumber", levelNum);
        levels[i]->setCursor(Qt::PointingHandCursor);
        levels[i]->setFixedSize(levelButtonWidth, levelButtonHeight); // 普通灰色金属框大小
        levels[i]->move(levelPositions[i]);// 搬移到背景图对应的管道口位置
        // 根据 unlockedLevel 判断关卡状态并上色
        QString templateURL;
        QString qss;
        if (levelNum < unlockedLevel) {
            // 【状态 A：已通关】显示正常缩略图
            templateURL=loadAsset(QString("images/background/level_%1_normal.png").arg(""));//levelNum);
        }
        else if (levelNum == unlockedLevel) {
            // 【状态 B：当前正在挑战的关卡】显示高亮红框（或者根据图片，给按钮套上亮色皮肤）
            templateURL=loadAsset(QString("images/background/level_%1_current.png").arg(""));//levelNum);
        }
        else {
            // 【状态 C：未解锁】置灰，并且在右下角带一把锁
            templateURL=loadAsset(QString("images/background/level_%1_gray.png").arg(""));//levelNum);
            levels[i]->setEnabled(false);
        }
        levels[i]->setStyleSheet("QPushButton { border: none; background: transparent; }");
        // 创建一个独立的 QLabel 塞进按钮里，用来精准控制图片的大小
        QLabel* imgLabel = new QLabel(levels[i]);
        const int widthBorder=15,heightBorder=10;
        imgLabel->setGeometry(widthBorder, heightBorder, levelButtonWidth-widthBorder*2, levelButtonHeight-heightBorder*2);
        imgLabel->setStyleSheet(QString(
            "QLabel {"
            "    border: none;"
            "    border-image: url(%1) 0 0 0 0 stretch stretch;" // 在这里拉伸，但大小被限制在了 80x80
            "}"
            ).arg(templateURL));
        imgLabel->setAttribute(Qt::WA_TransparentForMouseEvents);//允许鼠标事件穿透 QLabel
        // 2. 创建一个 Label 叠在上面（负责第二层：中图，支持自动拉伸）
        QLabel* borderLabel = new QLabel(levels[i]);
        borderLabel->setGeometry(0, 0, levelButtonWidth, levelButtonHeight);
        borderLabel->setStyleSheet(QString("QLabel{border-image:url(%1);}").arg(loadAsset("images/icons/level.png")));
        borderLabel->setAttribute(Qt::WA_TransparentForMouseEvents); // 点击穿透，不挡住按钮点击

        // 3. 创建一个 Label 叠在最上面（负责第三层：文字）
        QLabel* textLabel = new QLabel(QString::number(levelNum), levels[i]);
        textLabel->setGeometry(0, 0, levelButtonWidth, levelButtonHeight);
        textLabel->setAlignment(Qt::AlignCenter); // 绝对居中
        textLabel->setStyleSheet("QLabel{color: white; font-size: 30px; font-weight: bold;}");
        textLabel->setAttribute(Qt::WA_TransparentForMouseEvents); // 点击穿透
        //统一绑定点击信号
        connect(levels[i], &QPushButton::clicked, this, &LevelChoosePage::onStartButtonClicked);
        levels[i]->show();
    }
    //加上连接管子
    QPoint tubePositions[] = {//精准测量的关卡按钮坐标 (X, Y)
        QPoint((+levelButtonWidth+pageWidth)/2-levelButtonWidth-widthGap-12,  topMargin+levelButtonHeight/2-12),
        QPoint((+levelButtonWidth+pageWidth)/2-levelButtonWidth-widthGap-12,  topMargin+levelButtonHeight/2+levelButtonHeight+heightGap-12),
        QPoint((+levelButtonWidth+pageWidth)/2-levelButtonWidth-widthGap-12,  topMargin+levelButtonHeight/2+levelButtonHeight*2+heightGap*2-12),
        QPoint((+levelButtonWidth+pageWidth)/2-12,      topMargin+levelButtonHeight/2-12),
        QPoint((+levelButtonWidth+pageWidth)/2-12,      topMargin+levelButtonHeight/2+levelButtonHeight+heightGap-12),
        QPoint((+levelButtonWidth+pageWidth)/2-12,      topMargin+levelButtonHeight/2+levelButtonHeight*2+heightGap*2-12)
    };
    for(int i=0;i<6;i++)
    {
        QLabel* tubeLabel = new QLabel(this);
        tubeLabel->move(tubePositions[i]);
        tubeLabel->setFixedSize(widthGap+24, 30);
        tubeLabel->setStyleSheet(QString("QLabel{border:none;border-image:url(%1);}").arg(loadAsset("images/icons/pipe1.png")));
        tubeLabel->show();
    }
    QLabel* tubeLabel = new QLabel(this);
    tubeLabel->move(QPoint((+levelButtonWidth+pageWidth)/2+levelButtonWidth+widthGap-10,topMargin+levelButtonHeight/2-12));
    tubeLabel->setFixedSize(60, levelButtonHeight+heightGap+24);
    tubeLabel->setStyleSheet(QString("QLabel{border:none;border-image:url(%1);}").arg(loadAsset("images/icons/pipeC.png")));
    tubeLabel->show();
    tubeLabel = new QLabel(this);
    tubeLabel->move(QPoint((-levelButtonWidth+pageWidth)/2-levelButtonWidth-widthGap-50,topMargin+levelButtonHeight/2+levelButtonHeight+heightGap-12));
    tubeLabel->setFixedSize(60, levelButtonHeight+heightGap+24);
    tubeLabel->setStyleSheet(QString("QLabel{border:none;border-image:url(%1);}").arg(loadAsset("images/icons/pipeCr.png")));
    tubeLabel->show();
    // 加一个左上角的返回按钮
    QPushButton* backBtn = new QPushButton(this);
    backBtn->setGeometry(15, 15, 80, 80); // 根据左上角黄色箭头的尺寸
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(QString("QPushButton { border: none; border-image: url(%1) 0 0 0 0 stretch stretch; }").arg(loadAsset("images/icons/return.png")));
    connect(backBtn, &QPushButton::clicked, this, &LevelChoosePage::close);
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
void LevelChoosePage::saveProcess()
{
    if(!writeLevelSave(unlockedLevel))
    {
        message::otherError("进度文件保存失败");
        return;
    }
}
void LevelChoosePage::upgradeLevelUnlocked(int levelNumber)
{
    int levelNow=loadLevel();
    if(levelNumber>levelNow)writeLevelSave(levelNumber);
}
