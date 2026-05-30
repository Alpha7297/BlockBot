QT += widgets

CONFIG += c++17
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
SOURCES += \
    main.cpp \
    ui/ArchivePage.cpp \
    core/Block.cpp \
    core/Ops.cpp \
    core/Runtime.cpp \
    core/Value.cpp \
    level/Level.cpp \
    level/mazeGernerator.cpp \
    message/Message.cpp \
    ui/App.cpp\
    ui/MainWindow.cpp \
    ui/LevelChoosePage.cpp
HEADERS += \
    ui/ArchivePage.h \
    core/Block.h \
    core/Ops.h \
    core/Runtime.h \
    core/Value.h \
    level/Level.h \
    level/LevelConstants.h \
    level/MazeGenerator.h \
    message/Message.h \
    ui/App.h \
    ui/AppGraphicsView.h \
    ui/MainWindow.h \
    ui/UiConstants.h \
    ui/Widgets.h\
    ui/LevelChoosePage.h


