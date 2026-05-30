#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include<QMainWindow>
#include<QPushButton>
#include<QGraphicsScene>
#include "AppGraphicsView.h"
#include "LevelChoosePage.h"
#include "ArchivePage.h"
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
    void onArchiveButtonClicked();
    void onChooseLevelPageClosed();
    void onArchivePageClosed();
private:
    QPushButton *startBtn;
    QPushButton *levelBtn;
    QPushButton *archiveBtn;
    AppGraphicsView *view=nullptr;
    QGraphicsScene *scene=nullptr;
    LevelChoosePage *levelChoosePage=nullptr;
    ArchivePage *archivePage=nullptr;
};
#endif // MAINWINDOW_H
