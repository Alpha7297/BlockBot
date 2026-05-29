#ifndef LEVELCHOOSEPAGE_H
#define LEVELCHOOSEPAGE_H

#include<QDialog>
#include<QPushButton>
#include<QGraphicsScene>
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
    AppGraphicsView *view=nullptr;
    QGraphicsScene *scene=nullptr;
protected:
    void closeEvent(QCloseEvent *event)override
    {
        emit pageClosed();
        QDialog::closeEvent(event);
    }
};

#endif // LEVELCHOOSEPAGE_H
