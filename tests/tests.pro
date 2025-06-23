QT += core testlib
CONFIG += console c++17 testcase
SOURCES += signininterval_test.cpp \
           ../scriptparser.cpp
HEADERS += ../scriptparser.h \
           ../ScriptData.h
TARGET = signininterval_test
