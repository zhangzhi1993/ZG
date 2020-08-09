#-------------------------------------------------
#
# Project created by QtCreator 2019-10-21T15:08:42
#
#-------------------------------------------------

QT       += core gui network
CONFIG += console
CONFIG -= app_bundle

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ZMDClientProxy
TEMPLATE = app
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

DEFINES += WIN32_LEAN_AND_MEAN

QMAKE_CXXFLAGS_RELEASE += /NODEFAULTLIB:"libcmt.lib"
QMAKE_LFLAGS_RELEASE += /DEBUG

include($(GLDRS)/Glodon/shared/zeroMQ.pri)
include($(GLDRS)/Glodon/shared/libevent.pri)
include($(GLDRS)/Glodon/shared/GEPEngine.pri)
include($(GLDRS)/Glodon/shared/GLDStaticLib.pri)
include($(GLDRS)/Glodon/shared/GSPStaticLib.pri)
include($(GLDRS)/Glodon/shared/VLD.pri)
include($(GLDRS)/Glodon/shared/GSCRSARefGLDStaticLib.pri)
include($(GLDRS)/Glodon/shared/QtitanRibbon.pri)
include($(GLDRS)/Glodon/shared/GLDXLS.pri)
include(../ZMDCommon/ZMDCommon.pri)

SOURCES += \
        main.cpp \
        MainWindow.cpp

HEADERS += \
        MainWindow.h

FORMS += \
        MainWindow.ui
