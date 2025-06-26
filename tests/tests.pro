QT += core gui widgets multimedia testlib
CONFIG += c++17 console

# Ensure this project is processed with Qt 6's qmake tool
!equals(QT_MAJOR_VERSION, 6) {
    error("tests.pro requires qmake6 from Qt 6. Please install the qmake6 package and invoke qmake6.")
}

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
    ../linewriter.cpp \
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
    ../deleteassignments.h \
    ../linewriter.h

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
    ../deleteassignments.ui \
    ../linewriter.ui

TARGET = runtests

