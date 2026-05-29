#include "levelchoosepage.h"
#include "fstream"
#include"../message/Message.h"
#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QFileInfo>

namespace{
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
    return QDir::fromNativeSeparators(QDir::current().absoluteFilePath(relativePath));
}

QString styleUrl(const QString& relativePath){
    return "url(\"" + assetPath(relativePath) + "\")";
}
}

void LevelChoosePage::init()
{
    qDeleteAll(findChildren<QPushButton*>(QString(),Qt::FindDirectChildrenOnly));

    // 1. 固定窗口大小，铺上你的机械工厂背景图
    this->setFixedSize(1280, 720); // 假设背景图是这个尺寸
    this->setStyleSheet("QDialog { background-image: " + styleUrl("images/background/background.png") + "; }");


    // 2. 精准测量的关卡按钮坐标 (X, Y) —— 匹配你背景图上正方形框的位置
    // 1~5 从左往右，6~9 从右往左（蛇形）
    QPoint levelPositions[] = {
        QPoint(50,  280),  // 关卡 1
        QPoint(240, 295),  // 关卡 2
        QPoint(410, 295),  // 关卡 3
        QPoint(580, 295),  // 关卡 4
        QPoint(750, 295),  // 关卡 5
        QPoint(690, 580),  // 关卡 6 (注意：下面这一排往左走)
        QPoint(470, 580),  // 关卡 7
        QPoint(270, 580),  // 关卡 8
        QPoint(50,  580)   // 关卡 9
    };
    int totalLevels = 9; // 根据你图片中显示，共有9关
    std::vector<QPushButton*>levels;
    levels.resize(totalLevels);

    // 3. 批量生成并配置按钮
    for (int i = 0; i < totalLevels; ++i) {
        int levelNum = i + 1; // 关卡编号从 1 开始

        levels[i] = new QPushButton(this);
        levels[i]->setProperty("levelNumber", levelNum);
        levels[i]->setCursor(Qt::PointingHandCursor);
        levels[i]->setFixedSize(145, 145); // 普通灰色金属框大小

        // 搬移到背景图对应的管道口位置
        levels[i]->move(levelPositions[i]);

        // 4. ✨ 核心核心：根据 unlockedLevel 判断关卡状态并上色 ✨
        QString qss;
        if (levelNum < unlockedLevel) {
            // 【状态 A：已通关】显示正常缩略图
            qss = QString(
                      "QPushButton {"
                      "    border: none;"
                      "    background-image: " + styleUrl("images/background/level_%1_normal.png") + ";"
                      "}"
                      ).arg("");//levelNum);
        }
        else if (levelNum == unlockedLevel) {
            // 【状态 B：当前正在挑战的关卡】显示高亮红框（或者根据图片，给按钮套上亮色皮肤）
            qss = QString(
                      "QPushButton {"
                      "    border: none;"
                      "    background-image: " + styleUrl("images/background/level_%1_current.png") + ";" // 或者是带红色呼吸灯效果的图
                      "}"
                      ).arg("");//levelNum);
        }
        else {
            // 【状态 C：未解锁】置灰，并且在右下角带一把锁
            // 巧妙利用 QSS 的多背景叠加技术：底层是置灰的缩略图，顶层是一把锁的 PNG
            qss = QString(
                      "QPushButton {"
                      "    border: none;"
                      "    background-image: " + styleUrl("images/background/level_%1_gray.png") + ";"
                      "}"
                      ).arg("");//(levelNum);

            // 未解锁的按钮，禁用点击事件
            levels[i]->setEnabled(false);
        }

        levels[i]->setStyleSheet(qss);

        // 5. 统一绑定点击信号
        connect(levels[i], &QPushButton::clicked, this, &LevelChoosePage::onStartButtonClicked);
        levels[i]->show();
    }

    // 6. 顺手加一个左上角的返回按钮
    QPushButton* backBtn = new QPushButton(this);
    backBtn->setGeometry(15, 15, 80, 80); // 根据左上角黄色箭头的尺寸
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet("QPushButton { border: none; background-image: " + styleUrl("images/icons/return.png") + "; }");
    connect(backBtn, &QPushButton::clicked, this, &LevelChoosePage::close);
}
LevelChoosePage::LevelChoosePage(QWidget *parent ):QDialog(parent)
{
    //init();
}
void LevelChoosePage::loadProcess()
{
    std::ifstream inFile(":/process");
    if(!inFile.is_open())
    {
        //message::otherError("进度文件损坏");
        unlockedLevel=1;
    }
    else inFile>>unlockedLevel;
    inFile.close();
    init();
}
void LevelChoosePage::saveProcess()
{
    std::ofstream outFile(":/process");
    if(!outFile.is_open())
    {
        message::otherError("进度文件无法打开");
        return;
    }
    outFile<<unlockedLevel;
    outFile.close();
}
