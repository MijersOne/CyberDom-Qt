QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += console

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    about.cpp \
    addclothing.cpp \
    addclothtype.cpp \
    askclothing.cpp \
    askinstructions.cpp \
    assignments.cpp \
    clothingitem.cpp \
    deleteassignments.cpp \
    main.cpp \
    cyberdom.cpp \
    reportclothing.cpp \
    rules.cpp \
    scriptparser.cpp \
    selectpunishment.cpp \
    setflags.cpp \
    timeadd.cpp \
    listflags.cpp \
    joblaunch.cpp \
    selectpopup.cpp \
    changemerits.cpp \
    changestatus.cpp \
    askpunishment.cpp

HEADERS += \
    about.h \
    addclothing.h \
    addclothtype.h \
    askclothing.h \
    askinstructions.h \
    assignments.h \
    clothingitem.h \
    cyberdom.h \
    reportclothing.h \
    rules.h \
    scriptparser.h \
    timeadd.h \
    askpunishment.h \
    changemerits.h \
    changestatus.h \
    joblaunch.h \
    selectpunishment.h \
    selectpopup.h \
    listflags.h \
    setflags.h \
    deleteassignments.h

FORMS += \
    about.ui \
    addclothing.ui \
    addclothtype.ui \
    askclothing.ui \
    askinstructions.ui \
    assignments.ui \
    cyberdom.ui \
    reportclothing.ui \
    rules.ui \
    timeadd.ui \
    askpunishment.ui \
    changemerits.ui \
    changestatus.ui \
    joblaunch.ui \
    selectpunishment.ui \
    selectpopup.ui \
    listflags.ui \
    setflags.ui \
    deleteassignments.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
