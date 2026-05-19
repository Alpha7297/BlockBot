#ifndef UI_CONSTANTS_H
#define UI_CONSTANTS_H

#include <QtGlobal>

inline constexpr int shadowPadding=0;
inline constexpr int floatBlockWidth=25;
inline constexpr int opHorizontalPadding=5;
inline constexpr int opVerticalPadding=3;
inline constexpr int floatAttachDistance=20;
inline constexpr int screensize=40;
inline constexpr int squaresize=320/screensize;
inline constexpr double PI=3.14159265358979323846;
inline constexpr int scrollNone=0;
inline constexpr int scrollToolbox=1;
inline constexpr int scrollWorkspace=2;
inline constexpr qreal panelMaskZ=100000;
inline constexpr qreal sliderZ=100010;
inline constexpr qreal draggingZ=1000000000;
inline constexpr qreal absorbHighlightZ=1000000100;

#endif
