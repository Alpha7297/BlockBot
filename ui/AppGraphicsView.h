#ifndef APPGRAPHICSVIEW_H
#define APPGRAPHICSVIEW_H
#include<QGraphicsView>
#include<QDir>
#include<QCoreApplication>
class AppGraphicsView:public QGraphicsView{
public:
    std::function<void()> onClosed;
    AppGraphicsView(QGraphicsScene* scene);
protected:
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
};
inline QString loadAsset(const QString& fileName){
    QStringList roots;
    roots<<QDir::currentPath()
          <<QCoreApplication::applicationDirPath()
          <<QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("..")
          <<QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../..");
    for(const QString& root:roots){
        QString path=QDir(root).filePath(fileName);
        if(QFileInfo::exists(path)){
            path.replace("\\", "/");
            return path;
        }
    }
    return QString();
}
#endif // APPGRAPHICSVIEW_H
