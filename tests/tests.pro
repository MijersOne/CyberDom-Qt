QT += core gui widgets multimedia network testlib
CONFIG += console c++17 testcase

include(../CyberDom.pro)

SOURCES -= main.cpp

SOURCES += signininterval_test.cpp \
           session_test.cpp

TARGET = cyberdom_tests
