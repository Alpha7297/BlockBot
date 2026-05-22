#ifndef UI_CONSTANTS_H
#define UI_CONSTANTS_H

#include <QtGlobal>

inline constexpr int shadowPadding=0;
inline constexpr int appWidth=1440;
inline constexpr int appHeight=720;
inline constexpr int stageX=20;
inline constexpr int stageY=20;
inline constexpr int stagePixelSize=320;
inline constexpr int toolboxX=360;
inline constexpr int toolboxY=0;
inline constexpr int toolboxWidth=180;
inline constexpr int toolboxHeight=appHeight;
inline constexpr int workspaceX=560;
inline constexpr int workspaceY=0;
inline constexpr int workspaceWidth=appWidth-workspaceX;
inline constexpr int workspaceHeight=appHeight;
inline constexpr int floatBlockWidth=25;
inline constexpr int opHorizontalPadding=5;
inline constexpr int opVerticalPadding=3;
inline constexpr qreal codeConnectorX=8;
inline constexpr qreal codeConnectorWidth=7;
inline constexpr qreal codeConnectorHeight=5;
inline constexpr int floatAttachDistance=20;
inline constexpr int screensize=40;
inline constexpr int squaresize=stagePixelSize/screensize;
inline constexpr double PI=3.14159265358979323846;
inline constexpr double EPS=1e-8;
inline constexpr int scrollNone=0;
inline constexpr int scrollToolbox=1;
inline constexpr int scrollWorkspace=2;
inline constexpr qreal panelMaskZ=100000;
inline constexpr qreal sliderZ=100010;
inline constexpr qreal draggingZ=1000000000;
inline constexpr qreal absorbHighlightZ=1000000100;

#endif
