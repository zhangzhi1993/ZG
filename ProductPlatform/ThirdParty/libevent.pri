
INCLUDEPATH += \
    $$ThirdParty/libevent/include \
    $$ThirdParty/libevent/include/event2

CONFIG(debug, debug|release) {
    contains(QMAKE_HOST.arch, x86_64) {
        LIBS += -L$$ThirdParty/libevent/lib
    } else {
        LIBS += -L$$ThirdParty/libevent/lib
    }

    win32:LIBS += -llibevent -llibevent_core -llibevent_extras
}

CONFIG(release, debug|release) {
    contains(QMAKE_HOST.arch, x86_64) {
        LIBS += -L$$ThirdParty/libevent/lib
    } else {
        LIBS += -L$$ThirdParty/libevent/lib
    }

    win32:LIBS += -llibevent -llibevent_core -llibevent_extras
}


