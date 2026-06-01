#ifndef UI_HINT_H
#define UI_HINT_H

#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QString>

class LevelHintPanel:public QGraphicsRectItem{
public:
    QGraphicsPixmapItem* background;
    QGraphicsTextItem* contentText;

    LevelHintPanel(QGraphicsItem* parent=nullptr);
    void setHintText(const QString& text);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
};

QString chineseLevelTitle(int levelNumber);
QString hintTextForLevel(int levelNumber);

#endif
