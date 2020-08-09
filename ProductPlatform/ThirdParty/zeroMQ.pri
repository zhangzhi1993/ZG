
ThirdParty = $$quote($$(ThirdParty))
INCLUDEPATH += \
    $$ThirdParty/ZeroMQ/czmq/include \
    $$ThirdParty/ZeroMQ/libzmq/include \
    $$ThirdParty/ZeroMQ/libzmq/src

#HEADERS += \
#    $$ThirdParty/ZeroMQ/czmq/include/czmq.h \
#    $$ThirdParty/ZeroMQ/czmq/include/zmq.h \
#    $$ThirdParty/ZeroMQ/czmq/include/zmq_utils.h \
#    $$ThirdParty/ZeroMQ/czmq/include/zmq.hpp

CONFIG(debug, debug|release) {
    contains(QMAKE_HOST.arch, x86_64) {
        LIBS += -L$$ThirdParty/ZeroMQ/libzmq/bin/X64/Debug/v100/dynamic
    } else {
        LIBS += -L$$ThirdParty/ZeroMQ/libzmq/bin/Win32/Debug/v100/dynamic
    }

    win32:LIBS += -llibzmq
    unix:LIBS += -llibzmq_debug
}

CONFIG(debug, debug|release) {
    contains(QMAKE_HOST.arch, x86_64) {
        LIBS += -L$$ThirdParty/ZeroMQ/libsodium/bin/X64/Debug/v100/dynamic
    } else {
        LIBS += -L$$ThirdParty/ZeroMQ/libsodium/bin/Win32/Debug/v100/dynamic
    }

    win32:LIBS += -llibsodium
    unix:LIBS += -llibsodium_debug
}

CONFIG(release, debug|release) {
    contains(QMAKE_HOST.arch, x86_64) {
        LIBS += -L$$ThirdParty/ZeroMQ/libzmq/bin/X64/Release/v100/dynamic
    } else {
        LIBS += -L$$ThirdParty/ZeroMQ/libzmq/bin/Win32/Release/v100/dynamic
    }

    LIBS += -llibzmq
}

CONFIG(release, debug|release) {
    contains(QMAKE_HOST.arch, x86_64) {
        LIBS += -L$$ThirdParty/ZeroMQ/libsodium/bin/X64/Release/v100/dynamic
    } else {
        LIBS += -L$$ThirdParty/ZeroMQ/libsodium/bin/Win32/Release/v100/dynamic
    }

    LIBS += -llibsodium
}


