#ifndef TALE_TALEWINDOW_H
#define TALE_TALEWINDOW_H

#include <functional>

class QWidget;

namespace tale{

enum class TaleScene{
    Start,
    Level1Start,
    Level1Win,
    Level1Lose,
    Level2Start,
    Level2Win,
    Level2Lose,
    Level3Start,
    Level3Win,
    Level3Lose,
    Level4Start,
    Level4Win,
    Level4Lose,
    Level5Start,
    Level5Win,
    Level5Lose,
    Level6Start,
    Level6Win,
    Level6Lose,
    Level7Start,
    Level7Win,
    Level7Lose,
    Level8Start,
    Level8Win,
    Level8Lose,
    Level9Start,
    Level9CoreFailure,
    Level9EscapeOnly,
    Level9SendOnly,
    Level9Complete
};

QWidget* createTaleWindow(TaleScene scene,std::function<void()> onClosed={});
int runTale(int argc,char* argv[]);
int runTale(int argc,char* argv[],TaleScene scene);

}

#endif
