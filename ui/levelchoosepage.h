#ifndef LEVELCHOOSEPAGE_H
#define LEVELCHOOSEPAGE_H

#include<QDialog>
#include<QGraphicsScene>
#include<QGraphicsView>
#include<QPointer>
#include "AppGraphicsView.h"
class LevelChoosePage : public QDialog
{
    Q_OBJECT
public:
    explicit LevelChoosePage(QWidget *parent = nullptr);
    ~LevelChoosePage() override
    {
        delete view;
    }
    void loadProcess();
    void saveProcess();
    void init();
    void startLevel(int levelNumber);
    int unlockedLevel=0;
signals:
    void pageClosed();
private slots:
    void onStartButtonClicked();
private:
    QPointer<QGraphicsView> chooseView=nullptr;
    QPointer<QGraphicsScene> chooseScene=nullptr;
    QPointer<AppGraphicsView> view=nullptr;
    QPointer<QGraphicsScene> scene=nullptr;
protected:
    void closeEvent(QCloseEvent *event)override
    {
        emit pageClosed();
        QDialog::closeEvent(event);
    }
};

#endif // LEVELCHOOSEPAGE_H
