#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include<QMainWindow>
#include<QGraphicsScene>
#include<QGraphicsView>
#include<QPointer>
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
    QPointer<QGraphicsView> menuView=nullptr;
    QPointer<QGraphicsScene> menuScene=nullptr;
    QPointer<AppGraphicsView> view=nullptr;
    QPointer<QGraphicsScene> scene=nullptr;
    QPointer<LevelChoosePage> levelChoosePage=nullptr;
};
#endif // MAINWINDOW_H
