QT       += core gui network widgets

TARGET = ZMDWorker
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
include(../ZMDCommon/ZMDCommon.pri)

SOURCES += main.cpp \
    MainWindow.cpp



HEADERS += \
    MainWindow.h

FORMS += \
    MainWindow.ui


