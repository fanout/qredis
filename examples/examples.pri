QT -= gui
QT += network

INCLUDEPATH += $$PWD/../src
LIBS += -L$$PWD/../src -lqredis
PRE_TARGETDEPS += $$PWD/../src/libqredis.a

exists($$PWD/../conf.pri):include($$PWD/../conf.pri)
