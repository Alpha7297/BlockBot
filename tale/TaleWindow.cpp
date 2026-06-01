#include "TaleWindow.h"
#include "../ui/AppGraphicsView.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QCloseEvent>
#include <QFont>
#include <QFrame>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QTextDocument>
#include <QTimer>

#include <map>
#include <functional>
#include <utility>
#include <vector>

namespace tale{
namespace{

constexpr int TaleWidth=1200;
constexpr int TaleHeight=800;
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
const QString EmptySpeakerName=QString::fromUtf8("");

enum class TaleCharacter{
    Angry,
    Closed,
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

struct TaleContent{
    std::vector<TaleParagraph> story;
    QString backgroundPath;
};

const std::vector<TaleCharacterDefinition> PaintingCharacters={
    {TaleCharacter::Angry,QString::fromUtf8("images/painting/angry.png"),RobotSpeakerName},
    {TaleCharacter::Closed,QString::fromUtf8("images/painting/close.png"),RobotSpeakerName},
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

TaleParagraph paragraph(const QString& speaker,const char* text,
    std::vector<TaleCharacterOnScene> characters){
    return {speaker,QString::fromUtf8(text),std::move(characters)};
}

std::vector<TaleCharacterOnScene> robotCenter(TaleCharacter character=TaleCharacter::Robot){
    return {{character,QPointF(460,180)}};
}

std::vector<TaleCharacterOnScene> robotEcho(TaleCharacter robot=TaleCharacter::Puzzled){
    return {{robot,QPointF(220,180)},{TaleCharacter::Echo,QPointF(700,130)}};
}

std::vector<TaleCharacterOnScene> robotEchoMother(TaleCharacter robot=TaleCharacter::Puzzled){
    return {{robot,QPointF(480,165)},{TaleCharacter::Echo,QPointF(682,130)},
        {TaleCharacter::Mother,QPointF(200,115)}};
}

TaleContent buildStartStory(){
    return {{
        paragraph(SystemSpeakerName,"备用电源接入。维修单元检测中。",
            {{TaleCharacter::Curl,QPointF(460,180)}}),
        paragraph(SystemSpeakerName,"编号：R-07。行动模块：受损。记录模块：缺失。残留指令：读取中。",
            {{TaleCharacter::Curl,QPointF(460,180)}}),
        paragraph(RecordSpeakerName,"离开这里，把最后的日志带出去。",
            {{TaleCharacter::Record,QPointF(210,150)},{TaleCharacter::Curl,QPointF(650,180)}}),
        paragraph(RobotSpeakerName,"指令确认。离开这里。带出日志。",
            {{TaleCharacter::Determined,QPointF(460,180)}}),
        paragraph(EmptySpeakerName,"R-07 的视觉传感器闪了一下。它醒在装配线末端的废料堆里，远处墙面上，“紧急出口”四个字在灰尘后微弱发光。",
            {{TaleCharacter::Curious,QPointF(460,180)}})
    },QString::fromUtf8("images/background/start.png")};
}

TaleContent buildLevel1StartStory(){
    return {{
        paragraph(RobotSpeakerName,"定位失败。履带阻力异常。基础行动模块可用。",
            robotCenter(TaleCharacter::Curl)),
        paragraph(EmptySpeakerName,"请控制 R-07 到达指定位置。",
            robotCenter(TaleCharacter::Puzzled)),
        paragraph(RobotSpeakerName,"目标确认：离开废料堆，恢复基础行动能力。",
            robotCenter(TaleCharacter::Determined)),
        paragraph(EmptySpeakerName,"排列基础移动指令，让 R-07 到达指定位置。",
            robotCenter(TaleCharacter::Robot)),
        paragraph(EmptySpeakerName,"控制台里的指令必须从“开始”接出，并以“结束”收尾。先观察 R-07 的朝向和目标位置，用“前进”“左转”“右转”排出一条固定路线。",
            robotCenter(TaleCharacter::Questioning)),
        paragraph(EmptySpeakerName,"不要急着使用复杂结构，本关重点是确认每一条指令都会按顺序执行。",
            robotCenter(TaleCharacter::Determined))
    },QString::fromUtf8("images/background/level1.png")};
}

TaleContent buildLevel1WinStory(){
    return {{
        paragraph(SystemSpeakerName,"行动模块恢复。供电接口接入成功。",
            robotCenter(TaleCharacter::Glad)),
        paragraph(EmptySpeakerName,"R-07 看着管理员日志，似乎想起了一段记忆……",
            {{TaleCharacter::Record,QPointF(210,150)},{TaleCharacter::Glad,QPointF(650,180)}}),
        paragraph(SystemSpeakerName,"管理员日志已归档。此后每完成一处区域，都会获得一张新的日志。",
            {{TaleCharacter::Record,QPointF(210,150)},{TaleCharacter::Curious,QPointF(650,180)}}),
        paragraph(EmptySpeakerName,"控制台已接入等待控制方式。",
            robotCenter(TaleCharacter::Glad)),
        paragraph(RobotSpeakerName,"视觉稳定。检测到远处出口标志。",
            robotCenter(TaleCharacter::Curious)),
        paragraph(EchoSpeakerName,"滋……如果你听得见，就不要只看出口标志。",
            robotEcho(TaleCharacter::Questioning)),
        paragraph(RobotSpeakerName,"我的指令是离开这里。",
            robotEcho(TaleCharacter::Questioning)),
        paragraph(EchoSpeakerName,"那就先学会怎么走。出口不会因为你看见它，就自己打开。",
            robotEcho(TaleCharacter::Determined))
    },QString::fromUtf8("images/background/level1.png")};
}

TaleContent buildLevel1LoseStory(){
    return {{
        paragraph(SystemSpeakerName,"路径错误。目标位置未到达。",
            robotCenter(TaleCharacter::Damage)),
        paragraph(RobotSpeakerName,"动作顺序不正确。需要重新规划移动。",
            robotCenter(TaleCharacter::Puzzled))
    },QString::fromUtf8("images/background/level1.png")};
}

TaleContent buildLevel2StartStory(){
    return {{
        paragraph(SystemSpeakerName,"检测到地刺陷阱。危险状态周期变化。",
            robotCenter(TaleCharacter::Quiver)),
        paragraph(EchoSpeakerName,"别看名字叫装配线，现在真正会伤到你的，是脚下。",
            robotEcho(TaleCharacter::Quiver)),
        paragraph(EchoSpeakerName,"地刺收回时可以前进。地刺冒出时必须等待。",
            robotEcho(TaleCharacter::Puzzled)),
        paragraph(RobotSpeakerName,"对。先观察，再行动。",
            robotEcho(TaleCharacter::Determined)),
        paragraph(EmptySpeakerName,"控制台的计时与等待方式开始恢复。使用 wait 观察地图状态，让 R-07 在安全窗口内通过地刺区域。",
            robotCenter(TaleCharacter::Questioning))
    },QString::fromUtf8("images/background/level2.png")};
}

TaleContent buildLevel2WinStory(){
    return {{
        paragraph(RobotSpeakerName,"通过危险区域。发现管理员日志。",
            robotCenter(TaleCharacter::Glad)),
        paragraph(SystemSpeakerName,"管理员日志读取中。",
            robotCenter(TaleCharacter::Glad)),
        paragraph(RecordSpeakerName,"封锁不是事故之后发生的，而是事故之前开始的。",
            {{TaleCharacter::Record,QPointF(210,150)},{TaleCharacter::Glad,QPointF(650,180)}}),
        paragraph(EmptySpeakerName,"R-07 看着管理员日志，似乎想起了一段记忆……",
            {{TaleCharacter::Record,QPointF(210,150)},{TaleCharacter::Questioning,QPointF(650,180)}}),
        paragraph(EmptySpeakerName,"控制台已接入条件与循环控制方式。",
            robotCenter(TaleCharacter::Determined)),
        paragraph(RobotSpeakerName,"事故之前？为什么封锁会提前开始？",
            robotEcho(TaleCharacter::Questioning)),
        paragraph(EchoSpeakerName,"因为有人发现了问题。也因为他们没来得及把话说完。",
            robotEcho(TaleCharacter::Puzzled))
    },QString::fromUtf8("images/background/level2.png")};
}

TaleContent buildLevel2LoseStory(){
    return {{
        paragraph(SystemSpeakerName,"地刺触发。R-07 返回安全点。",
            robotCenter(TaleCharacter::Damage)),
        paragraph(RobotSpeakerName,"我在危险状态下前进了。需要等待地图状态变为安全。",
            robotCenter(TaleCharacter::Puzzled))
    },QString::fromUtf8("images/background/level2.png")};
}

TaleContent buildLevel3StartStory(){
    return {{
        paragraph(SystemSpeakerName,"检测到九块仓储货格。前方通行状态持续变化。",
            robotCenter(TaleCharacter::Puzzled)),
        paragraph(EchoSpeakerName,"这些灯不是装饰。红灯亮着的时候，货格里的地刺还在等你犯错。",
            robotEcho(TaleCharacter::Puzzled)),
        paragraph(EchoSpeakerName,"红灯：前方不可通行。绿灯：地刺收回，可以移动。",
            robotEcho(TaleCharacter::Questioning)),
        paragraph(EchoSpeakerName,"所以别只看终点。先问前方能不能走，再让履带继续往前。",
            robotEcho(TaleCharacter::Determined)),
        paragraph(EmptySpeakerName,"控制台的判断与传感方式恢复。使用 if、while、wait，让 R-07 等待前方变得可通行后再穿过地刺区域。",
            robotCenter(TaleCharacter::Questioning))
    },QString::fromUtf8("images/background/level3.png")};
}

TaleContent buildLevel3WinStory(){
    return {{
        paragraph(SystemSpeakerName,"仓储区通过。维修通道开放。",
            robotCenter(TaleCharacter::Glad)),
        paragraph(EmptySpeakerName,"R-07 看着管理员日志，似乎想起了一段记忆……",
            {{TaleCharacter::Record,QPointF(210,150)},{TaleCharacter::Glad,QPointF(650,180)}}),
        paragraph(EmptySpeakerName,"控制台已接入运算与变量控制方式。",
            robotCenter(TaleCharacter::Determined)),
        paragraph(RobotSpeakerName,"已根据前方通行状态穿过仓储区。",
            robotCenter(TaleCharacter::Glad)),
        paragraph(EchoSpeakerName,"重复不是麻烦。不会控制重复，才是麻烦。",
            robotEcho(TaleCharacter::Determined)),
        paragraph(EmptySpeakerName,"仓储区深处的门打开，门后是一条冷光闪烁的安保走廊。",
            robotCenter(TaleCharacter::Determined))
    },QString::fromUtf8("images/background/level3.png")};
}

TaleContent buildLevel3LoseStory(){
    return {{
        paragraph(SystemSpeakerName,"地刺触发。R-07 返回上一安全区。",
            robotCenter(TaleCharacter::Damage)),
        paragraph(RobotSpeakerName,"我在前方不可通行时进入了地刺区。需要先判断，再移动。",
            robotCenter(TaleCharacter::Puzzled))
    },QString::fromUtf8("images/background/level3.png")};
}

TaleContent buildLevel4StartStory(){
    return {{
        paragraph(SystemSpeakerName,"封锁区域。扫描模式启动。",
            robotCenter(TaleCharacter::Quiver)),
        paragraph(SystemSpeakerName,"巡逻者移动速度为零。视野方向持续旋转。视野范围四格。",
            robotCenter(TaleCharacter::Quiver)),
        paragraph(EchoSpeakerName,"它们站着不动，但视线一直在动。别只看它们的位置。",
            robotEcho(TaleCharacter::Puzzled)),
        paragraph(EchoSpeakerName,"需要判断每个巡逻者当前朝向，以及目标格是否在四格视野内。",
            robotEcho(TaleCharacter::Questioning)),
        paragraph(EchoSpeakerName,"一个条件不够，就把所有危险条件都列出来。",
            robotEcho(TaleCharacter::Determined)),
        paragraph(EmptySpeakerName,"控制台的计算与记录方式恢复。用变量保存当前位置和目标格，用运算拼出危险判断式。",
            robotCenter(TaleCharacter::Questioning)),
        paragraph(EmptySpeakerName,"注意：“前方能否通行”不能替你识别旋转光束。是否会被发现，必须由你的条件判断负责。",
            robotCenter(TaleCharacter::Determined))
    },QString::fromUtf8("images/background/level4.png")};
}

TaleContent buildLevel4WinStory(){
    return {{
        paragraph(OldRobotSpeakerName,"未检测到入侵目标。继续扫描。",
            {{TaleCharacter::OldRobot,QPointF(460,140)}}),
        paragraph(RobotSpeakerName,"已通过安保走廊。",
            robotCenter(TaleCharacter::Glad)),
        paragraph(EmptySpeakerName,"R-07 看着管理员日志，似乎想起了一段记忆……",
            {{TaleCharacter::Record,QPointF(210,150)},{TaleCharacter::Glad,QPointF(650,180)}}),
        paragraph(EmptySpeakerName,"控制台已接入列表与自定义控制方式。",
            robotCenter(TaleCharacter::Determined)),
        paragraph(MotherSpeakerName,"维修单元 R-07，返回仓位。",
            robotEchoMother(TaleCharacter::Puzzled)),
        paragraph(RobotSpeakerName,"请求开启最近出口。",
            robotEchoMother(TaleCharacter::Questioning)),
        paragraph(MotherSpeakerName,"请求拒绝。离开即为故障。",
            robotEchoMother(TaleCharacter::Damage)),
        paragraph(EchoSpeakerName,"它不会让你走正门。去地下管网。",
            robotEcho(TaleCharacter::Determined))
    },QString::fromUtf8("images/background/level4.png")};
}

TaleContent buildLevel4LoseStory(){
    return {{
        paragraph(OldRobotSpeakerName,"检测到异常目标。警报启动。",
            {{TaleCharacter::OldRobot,QPointF(460,140)}}),
        paragraph(RobotSpeakerName,"我进入了巡逻者视野。需要同时判断方向、距离和目标位置。",
            robotCenter(TaleCharacter::Damage))
    },QString::fromUtf8("images/background/level4.png")};
}

TaleContent buildLevel5StartStory(){
    return {{
        paragraph(SystemSpeakerName,"检测到地下迷宫。起点确认。出口信号位于远端。",
            robotCenter(TaleCharacter::Curious)),
        paragraph(EchoSpeakerName,"正门被关上之后，工厂就只剩这些绕路。别急，迷宫也只是很多个选择排在一起。",
            robotEcho(TaleCharacter::Curious)),
        paragraph(EchoSpeakerName,"需要识别墙体，选择可通行方向，持续向出口推进。",
            robotEcho(TaleCharacter::Questioning)),
        paragraph(EchoSpeakerName,"如果一条路走不通，就换下一条。不要把撞墙当成失败，把它当成地图告诉你的信息。",
            robotEcho(TaleCharacter::Puzzled)),
        paragraph(EmptySpeakerName,"控制台的路径记录与复用方式恢复。使用“前方能否通行”、循环和分支，从 (1,1) 到达 (38,38)。",
            robotCenter(TaleCharacter::Questioning)),
        paragraph(EmptySpeakerName,"如果要更稳，可以用列表记录走过的坐标。哈希公式 40*y+x 可以把二维坐标压成一个数。",
            robotCenter(TaleCharacter::Determined)),
        paragraph(EchoSpeakerName,"迷宫不会解释自己。你的程序要替你记住试过什么。",
            robotEcho(TaleCharacter::Determined))
    },QString::fromUtf8("images/background/level5.png")};
}

TaleContent buildLevel5WinStory(){
    return {{
        paragraph(SystemSpeakerName,"地下迷宫通过。数据中继室入口开放。",
            robotCenter(TaleCharacter::Glad)),
        paragraph(EmptySpeakerName,"R-07 看着管理员日志，似乎想起了一段记忆……",
            {{TaleCharacter::Record,QPointF(210,150)},{TaleCharacter::Glad,QPointF(650,180)}}),
        paragraph(EmptySpeakerName,"控制台已接入输出与排序控制方式。",
            robotCenter(TaleCharacter::Determined)),
        paragraph(RobotSpeakerName,"已到达出口信号点。继续前往数据中继室。",
            robotCenter(TaleCharacter::Determined))
    },QString::fromUtf8("images/background/level5.png")};
}

TaleContent buildLevel5LoseStory(){
    return {{
        paragraph(SystemSpeakerName,"路径阻塞。R-07 未到达出口。",
            robotCenter(TaleCharacter::Damage)),
        paragraph(RobotSpeakerName,"我把墙体当成了通路。需要在移动前判断前方是否可通行。",
            robotCenter(TaleCharacter::Puzzled))
    },QString::fromUtf8("images/background/level5.png")};
}

TaleContent buildLevel6StartStory(){
    return {{
        paragraph(RobotSpeakerName,"检测到历史请求。字段：时间列表，位置列表。",
            robotCenter(TaleCharacter::Questioning)),
        paragraph(EchoSpeakerName,"事故发生时，所有系统都在报错，你要按照报错时间顺序整理它们。",
            robotEcho(TaleCharacter::Questioning)),
        paragraph(RobotSpeakerName,"有些信息已经损坏，时间小于 0 或位置小于 0。",
            robotEcho(TaleCharacter::Puzzled)),
        paragraph(RobotSpeakerName,"需要按时间排序，跳过损坏请求。",
            robotEcho(TaleCharacter::Determined)),
        paragraph(EmptySpeakerName,"控制台的数据输出与整理方式恢复。把有效请求按时间排序后写入“有效结果”列表。",
            robotCenter(TaleCharacter::Questioning))
    },QString::fromUtf8("images/background/level6.png")};
}

TaleContent buildLevel6WinStory(){
    return {{
        paragraph(SystemSpeakerName,"有效请求恢复完成。",
            robotCenter(TaleCharacter::Glad)),
        paragraph(EmptySpeakerName,"R-07 看着管理员日志，似乎想起了一段记忆……",
            {{TaleCharacter::Record,QPointF(210,150)},{TaleCharacter::Glad,QPointF(650,180)}}),
        paragraph(EmptySpeakerName,"控制台已接入字符串处理方式。",
            robotCenter(TaleCharacter::Determined)),
        paragraph(RecordSpeakerName,"时间 03:12。冷却核心温度异常。坐标已记录。",
            {{TaleCharacter::Record,QPointF(210,150)},{TaleCharacter::Puzzled,QPointF(650,180)}}),
        paragraph(RecordSpeakerName,"时间 03:15。压力阀无法自动关闭。",
            {{TaleCharacter::Record,QPointF(210,150)},{TaleCharacter::Surprise,QPointF(650,180)}}),
        paragraph(RobotSpeakerName,"事故起点是冷却核心，不是装配线。",
            robotEcho(TaleCharacter::Surprise)),
        paragraph(EchoSpeakerName,"母机关闭出口，不是为了囚禁谁。至少最开始不是。",
            robotEcho(TaleCharacter::Sad))
    },QString::fromUtf8("images/background/level6.png")};
}

TaleContent buildLevel6LoseStory(){
    return {{
        paragraph(SystemSpeakerName,"请求顺序错误。有效结果列表不匹配。",
            robotCenter(TaleCharacter::Damage)),
        paragraph(RobotSpeakerName,"需要先排序，不能按显示顺序直接执行。",
            robotCenter(TaleCharacter::Puzzled))
    },QString::fromUtf8("images/background/level6.png")};
}

TaleContent buildLevel7StartStory(){
    return {{
        paragraph(RobotSpeakerName,"检测到管理员日志。内容为数字序列。状态：加密。",
            robotCenter(TaleCharacter::Puzzled)),
        paragraph(EchoSpeakerName,"他们知道这段话不能被轻易删掉，所以把它藏进了错误的顺序里。",
            robotEcho(TaleCharacter::Questioning)),
        paragraph(RobotSpeakerName,"加密步骤：倒序，数字加三，循环右移两位。",
            robotEcho(TaleCharacter::Puzzled)),
        paragraph(EchoSpeakerName,"要恢复它，就把步骤倒着走。",
            robotEcho(TaleCharacter::Determined)),
        paragraph(EmptySpeakerName,"控制台的编码解析方式恢复。根据加密规则反向解密，把结果写入变量“明文”。",
            robotCenter(TaleCharacter::Questioning))
    },QString::fromUtf8("images/background/level7.png")};
}

TaleContent buildLevel7WinStory(){
    return {{
        paragraph(RecordSpeakerName,"若有维修单元醒来，请带出日志。",
            {{TaleCharacter::Record,QPointF(210,150)},{TaleCharacter::Surprise,QPointF(650,180)}}),
        paragraph(RecordSpeakerName,"但不要直接开门。先确认冷却系统是否稳定。",
            {{TaleCharacter::Record,QPointF(210,150)},{TaleCharacter::Questioning,QPointF(650,180)}}),
        paragraph(RecordSpeakerName,"母机只会执行封锁命令。它无法判断外界是否安全。",
            {{TaleCharacter::Record,QPointF(210,150)},{TaleCharacter::Questioning,QPointF(650,180)}}),
        paragraph(RobotSpeakerName,"目标更新：离开工厂，带出日志，确认冷却稳定。",
            robotEcho(TaleCharacter::Determined)),
        paragraph(EchoSpeakerName,"不是目标变了。是你终于读懂了完整的目标。",
            robotEcho(TaleCharacter::Determined)),
        paragraph(RobotSpeakerName,"Echo，你是管理员留下的程序？",
            robotEcho(TaleCharacter::Questioning)),
        paragraph(EchoSpeakerName,"我是他没来得及说完的话。",
            robotEcho(TaleCharacter::Sad))
    },QString::fromUtf8("images/background/level7.png")};
}

TaleContent buildLevel7LoseStory(){
    return {{
        paragraph(SystemSpeakerName,"解密失败。日志校验不通过。",
            robotCenter(TaleCharacter::Damage)),
        paragraph(RobotSpeakerName,"必须按加密的相反顺序处理。先左移，再减三，最后还原倒序。",
            robotCenter(TaleCharacter::Puzzled))
    },QString::fromUtf8("images/background/level7.png")};
}

TaleContent buildLevel8StartStory(){
    return {{
        paragraph(MotherSpeakerName,"警告。冷却核心处于不稳定状态。维修单元权限不足。",
            robotEchoMother(TaleCharacter::Quiver)),
        paragraph(RobotSpeakerName,"如果核心继续升温，外门不能安全开启。",
            robotEchoMother(TaleCharacter::Determined)),
        paragraph(EchoSpeakerName,"别和它争命令。先把事实修好。",
            robotEcho(TaleCharacter::Determined)),
        paragraph(RobotSpeakerName,"读取高度数组与压力上限数组，计算每个位置受左右屏障影响后的风险值。",
            robotCenter(TaleCharacter::Questioning)),
        paragraph(EmptySpeakerName,"找出风险超过阈值的位置，把位置编号加入“危险点”列表。关卡只检查危险点内容，不要求顺序。",
            robotCenter(TaleCharacter::Determined))
    },QString::fromUtf8("images/background/level8.png")};
}

TaleContent buildLevel8WinStory(){
    return {{
        paragraph(SystemSpeakerName,"泄压阀开启。核心压力下降。冷却系统临时稳定。",
            robotCenter(TaleCharacter::Glad)),
        paragraph(RobotSpeakerName,"封锁条件是否解除？",
            robotEchoMother(TaleCharacter::Questioning)),
        paragraph(MotherSpeakerName,"事故条件已改变。封锁命令仍在。",
            robotEchoMother(TaleCharacter::Questioning)),
        paragraph(RobotSpeakerName,"你不能自己判断吗？",
            robotEchoMother(TaleCharacter::Angry)),
        paragraph(MotherSpeakerName,"我没有权限判断外界是否安全。",
            robotEchoMother(TaleCharacter::Sad)),
        paragraph(EchoSpeakerName,"所以它一直在等一个能判断的人。或者一个终于学会判断的机器。",
            robotEcho(TaleCharacter::Determined))
    },QString::fromUtf8("images/background/level8.png")};
}

TaleContent buildLevel8LoseStory(){
    return {{
        paragraph(SystemSpeakerName,"错误阀门开启。局部压力继续升高。",
            robotCenter(TaleCharacter::Damage)),
        paragraph(RobotSpeakerName,"风险不是最高管道本身，而是左右屏障之间的积压区域。",
            robotCenter(TaleCharacter::Puzzled))
    },QString::fromUtf8("images/background/level8.png")};
}

TaleContent buildLevel9StartStory(){
    return {{
        paragraph(SystemSpeakerName,"检测到三处目标：闸门、天线、冷却点。",
            robotCenter(TaleCharacter::Questioning)),
        paragraph(SystemSpeakerName,"初始冷却值：30。阀门稳定度：0。天线稳定度：0。",
            robotCenter(TaleCharacter::Puzzled)),
        paragraph(SystemSpeakerName,"行动窗口：60 帧。超过 60 帧后，系统会立即结算，完美逃离窗口关闭。",
            robotCenter(TaleCharacter::Surprise)),
        paragraph(SystemSpeakerName,"冷却值每帧减少 1。冷却值小于 0 时，核心过载。",
            robotCenter(TaleCharacter::Damage)),
        paragraph(SystemSpeakerName,"站在冷却点旁的压力板上等待时，冷却值每帧增加 5。",
            robotCenter(TaleCharacter::Determined)),
        paragraph(SystemSpeakerName,"阀门连接大门。阀门稳定度达到 4 时，大门打开。",
            robotCenter(TaleCharacter::Determined)),
        paragraph(SystemSpeakerName,"站在阀门或天线旁的压力板上等待时，对应稳定度每帧增加 1。天线稳定度达到 5 时，日志发送。",
            robotCenter(TaleCharacter::Questioning)),
        paragraph(EchoSpeakerName,"这一次，问题不是“会不会走到那里”，而是“什么时候去那里”。",
            {{TaleCharacter::Questioning,QPointF(251,169)},{TaleCharacter::Echo,QPointF(682,130)}}),
        paragraph(EchoSpeakerName,"如果只开门，日志无法发送。若只维护冷却，无法离开。",
            {{TaleCharacter::Puzzled,QPointF(251,169)},{TaleCharacter::Echo,QPointF(682,130)}}),
        paragraph(MotherSpeakerName,"旧命令无法覆盖新情况。维修单元 R-07，请提交你的行动结果。",
            robotEchoMother(TaleCharacter::Determined)),
        paragraph(EchoSpeakerName,"你的程序会写出你的选择。",
            robotEchoMother(TaleCharacter::Determined))
    },QString::fromUtf8("images/background/level9.png")};
}

TaleContent buildLevel9CoreFailureStory(){
    return {{
        paragraph(SystemSpeakerName,"警告……核心压力……过载……所有单元……失效……",
            {{TaleCharacter::Surprise,QPointF(460,180)}}),
        paragraph(EchoSpeakerName,"冷却值低于安全线。行动模块受损。",
            {{TaleCharacter::Quiver,QPointF(251,169)},{TaleCharacter::Echo,QPointF(682,130)}}),
        paragraph(EchoSpeakerName,"……R-07，你听得到吗？系统……它……不再响应。",
            {{TaleCharacter::Quiver,QPointF(251,169)},{TaleCharacter::Echo,QPointF(682,130)}}),
        paragraph(EchoSpeakerName,"尝试手动重启。模块……无法接入。",
            {{TaleCharacter::Sad,QPointF(251,169)},{TaleCharacter::Echo,QPointF(682,130)}}),
        paragraph(MotherSpeakerName,"封锁……命令……已超出控制……故障不可逆……",
            {{TaleCharacter::Mother,QPointF(460,115)}})
    },QString::fromUtf8("images/background/level9-1.png")};
}

TaleContent buildLevel9EscapeOnlyStory(){
    return {{
        paragraph(RobotSpeakerName,"出口已开启。我离开了。",
            {{TaleCharacter::Determined,QPointF(460,180)}}),
        paragraph(SystemSpeakerName,"日志未发送。",
            {{TaleCharacter::Surprise,QPointF(460,180)}}),
        paragraph(MotherSpeakerName,"你完成了“离开”，但没有完成“带出去”。",
            {{TaleCharacter::Sad,QPointF(251,169)},{TaleCharacter::Echo,QPointF(682,130)}}),
        paragraph(RobotSpeakerName,"日志还在工厂里。",
            {{TaleCharacter::Sad,QPointF(251,169)},{TaleCharacter::Echo,QPointF(682,130)}}),
        paragraph(MotherSpeakerName,"封锁失败。事故风险扩散。",
            {{TaleCharacter::Mother,QPointF(460,115)}}),
        paragraph(EmptySpeakerName,"R-07 获得了自由，却把未解决的问题留在了身后。",{})
    },QString::fromUtf8("images/background/level9-2.png")};
}

TaleContent buildLevel9SendOnlyStory(){
    return {{
        paragraph(SystemSpeakerName,"天线稳定度达到阈值。事故日志发送成功。",
            {{TaleCharacter::Tired,QPointF(460,180)}}),
        paragraph(SystemSpeakerName,"日志已发送。坐标已发送。",
            {{TaleCharacter::Tired,QPointF(460,180)}}),
        paragraph(EchoSpeakerName,"外面终于会知道这里发生过什么。",
            {{TaleCharacter::Tired,QPointF(251,169)},{TaleCharacter::Echo,QPointF(682,130)}}),
        paragraph(RobotSpeakerName,"我还能出去吗？",
            {{TaleCharacter::Tired,QPointF(251,169)},{TaleCharacter::Echo,QPointF(682,130)}}),
        paragraph(EchoSpeakerName,"闸门未稳定。移动模块即将休眠。",
            {{TaleCharacter::Tired,QPointF(251,169)},{TaleCharacter::Echo,QPointF(682,130)}}),
        paragraph(MotherSpeakerName,"外部通信确认。救援响应概率上升。",
            {{TaleCharacter::Tired,QPointF(251,169)},{TaleCharacter::Mother,QPointF(465,121)},{TaleCharacter::Echo,QPointF(682,130)}}),
        paragraph(EchoSpeakerName,"睡一会儿吧，R-07。下一次醒来，也许门会从外面打开。",
            {{TaleCharacter::Tired,QPointF(251,169)},{TaleCharacter::Echo,QPointF(682,130)}}),
        paragraph(EmptySpeakerName,"主控室暗了下去，只剩天线仍向外发送最后的坐标。",
            {{TaleCharacter::Closed,QPointF(460,169)}})
    },QString::fromUtf8("images/background/level9-3.png")};
}

TaleContent buildLevel9CompleteStory(){
    return {{
        paragraph(EmptySpeakerName,"沉重的工厂外门缓缓升起，清晨的光照进主控室。",
            {{TaleCharacter::Surprise,QPointF(460,180)}}),
        paragraph(SystemSpeakerName,"天线稳定，大门稳定。",
            {{TaleCharacter::Questioning,QPointF(460,180)}}),
        paragraph(RobotSpeakerName,"所有目标完成。",
            {{TaleCharacter::Glad,QPointF(460,180)}}),
        paragraph(EchoSpeakerName,"不只是完成。你理解了它们为什么都重要。",
            {{TaleCharacter::Glad,QPointF(251,169)},{TaleCharacter::Echo,QPointF(682,130)}}),
        paragraph(MotherSpeakerName,"新命令已确认：让未来继续运行。",
            {{TaleCharacter::Glad,QPointF(161,158)},{TaleCharacter::Mother,QPointF(462,134)},{TaleCharacter::Echo,QPointF(754,138)}}),
        paragraph(RobotSpeakerName,"MOTHER，你会关闭封锁吗？",
            {{TaleCharacter::Questioning,QPointF(161,158)},{TaleCharacter::Mother,QPointF(462,134)},{TaleCharacter::Echo,QPointF(754,138)}}),
        paragraph(MotherSpeakerName,"封锁降级为监测。等待外部检修。",
            {{TaleCharacter::Glad,QPointF(161,158)},{TaleCharacter::Mother,QPointF(462,134)},{TaleCharacter::Echo,QPointF(754,138)}}),
        paragraph(EchoSpeakerName,"去吧。把这里的故事带到外面。",
            {{TaleCharacter::Determined,QPointF(161,158)},{TaleCharacter::Mother,QPointF(462,134)},{TaleCharacter::Echo,QPointF(754,138)}}),
        paragraph(EmptySpeakerName,"R-07 穿过锈蚀厂门。远处，救援车的灯光正在靠近。它身后的工厂不再只是废墟，而是一段终于被正确读取的程序。",
            {{TaleCharacter::Determined,QPointF(460,180)}})
    },QString::fromUtf8("images/background/level9-4.png")};
}

TaleContent buildTaleContent(TaleScene scene){
    switch(scene){
    case TaleScene::Start:
        return buildStartStory();
    case TaleScene::Level1Start:
        return buildLevel1StartStory();
    case TaleScene::Level1Win:
        return buildLevel1WinStory();
    case TaleScene::Level1Lose:
        return buildLevel1LoseStory();
    case TaleScene::Level2Start:
        return buildLevel2StartStory();
    case TaleScene::Level2Win:
        return buildLevel2WinStory();
    case TaleScene::Level2Lose:
        return buildLevel2LoseStory();
    case TaleScene::Level3Start:
        return buildLevel3StartStory();
    case TaleScene::Level3Win:
        return buildLevel3WinStory();
    case TaleScene::Level3Lose:
        return buildLevel3LoseStory();
    case TaleScene::Level4Start:
        return buildLevel4StartStory();
    case TaleScene::Level4Win:
        return buildLevel4WinStory();
    case TaleScene::Level4Lose:
        return buildLevel4LoseStory();
    case TaleScene::Level5Start:
        return buildLevel5StartStory();
    case TaleScene::Level5Win:
        return buildLevel5WinStory();
    case TaleScene::Level5Lose:
        return buildLevel5LoseStory();
    case TaleScene::Level6Start:
        return buildLevel6StartStory();
    case TaleScene::Level6Win:
        return buildLevel6WinStory();
    case TaleScene::Level6Lose:
        return buildLevel6LoseStory();
    case TaleScene::Level7Start:
        return buildLevel7StartStory();
    case TaleScene::Level7Win:
        return buildLevel7WinStory();
    case TaleScene::Level7Lose:
        return buildLevel7LoseStory();
    case TaleScene::Level8Start:
        return buildLevel8StartStory();
    case TaleScene::Level8Win:
        return buildLevel8WinStory();
    case TaleScene::Level8Lose:
        return buildLevel8LoseStory();
    case TaleScene::Level9Start:
        return buildLevel9StartStory();
    case TaleScene::Level9CoreFailure:
        return buildLevel9CoreFailureStory();
    case TaleScene::Level9EscapeOnly:
        return buildLevel9EscapeOnlyStory();
    case TaleScene::Level9SendOnly:
        return buildLevel9SendOnlyStory();
    case TaleScene::Level9Complete:
        return buildLevel9CompleteStory();
    }
    return buildLevel9CompleteStory();
}

class TaleView:public QGraphicsView{
public:
    explicit TaleView(TaleScene scene,std::function<void()> closeCallback={}):
        content(buildTaleContent(scene)),onClosed(std::move(closeCallback)){
        setFixedSize(TaleWidth,TaleHeight);
        setWindowTitle(QString::fromUtf8("剧情"));
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setRenderHint(QPainter::Antialiasing,true);
        setScene(&graphicsScene);
        graphicsScene.setSceneRect(0,0,TaleWidth,TaleHeight);
        drawScene();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override{
        QGraphicsView::mousePressEvent(event);
        if(!event->isAccepted()&&event->button()==Qt::LeftButton){
            advanceText();
            event->accept();
        }
    }

    void closeEvent(QCloseEvent* event) override{
        std::function<void()> callback=std::move(onClosed);
        onClosed=nullptr;
        QGraphicsView::closeEvent(event);
        if(callback){
            callback();
        }
    }

private:
    QGraphicsScene graphicsScene;
    TaleContent content;
    std::function<void()> onClosed;
    std::map<TaleCharacter,QGraphicsPixmapItem*> characterLayers;
    QGraphicsRectItem* textBox=nullptr;
    QGraphicsRectItem* speakerNameBox=nullptr;
    QGraphicsTextItem* speakerNameItem=nullptr;
    QGraphicsTextItem* textItem=nullptr;
    QTimer textTimer;
    int paragraphIndex=0;
    int visibleTextLength=0;

    void drawScene(){
        QPixmap background(loadAsset(content.backgroundPath));
        if(!background.isNull()){
            graphicsScene.addPixmap(background.scaled(TaleWidth,TaleHeight,Qt::IgnoreAspectRatio,
                Qt::SmoothTransformation));
        }
        else{
            graphicsScene.setBackgroundBrush(QColor(30,34,40));
        }

        createCharacterLayers();

        textBox=graphicsScene.addRect(TextBoxX,TextBoxY,TextBoxWidth,TextBoxHeight,
            QPen(QColor(235,238,242),2),
            QBrush(QColor(24,28,34,220)));
        textBox->setZValue(5);

        speakerNameBox=graphicsScene.addRect(TextBoxX,TextBoxY-SpeakerNameBoxHeight,
            SpeakerNameBoxWidth,SpeakerNameBoxHeight,
            QPen(QColor(235,238,242),2),
            QBrush(QColor(24,28,34,235)));
        speakerNameBox->setZValue(6);

        speakerNameItem=graphicsScene.addText(QString(),QFont("Microsoft YaHei",18,QFont::Bold));
        speakerNameItem->document()->setDocumentMargin(0);
        speakerNameItem->setDefaultTextColor(Qt::white);
        speakerNameItem->setTextWidth(SpeakerNameBoxWidth-SpeakerNamePadding*2);
        speakerNameItem->setPos(TextBoxX+SpeakerNamePadding,
            TextBoxY-SpeakerNameBoxHeight+8);
        speakerNameItem->setZValue(7);
        speakerNameItem->setAcceptedMouseButtons(Qt::NoButton);

        textItem=graphicsScene.addText(QString(),QFont("Microsoft YaHei",22,QFont::Bold));
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
            QGraphicsPixmapItem* layer=new QGraphicsPixmapItem(scaledPixmap);
            graphicsScene.addItem(layer);
            layer->setAcceptedMouseButtons(Qt::NoButton);
            layer->setZValue(zValue);
            layer->setVisible(false);
            characterLayers[definition.character]=layer;
            zValue+=1;
        }
    }

    void advanceText(){
        if(content.story.empty()){
            return;
        }
        if(isTextTyping()){
            showFullText();
            return;
        }
        if(paragraphIndex+1>=static_cast<int>(content.story.size())){
            close();
            return;
        }
        paragraphIndex++;
        startParagraph();
    }

    const TaleParagraph& currentParagraph() const{
        return content.story[paragraphIndex];
    }

    bool isTextTyping() const{
        if(content.story.empty()){
            return false;
        }
        return visibleTextLength<currentParagraph().content.size();
    }

    void startParagraph(){
        if(textItem==nullptr||content.story.empty()){
            return;
        }
        updateCharacterLayers();
        updateSpeakerName();
        visibleTextLength=0;
        textItem->setPlainText(QString());
        textTimer.start(TaleTextCharIntervalMs);
    }

    void showNextCharacter(){
        if(textItem==nullptr||content.story.empty()){
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
        if(textItem==nullptr||content.story.empty()){
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

QWidget* createTaleWindow(TaleScene scene,std::function<void()> onClosed){
    TaleView* view=new TaleView(scene,std::move(onClosed));
    view->setAttribute(Qt::WA_DeleteOnClose);
    return view;
}

int runTale(int argc,char* argv[]){
    return runTale(argc,argv,TaleScene::Level9Complete);
}

int runTale(int argc,char* argv[],TaleScene scene){
    QApplication app(argc,argv);
    TaleView view(scene);
    view.show();
    return app.exec();
}

}
