#ifndef SETTINGS_SETTINGSDIALOG_H
#define SETTINGS_SETTINGSDIALOG_H

#include <QDialog>

class QGraphicsScene;
class QGraphicsView;

namespace settings{

class SettingsDialog:public QDialog{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent=nullptr);

private:
    QGraphicsScene* scene=nullptr;
    QGraphicsView* view=nullptr;
};

}

#endif
