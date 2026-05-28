QT += widgets

CONFIG += c++17
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
SOURCES += \
    main.cpp \
    core/Block.cpp \
    core/Ops.cpp \
    core/Runtime.cpp \
    core/Value.cpp \
    level/Level.cpp \
    message/Message.cpp \
    ui/App.cpp\
    tale.cpp \
    ui/MainWindow.cpp \
    ui/levelchoosepage.cpp
HEADERS += \
    core/Block.h \
    core/Ops.h \
    core/Runtime.h \
    core/Value.h \
    level/Level.h \
    level/LevelConstants.h \
    message/Message.h \
    ui/App.h \
    ui/AppGraphicsView.h \
    ui/MainWindow.h \
    ui/UiConstants.h \
    ui/Widgets.h\
    tale.h \
    ui/levelchoosepage.h

RESOURCES += \
    resource.qrc
