QT += core
QT -= gui
QT += concurrent


TARGET = zeromqdemo
CONFIG += console
CONFIG -= app_bundle
CONFIG += C++11

TEMPLATE = app

DESTDIR = $$PWD/../../Release/Construction/Bin

DEFINES += WIN32_LEAN_AND_MEAN

QMAKE_CXXFLAGS_RELEASE += /NODEFAULTLIB:"libcmt.lib"
QMAKE_LFLAGS_RELEASE += /DEBUG

include(../ThirdParty/zeroMQ.pri)
include(../ThirdParty/libevent.pri)


SOURCES += main.cpp \
    ZeroUtils.cpp

HEADERS += \
    ZeroUtils.h \
    zhelpers.h

