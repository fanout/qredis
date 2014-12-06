TEMPLATE = lib
CONFIG -= app_bundle
CONFIG += staticlib
QT -= gui
QT += network
TARGET = qredis

MOC_DIR = $$OUT_PWD/_moc
OBJECTS_DIR = $$OUT_PWD/_obj

exists($$OUT_PWD/../conf.pri):include($$OUT_PWD/../conf.pri)
include(src.pri)

LIBS += -lhiredis
CONFIG *= create_prl
