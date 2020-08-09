#-------------------------------------------------
#
# Project created by QtCreator 2019-10-21T17:20:17
#
#-------------------------------------------------
QT += core
QT       -= gui

TARGET = ZMDCommon
TEMPLATE = lib

DEFINES += ZMDCOMMON_LIBRARY
DEFINES += WIN32_LEAN_AND_MEAN
DESTDIR = $$PWD/../../Release/Construction/Bin

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QMAKE_CXXFLAGS_RELEASE += /NODEFAULTLIB:"libcmt.lib"
QMAKE_LFLAGS_RELEASE += /DEBUG

include(../ThirdParty/zeroMQ.pri)
include(../ThirdParty/libevent.pri)

SOURCES += \
    ZMDConst.cpp \
    MTMessage.cpp \
    ZMDUtils.cpp


HEADERS += \
    zmdcommon_global.h \
    zhelpers.h \
    ZMDConst.h \
    MTMessage.h \
    ZMDUtils.h


unix {
    target.path = /usr/lib
    INSTALLS += target
}
