QT += core gui widgets multimedia testlib
CONFIG += c++17 console

INCLUDEPATH += ..

SOURCES += \
    ../ScriptUtils.cpp \
    ../about.cpp \
    ../addclothing.cpp \
    ../addclothtype.cpp \
    ../askclothing.cpp \
    ../askinstructions.cpp \
    ../assignments.cpp \
    ../clothingitem.cpp \
    ../deleteassignments.cpp \
    ../cyberdom.cpp \
    ../questiondialog.cpp \
    ../reportclothing.cpp \
    ../rules.cpp \
    ../scriptparser.cpp \
    ../selectpunishment.cpp \
    ../setflags.cpp \
    ../timeadd.cpp \
    ../listflags.cpp \
    ../joblaunch.cpp \
    ../selectpopup.cpp \
    ../changemerits.cpp \
    ../changestatus.cpp \
    ../askpunishment.cpp \
    punishmenttest.cpp

HEADERS += \
    ../ScriptData.h \
    ../ScriptUtils.h \
    ../about.h \
    ../addclothing.h \
    ../addclothtype.h \
    ../askclothing.h \
    ../askinstructions.h \
    ../assignments.h \
    ../clothingitem.h \
    ../cyberdom.h \
    ../questiondialog.h \
    ../reportclothing.h \
    ../rules.h \
    ../scriptparser.h \
    ../timeadd.h \
    ../askpunishment.h \
    ../changemerits.h \
    ../changestatus.h \
    ../joblaunch.h \
    ../selectpunishment.h \
    ../selectpopup.h \
    ../listflags.h \
    ../setflags.h \
    ../deleteassignments.h

FORMS += \
    ../about.ui \
    ../addclothing.ui \
    ../addclothtype.ui \
    ../askclothing.ui \
    ../askinstructions.ui \
    ../assignments.ui \
    ../cyberdom.ui \
    ../questiondialog.ui \
    ../reportclothing.ui \
    ../rules.ui \
    ../timeadd.ui \
    ../askpunishment.ui \
    ../changemerits.ui \
    ../changestatus.ui \
    ../joblaunch.ui \
    ../selectpunishment.ui \
    ../selectpopup.ui \
    ../listflags.ui \
    ../setflags.ui \
    ../deleteassignments.ui

TARGET = runtests

