#include "MainWindow.h"
#include"levelchoosepage.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QVBoxLayout>
#include"../message/Message.h"

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
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    this->setFixedSize(1365, 768);

    // 2. 设置主窗口背景图
    QPixmap background(assetPath("images/background/background.png")); // 你的游戏背景图路径
    QPalette palette;
    palette.setBrush(QPalette::Window, background.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    this->setPalette(palette);

    // 3. 创建中央核心区域并应用垂直布局
    QWidget *centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // 4. 关键：添加顶部的“大弹簧”，把按钮往下推，腾出地方放 Logo
    layout->addStretch(4); // 数字代表比例，可以根据 Logo 的高度微调

    // 5. 创建“关卡模式”按钮并美化
    levelBtn = new QPushButton(this);
    levelBtn->setFixedSize(360, 90); // 严格匹配你按钮切片的宽高
    levelBtn->setCursor(Qt::PointingHandCursor); // 鼠标移上去变成小手
    levelBtn->setStyleSheet(
        "QPushButton {"
        "    border: none;" // 去掉默认边框
        "    background: transparent;" // 正常状态贴图
        "}"
        );
    levelBtn->setIcon(QIcon(assetPath("images/bars/levelmode.png")));
    levelBtn->setIconSize(levelBtn->size());
    layout->addWidget(levelBtn, 0, Qt::AlignCenter); // 强制按钮在水平方向居中

    // 6. 稍微留一点两个按钮之间的间距
    layout->addSpacing(20);

    // 7. 创建“沙盒模式”按钮并美化
    startBtn = new QPushButton(this);
    startBtn->setFixedSize(360, 90);
    startBtn->setCursor(Qt::PointingHandCursor);
    startBtn->setStyleSheet(
        "QPushButton {"
        "    border: none;"
        "    background: transparent;"
        "}"
        );
    startBtn->setIcon(QIcon(assetPath("images/bars/sandboxmode.png")));
    startBtn->setIconSize(startBtn->size());
    layout->addWidget(startBtn, 0, Qt::AlignCenter); // 强制垂直居中

    // 8. 底部再加一个弹簧，顶住按钮，配合顶部的弹簧把按钮集群锁在黄金上下位置
    layout->addStretch(3);

    // 9. 连接信号
    connect(levelBtn, &QPushButton::clicked, this, &MainWindow::onLevelButtonClicked);
    connect(startBtn, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
}
void MainWindow::onLevelButtonClicked()
{
    if(levelChoosePage==nullptr)
    {
        levelChoosePage=new LevelChoosePage();
        connect(levelChoosePage,&LevelChoosePage::pageClosed,this,&MainWindow::onChooseLevelPageClosed);
    }
    levelChoosePage->loadProcess();
    this->hide();
    levelChoosePage->show();
    //this->show();
}
void MainWindow::onChooseLevelPageClosed()
{
    this->show();
}
