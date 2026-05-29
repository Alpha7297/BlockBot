#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include<QMainWindow>
#include<QPushButton>
#include<QGraphicsScene>
#include "AppGraphicsView.h"
#include"levelchoosepage.h"
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override
    {
        delete view;
    }
private slots:
    void onStartButtonClicked();
    void onLevelButtonClicked();
    void onChooseLevelPageClosed();
private:
    QPushButton *startBtn;
    QPushButton *levelBtn;
    AppGraphicsView *view=nullptr;
    QGraphicsScene *scene=nullptr;
    LevelChoosePage *levelChoosePage=nullptr;
};
#endif // MAINWINDOW_H