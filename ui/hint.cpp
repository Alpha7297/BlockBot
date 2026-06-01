#include "hint.h"
#include "../ui/AppGraphicsView.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QPen>
#include <QPixmap>
#include <QTextDocument>
#include <Qt>

LevelHintPanel::LevelHintPanel(QGraphicsItem* parent):QGraphicsRectItem(parent){
    setRect(0,0,800,466);
    setPen(Qt::NoPen);
    setAcceptedMouseButtons(Qt::LeftButton);

    background=new QGraphicsPixmapItem(this);
    QPixmap pixmap(loadAsset("images/bars/hint_panel.png"));
    if(!pixmap.isNull()){
        background->setPixmap(pixmap.scaled(800,466,Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
    }
    background->setPos(0,0);
    background->setZValue(-1);

    contentText=new QGraphicsTextItem(this);
    contentText->document()->setDocumentMargin(0);
    contentText->setDefaultTextColor(Qt::white);
    contentText->setFont(QFont(QStringLiteral("Microsoft YaHei"),20));
    contentText->setTextWidth(700);
    contentText->setPos(50,42);
    contentText->setAcceptedMouseButtons(Qt::NoButton);
}

void LevelHintPanel::setHintText(const QString& text){
    contentText->setPlainText(text);
}

void LevelHintPanel::mousePressEvent(QGraphicsSceneMouseEvent* event){
    if(event->button()==Qt::LeftButton){
        setVisible(false);
        event->accept();
    }
}

QString chineseLevelTitle(int levelNumber){
    static const QString titles[]={
        QString::fromUtf8("第一关"),
        QString::fromUtf8("第二关"),
        QString::fromUtf8("第三关"),
        QString::fromUtf8("第四关"),
        QString::fromUtf8("第五关"),
        QString::fromUtf8("第六关"),
        QString::fromUtf8("第七关"),
        QString::fromUtf8("第八关"),
        QString::fromUtf8("第九关")
    };
    if(levelNumber<1||levelNumber>9){
        return QString();
    }
    return titles[levelNumber-1];
}

QString hintTextForLevel(int levelNumber){
    static const QString hints[]={
        QString::fromUtf8(
            "你需要通过控制台，控制R-07从起点移动到终点\n"
            "在档案1学习如何控制 R-07"
        ),
        QString::fromUtf8(
            "地刺会周期性出现，你需要观察规律，控制R-07从起点到达终点\n"
            "在档案2学习如何观察关卡"
        ),
        QString::fromUtf8(
            "红灯时地刺升起，无法通行\n"
            "你需要等待绿灯，地刺降下时通过\n\n"
            "在档案3学习条件语句与前方能否通行"
        ),
        QString::fromUtf8(
            "四个侦察机器人每帧都会顺时针转动\n你需要通过控制台让R-07到达终点，不被侦察机器人发现\n\n"
            "档案4学习运算与变量\n"
            "注意机器人的侦测器无法分辨前方是激光还是可通行地面"
        ),
        QString::fromUtf8(
            "这一关你需要控制 R-07 走过迷宫到达终点\n\n"
            "档案5学习列表与存档\n"
            "档案6学习自定义控制方式\n"
            "在档案7学习哈希和取余"
        ),
        QString::fromUtf8(
            "这一关你需要对按照时间先后顺序对坐标进行排序，将结果保存到有效结果列表\n"
            "当时间相同时，坐标小的在前面\n"
            "注意过滤坐标<0或时间<0的无效信息\n"
            "例如\n"
            "n=4，时间列表=[7,3,-1,12]\n位置列表=[41,284,10,-1]\n"
            "第1项(7,41)和第2项(3,284)有效\n"
            "第3项时间=-1无效，第4项位置=-1无效，均跳过\n"
            "按时间从小到大排序后，有效结果为[284,41]\n"
            "在档案8学习输出与排序"
        ),
        QString::fromUtf8(
            "这一关你需要破解密码，加密方式是\n"
            "首先倒序，之后每个数字加3，数字10变成0，最后整体循环向右移动2位。\n"
            "例如：\n"
            "923456 -> 654329 -> 987652 -> 529876\n"
            "在档案9学习字符串"
        ),
        QString::fromUtf8(
            "冷却液从上方流下，管道会积累冷却液，积累的冷却液高度即为该点的压力\n"
            "当该点高度超过压力上限时管道爆裂，请找出这些点加入危险点列表，不必考虑先后顺序\n"
            "例如\n"
            "格子高度为4 3 4 1 3，压力上限为 0 0 0 1 1。\n"
            "格子压力为0 1 0 2 0"
            "最终危险位置为 2 和 4。"
        ),
        QString::fromUtf8(
            "请你综合考虑三个不同的目标，争取同时完成"
        )
    };
    if(levelNumber<1||levelNumber>9){
        return QString::fromUtf8("暂无提示。");
    }
    return hints[levelNumber-1];
}
