#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include<QMainWindow>
#include<QPushButton>
#include<QGraphicsScene>
#include "AppGraphicsView.h"
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
private:
    QPushButton *startBtn;
    AppGraphicsView *view;
    QGraphicsScene *scene;
};
#endif // MAINWINDOW_H
