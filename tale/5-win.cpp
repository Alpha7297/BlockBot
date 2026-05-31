#include "TaleWindow.h"
#include "../ui/AppGraphicsView.h"

#include <QApplication>
#include <QBrush>
#include <QFont>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QDebug>
#include <QTextDocument>
#include <QTimer>

#include <map>
#include <vector>

namespace tale{
namespace{

constexpr int TaleWidth=1200;
constexpr int TaleHeight=800;
constexpr qreal PortraitTop=105;
constexpr int PortraitHeight=420;
constexpr qreal TextBoxX=110;
constexpr qreal TextBoxY=550;
constexpr qreal TextBoxWidth=980;
constexpr qreal TextBoxHeight=150;
constexpr qreal TextPadding=28;
constexpr qreal SpeakerNameBoxWidth=180;
constexpr qreal SpeakerNameBoxHeight=42;
constexpr qreal SpeakerNamePadding=14;
constexpr qreal SpeakingCharacterOpacity=1.0;
constexpr qreal ListeningCharacterOpacity=0.55;
constexpr qreal AutoCharacterOpacity=-1.0;
constexpr int TaleTextCharIntervalMs=45;

const QString RobotSpeakerName=QString::fromUtf8("R-07");
const QString MotherSpeakerName=QString::fromUtf8("Mother");
const QString EchoSpeakerName=QString::fromUtf8("Echo");
const QString OldRobotSpeakerName=QString::fromUtf8("Old Robot");
const QString SystemSpeakerName=QString::fromUtf8("System");
const QString RecordSpeakerName=QString::fromUtf8("Record");
const QString ChirpSpeakerName=QString::fromUtf8("Chirp");
const QString EmptySpeakerName=QString::fromUtf8("");

enum class TaleCharacter{
    Angry,
    Chirp,
    Curl,
    Curious,
    Damage,
    Determined,
    Echo,
    Glad,
    Mother,
    OldRobot,
    Puzzled,
    Questioning,
    Quiver,
    Record,
    Robot,
    Sad,
    Surprise,
    Tired
};

struct TaleCharacterDefinition{
    TaleCharacter character;
    QString assetPath;
    QString displayName;
};

struct TaleCharacterOnScene{
    TaleCharacter character;
    QPointF position;
    qreal opacity=AutoCharacterOpacity;
};

struct TaleParagraph{
    QString speakerName;
    QString content;
    std::vector<TaleCharacterOnScene> characters;
};

const std::vector<TaleCharacterDefinition> PaintingCharacters={
    {TaleCharacter::Angry,QString::fromUtf8("images/painting/angry.png"),RobotSpeakerName},
    {TaleCharacter::Chirp,QString::fromUtf8("images/painting/chirp.png"),ChirpSpeakerName},
    {TaleCharacter::Curl,QString::fromUtf8("images/painting/curl.png"),RobotSpeakerName},
    {TaleCharacter::Curious,QString::fromUtf8("images/painting/curious.png"),RobotSpeakerName},
    {TaleCharacter::Damage,QString::fromUtf8("images/painting/damage.png"),RobotSpeakerName},
    {TaleCharacter::Determined,QString::fromUtf8("images/painting/determined.png"),RobotSpeakerName},
    {TaleCharacter::Echo,QString::fromUtf8("images/painting/echo.png"),EchoSpeakerName},
    {TaleCharacter::Glad,QString::fromUtf8("images/painting/glad.png"),RobotSpeakerName},
    {TaleCharacter::Mother,QString::fromUtf8("images/painting/mother.png"),MotherSpeakerName},
    {TaleCharacter::OldRobot,QString::fromUtf8("images/painting/old_robot.png"),OldRobotSpeakerName},
    {TaleCharacter::Puzzled,QString::fromUtf8("images/painting/puzzled.png"),RobotSpeakerName},
    {TaleCharacter::Questioning,QString::fromUtf8("images/painting/questioning.png"),RobotSpeakerName},
    {TaleCharacter::Quiver,QString::fromUtf8("images/painting/quiver.png"),RobotSpeakerName},
    {TaleCharacter::Record,QString::fromUtf8("images/painting/record.png"),RecordSpeakerName},
    {TaleCharacter::Robot,QString::fromUtf8("images/painting/robot.png"),RobotSpeakerName},
    {TaleCharacter::Sad,QString::fromUtf8("images/painting/sad.png"),RobotSpeakerName},
    {TaleCharacter::Surprise,QString::fromUtf8("images/painting/surprise.png"),RobotSpeakerName},
    {TaleCharacter::Tired,QString::fromUtf8("images/painting/tired.png"),RobotSpeakerName}
};

std::vector<TaleParagraph> buildLevelOneStory(){
    std::vector<TaleParagraph> story;

    story.push_back({
        SystemSpeakerName,
        QString::fromUtf8("地下迷宫通过。数据中继室入口开放。"),
        {{TaleCharacter::Glad,QPointF(460,180)}}
    });

    story.push_back({
        RobotSpeakerName,
        QString::fromUtf8("已到达出口信号点。继续前往数据中继室。"),
        {{TaleCharacter::Determined,QPointF(460,180)}}
    });
    return story;
}

class DraggableCharacterLayer:public QGraphicsPixmapItem{
public:
    DraggableCharacterLayer(const QString& name,const QPixmap& pixmap):
        QGraphicsPixmapItem(pixmap),characterName(name){
        setAcceptedMouseButtons(Qt::LeftButton);
        setCursor(Qt::OpenHandCursor);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override{
        dragOffset=event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override{
        setPos(event->scenePos()-dragOffset);
        event->accept();
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override{
        setCursor(Qt::OpenHandCursor);
        qDebug()<<characterName<<"position:"<<pos();
        event->accept();
    }

private:
    QString characterName;
    QPointF dragOffset;
};

class TaleView:public QGraphicsView{
public:
    TaleView():story(buildLevelOneStory()){
        setFixedSize(TaleWidth,TaleHeight);
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setRenderHint(QPainter::Antialiasing,true);
        setScene(&scene);
        scene.setSceneRect(0,0,TaleWidth,TaleHeight);
        drawScene();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override{
        QGraphicsView::mousePressEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override{
        if(event->key()==Qt::Key_Space){
            advanceText();
            event->accept();
            return;
        }
        QGraphicsView::keyPressEvent(event);
    }

private:
    QGraphicsScene scene;
    std::vector<TaleParagraph> story;
    std::map<TaleCharacter,QGraphicsPixmapItem*> characterLayers;
    QGraphicsRectItem* textBox=nullptr;
    QGraphicsRectItem* speakerNameBox=nullptr;
    QGraphicsTextItem* speakerNameItem=nullptr;
    QGraphicsTextItem* textItem=nullptr;
    QTimer textTimer;
    int paragraphIndex=0;
    int visibleTextLength=0;

    void drawScene(){
        QPixmap background(loadAsset("images/background/level1.png"));
        if(!background.isNull()){
            scene.addPixmap(background.scaled(TaleWidth,TaleHeight,Qt::IgnoreAspectRatio,
                Qt::SmoothTransformation));
        }
        else{
            scene.setBackgroundBrush(QColor(30,34,40));
        }

        createCharacterLayers();

        textBox=scene.addRect(TextBoxX,TextBoxY,TextBoxWidth,TextBoxHeight,
            QPen(QColor(235,238,242),2),
            QBrush(QColor(24,28,34,220)));
        textBox->setZValue(5);

        speakerNameBox=scene.addRect(TextBoxX,TextBoxY-SpeakerNameBoxHeight,
            SpeakerNameBoxWidth,SpeakerNameBoxHeight,
            QPen(QColor(235,238,242),2),
            QBrush(QColor(24,28,34,235)));
        speakerNameBox->setZValue(6);

        speakerNameItem=scene.addText(QString(),QFont("Microsoft YaHei",18,QFont::Bold));
        speakerNameItem->document()->setDocumentMargin(0);
        speakerNameItem->setDefaultTextColor(Qt::white);
        speakerNameItem->setTextWidth(SpeakerNameBoxWidth-SpeakerNamePadding*2);
        speakerNameItem->setPos(TextBoxX+SpeakerNamePadding,
            TextBoxY-SpeakerNameBoxHeight+8);
        speakerNameItem->setZValue(7);
        speakerNameItem->setAcceptedMouseButtons(Qt::NoButton);

        textItem=scene.addText(QString(),QFont("Microsoft YaHei",22,QFont::Bold));
        textItem->document()->setDocumentMargin(0);
        textItem->setDefaultTextColor(Qt::white);
        textItem->setTextWidth(TextBoxWidth-TextPadding*2);
        textItem->setPos(TextBoxX+TextPadding,TextBoxY+TextPadding);
        textItem->setZValue(6);
        textItem->setAcceptedMouseButtons(Qt::NoButton);

        QObject::connect(&textTimer,&QTimer::timeout,this,[this](){
            showNextCharacter();
        });
        startParagraph();
    }

    void createCharacterLayers(){
        qreal zValue=2;
        for(const TaleCharacterDefinition& definition:PaintingCharacters){
            QPixmap pixmap(loadAsset(definition.assetPath));
            if(pixmap.isNull()){
                continue;
            }
            QPixmap scaledPixmap=pixmap.scaledToHeight(PortraitHeight,Qt::SmoothTransformation);
            QGraphicsPixmapItem* layer=new DraggableCharacterLayer(definition.displayName,scaledPixmap);
            scene.addItem(layer);
            layer->setZValue(zValue);
            layer->setVisible(false);
            characterLayers[definition.character]=layer;
            zValue+=1;
        }
    }

    void advanceText(){
        if(story.empty()){
            return;
        }
        if(isTextTyping()){
            showFullText();
            return;
        }
        paragraphIndex=(paragraphIndex+1)%static_cast<int>(story.size());
        startParagraph();
    }

    const TaleParagraph& currentParagraph() const{
        return story[paragraphIndex];
    }

    bool isTextTyping() const{
        if(story.empty()){
            return false;
        }
        return visibleTextLength<currentParagraph().content.size();
    }

    void startParagraph(){
        if(textItem==nullptr||story.empty()){
            return;
        }
        updateCharacterLayers();
        updateSpeakerName();
        visibleTextLength=0;
        textItem->setPlainText(QString());
        textTimer.start(TaleTextCharIntervalMs);
    }

    void showNextCharacter(){
        if(textItem==nullptr||story.empty()){
            textTimer.stop();
            return;
        }
        const QString& text=currentParagraph().content;
        if(visibleTextLength>=text.size()){
            textTimer.stop();
            return;
        }
        visibleTextLength++;
        textItem->setPlainText(text.left(visibleTextLength));
        if(visibleTextLength>=text.size()){
            textTimer.stop();
        }
    }

    void showFullText(){
        if(textItem==nullptr||story.empty()){
            return;
        }
        textTimer.stop();
        visibleTextLength=currentParagraph().content.size();
        textItem->setPlainText(currentParagraph().content);
    }

    QGraphicsPixmapItem* layerForCharacter(TaleCharacter character) const{
        auto it=characterLayers.find(character);
        return it==characterLayers.end()?nullptr:it->second;
    }

    QString displayNameForCharacter(TaleCharacter character) const{
        for(const TaleCharacterDefinition& definition:PaintingCharacters){
            if(definition.character==character){
                return definition.displayName;
            }
        }
        return QString();
    }

    qreal opacityForCharacter(const TaleCharacterOnScene& characterOnScene) const{
        if(characterOnScene.opacity>=0.0){
            return characterOnScene.opacity;
        }
        return displayNameForCharacter(characterOnScene.character)==currentParagraph().speakerName?
            SpeakingCharacterOpacity:
            ListeningCharacterOpacity;
    }

    void updateCharacterLayers(){
        for(const auto& item:characterLayers){
            if(item.second!=nullptr){
                item.second->setVisible(false);
            }
        }
        for(const TaleCharacterOnScene& characterOnScene:currentParagraph().characters){
            QGraphicsPixmapItem* layer=layerForCharacter(characterOnScene.character);
            if(layer==nullptr){
                continue;
            }
            layer->setPos(characterOnScene.position);
            layer->setOpacity(opacityForCharacter(characterOnScene));
            layer->setVisible(true);
        }
    }

    void updateSpeakerName(){
        QString speakerName=currentParagraph().speakerName;
        if(speakerNameItem!=nullptr){
            speakerNameItem->setPlainText(speakerName);
        }
        bool hasSpeaker=!speakerName.isEmpty();
        if(speakerNameBox!=nullptr){
            speakerNameBox->setVisible(hasSpeaker);
        }
        if(speakerNameItem!=nullptr){
            speakerNameItem->setVisible(hasSpeaker);
        }
    }
};

}

int runTale(int argc,char* argv[]){
    QApplication app(argc,argv);
    TaleView view;
    view.show();
    return app.exec();
}

}

