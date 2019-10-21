QT += core
QT -= gui
QT += concurrent

TARGET = zeromqdemo
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

DESTDIR = $$PWD/../../Release/Construction/Bin

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

SOURCES += main.cpp \
    ZeroUtils.cpp

HEADERS += \
    ZeroUtils.h \
    zhelpers.h

