#ifndef APPGRAPHICSVIEW_H
#define APPGRAPHICSVIEW_H
#include<QGraphicsView>
class AppGraphicsView:public QGraphicsView{
public:
    std::function<void()> onClosed;
    AppGraphicsView(QGraphicsScene* scene);
protected:
    void closeEvent(QCloseEvent *event) override {
        if(onClosed)onClosed();
        QGraphicsView::closeEvent(event);
    }
    void mousePressEvent(QMouseEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
};
#endif // APPGRAPHICSVIEW_H
