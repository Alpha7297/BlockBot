#include "MainWindow.h"
#include "LevelChoosePage.h"
#include <QVBoxLayout>
#include <QLabel>
#include"../message/Message.h"
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    this->setFixedSize(1200, 800);
    QPixmap background(loadAsset("images/background/background.png")); // 你的游戏背景图路径
    QPalette palette;
    palette.setBrush(QPalette::Window, background.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    this->setPalette(palette);
    QWidget *centralWidget = new QWidget(this);// 创建中央核心区域并应用垂直布局
    this->setCentralWidget(centralWidget);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    //添加顶部的“大弹簧”
    layout->addStretch(2); // 数字代表比例，可以根据 Logo 的高度微调
    QLabel *logo=new QLabel(this);
    logo->setFixedSize(480, 160);
    logo->setStyleSheet(QString(
        "QLabel {"
        "    border: none;" // 去掉默认边框
        "    border-image: url(%1) 0 0 0 0 stretch stretch;" // 正常状态贴图
        "    background-repeat: no-repeat;"
        "    background-position: center;"
        "}"
        ).arg(loadAsset("images/bars/logo.png")));
    layout->addWidget(logo, 0, Qt::AlignCenter); // 强制按钮在水平方向居中
    layout->addSpacing(40);// 稍微留一点logo和按钮的间距

    levelBtn = new QPushButton("关卡模式",this);//创建“关卡模式”按钮并美化
    levelBtn->setFixedSize(360, 90); // 严格匹配你按钮切片的宽高
    levelBtn->setCursor(Qt::PointingHandCursor); // 鼠标移上去变成小手
    levelBtn->setStyleSheet(QString(
        "QPushButton {"
        "    color: #FFFFFF;"                // 文字颜色（白色）
        "    font-family: 'Microsoft YaHei';" // 字体（微软雅黑）
        "    font-size: 18px;"               // 字体大小
        "    font-weight: bold;"             // 字体加粗
        "    border: none;" // 去掉默认边框
        "    border-image: url(%1) 0 0 0 0 stretch stretch;" // 正常状态贴图
        "    background-repeat: no-repeat;"
        "    background-position: center;"
        "}"
        ).arg(loadAsset("images/bars/levelmode.png")));
    layout->addWidget(levelBtn, 0, Qt::AlignCenter); // 强制按钮在水平方向居中
    layout->addSpacing(20);// 稍微留一点两个按钮之间的间距

    startBtn = new QPushButton("沙盒模式",this);    // 创建“沙盒模式”按钮并美化
    startBtn->setFixedSize(360, 90);
    startBtn->setCursor(Qt::PointingHandCursor);
    startBtn->setStyleSheet(QString(
        "QPushButton {"
        "    color: #FFFFFF;"                // 文字颜色（白色）
        "    font-family: 'Microsoft YaHei';" // 字体（微软雅黑）
        "    font-size: 18px;"               // 字体大小
        "    font-weight: bold;"             // 字体加粗
        "    border: none;"
        "    border-image: url(%1) 0 0 0 0 stretch stretch;"
        "    background-repeat: no-repeat;"
        "    background-position: center;"
        "}"
        ).arg(loadAsset("images/bars/sandboxmode.png")));
    layout->addWidget(startBtn, 0, Qt::AlignCenter); // 强制垂直居中
    layout->addSpacing(20);

    archiveBtn = new QPushButton(QString::fromUtf8("记忆档案"),this);
    archiveBtn->setFixedSize(360, 90);
    archiveBtn->setCursor(Qt::PointingHandCursor);
    archiveBtn->setStyleSheet(QString(
        "QPushButton {"
        "    color: #FFFFFF;"
        "    font-family: 'Microsoft YaHei';"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    border: none;"
        "    border-image: url(%1) 0 0 0 0 stretch stretch;"
        "    background-repeat: no-repeat;"
        "    background-position: center;"
        "}"
        ).arg(loadAsset("images/bars/archive.png")));
    layout->addWidget(archiveBtn, 0, Qt::AlignCenter);
    layout->addStretch(3);// 底部再加一个弹簧，顶住按钮，配合顶部的弹簧把按钮集群锁在黄金上下位置
    connect(levelBtn, &QPushButton::clicked, this, &MainWindow::onLevelButtonClicked);    // 9. 连接信号
    connect(startBtn, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(archiveBtn, &QPushButton::clicked, this, &MainWindow::onArchiveButtonClicked);
}
void MainWindow::onLevelButtonClicked()
{
    if(levelChoosePage==nullptr)
    {
        levelChoosePage=new LevelChoosePage(this);
        levelChoosePage->setWindowFlags(Qt::Widget);
        levelChoosePage->setGeometry(0,0,width(),height());
        connect(levelChoosePage,&LevelChoosePage::pageClosed,this,&MainWindow::onChooseLevelPageClosed);
    }
    levelChoosePage->loadProcess();
    if(centralWidget()!=nullptr){
        centralWidget()->hide();
    }
    levelChoosePage->show();
    levelChoosePage->raise();
}
void MainWindow::onChooseLevelPageClosed()
{
    if(centralWidget()!=nullptr){
        centralWidget()->show();
    }
}

void MainWindow::onArchiveButtonClicked()
{
    if(archivePage==nullptr)
    {
        archivePage=new ArchivePage(this);
        archivePage->setWindowFlags(Qt::Widget);
        archivePage->setGeometry(0,0,width(),height());
        connect(archivePage,&ArchivePage::pageClosed,this,&MainWindow::onArchivePageClosed);
    }
    archivePage->init();
    if(centralWidget()!=nullptr){
        centralWidget()->hide();
    }
    archivePage->show();
    archivePage->raise();
}

void MainWindow::onArchivePageClosed()
{
    if(centralWidget()!=nullptr){
        centralWidget()->show();
    }
}
