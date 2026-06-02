#ifndef APPGRAPHICSVIEW_H
#define APPGRAPHICSVIEW_H
#include<QGraphicsView>
#include<QDir>
#include<QCoreApplication>
#include<QFileInfo>
#include<QStringList>
class AppGraphicsView:public QGraphicsView{
public:
    std::function<void()> onClosed;
    AppGraphicsView(QGraphicsScene* scene,QWidget* parent=nullptr);
protected:
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
};
inline QString loadAsset(const QString& fileName){
    QString resourcePath=fileName;
    resourcePath.replace("\\","/");
    if(resourcePath.startsWith(":/")){
        return resourcePath;
    }
    QString qrcPath=QString(":/%1").arg(resourcePath);
    if(QFileInfo::exists(qrcPath)){
        return qrcPath;
    }
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
    return fileName;
}
#endif // APPGRAPHICSVIEW_H
