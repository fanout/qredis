CONFIG *= console qtestlib testcase
CONFIG -= app_bundle
QT -= gui
QT *= network

TESTS_DIR = $$PWD
SRC_DIR = $$PWD/../src
DESTDIR = $$TESTS_DIR

INCLUDEPATH += $$SRC_DIR
LIBS += -L$$SRC_DIR -lqredis
PRE_TARGETDEPS += $$PWD/../src/libqredis.a

exists($$PWD/../conf.pri):include($$PWD/../conf.pri)
