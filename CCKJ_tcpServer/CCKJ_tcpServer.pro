QT       += core gui
QT       += network  #添加网络支持的模块

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mytcpsocket.cpp \
    tcp_serverwidget.cpp \
    tcp_serverworker.cpp

HEADERS += \
    mytcpsocket.h \
    tcp_serverwidget.h \
    tcp_serverworker.h

FORMS += \
    tcp_serverwidget.ui

RC_ICONS = icon\main.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
