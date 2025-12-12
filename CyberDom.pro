QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += console

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$PWD/src/Core \
               $$PWD/src/Gui \
               $$PWD/src/Widgets \
               $$PWD/src/Dialogs/Clothing \
               $$PWD/src/Dialogs/Tasks \
               $$PWD/src/Dialogs/Management \
               $$PWD/src/Dialogs/General

SOURCES += \
    src/Core/ScriptUtils.cpp \
    src/Core/clothingitem.cpp \
    src/Core/main.cpp \
    src/Core/scriptparser.cpp \
    src/Dialogs/Clothing/addclothing.cpp \
    src/Dialogs/Clothing/addclothtype.cpp \
    src/Dialogs/Clothing/askclothing.cpp \
    src/Dialogs/Clothing/reportclothing.cpp \
    src/Dialogs/General/about.cpp \
    src/Dialogs/General/askinstructions.cpp \
    src/Dialogs/General/datainspectordialog.cpp \
    src/Dialogs/General/rules.cpp \
    src/Dialogs/General/timeadd.cpp \
    src/Dialogs/Management/askpunishment.cpp \
    src/Dialogs/Management/assignments.cpp \
    src/Dialogs/Management/changemerits.cpp \
    src/Dialogs/Management/changestatus.cpp \
    src/Dialogs/Management/deleteassignments.cpp \
    src/Dialogs/Management/joblaunch.cpp \
    src/Dialogs/Management/listflags.cpp \
    src/Dialogs/Management/newassignmentpopup.cpp \
    src/Dialogs/Management/selectpopup.cpp \
    src/Dialogs/Management/selectpunishment.cpp \
    src/Dialogs/Management/setflags.cpp \
    src/Dialogs/Tasks/detention.cpp \
    src/Dialogs/Tasks/linewriter.cpp \
    src/Dialogs/Tasks/pointcameradialog.cpp \
    src/Dialogs/Tasks/posecameradialog.cpp \
    src/Dialogs/Tasks/questiondialog.cpp \
    src/Gui/cyberdom.cpp \
    src/Gui/mainwindow.cpp \
    src/Widgets/calendarview.cpp \
    src/Widgets/eventcalendarwidget.cpp \
    src/Widgets/googlecalendarwidget.cpp

HEADERS += \
    src/Core/ScriptData.h \
    src/Core/ScriptUtils.h \
    src/Core/clothingitem.h \
    src/Core/scriptparser.h \
    src/Dialogs/Clothing/addclothing.h \
    src/Dialogs/Clothing/addclothtype.h \
    src/Dialogs/Clothing/askclothing.h \
    src/Dialogs/Clothing/reportclothing.h \
    src/Dialogs/General/about.h \
    src/Dialogs/General/askinstructions.h \
    src/Dialogs/General/datainspectordialog.h \
    src/Dialogs/General/rules.h \
    src/Dialogs/General/timeadd.h \
    src/Dialogs/Management/askpunishment.h \
    src/Dialogs/Management/assignments.h \
    src/Dialogs/Management/changemerits.h \
    src/Dialogs/Management/changestatus.h \
    src/Dialogs/Management/deleteassignments.h \
    src/Dialogs/Management/joblaunch.h \
    src/Dialogs/Management/listflags.h \
    src/Dialogs/Management/newassignmentpopup.h \
    src/Dialogs/Management/selectpopup.h \
    src/Dialogs/Management/selectpunishment.h \
    src/Dialogs/Management/setflags.h \
    src/Dialogs/Tasks/detention.h \
    src/Dialogs/Tasks/linewriter.h \
    src/Dialogs/Tasks/pointcameradialog.h \
    src/Dialogs/Tasks/posecameradialog.h \
    src/Dialogs/Tasks/questiondialog.h \
    src/Gui/cyberdom.h \
    src/Gui/mainwindow.h \
    src/Widgets/calendarview.h \
    src/Widgets/eventcalendarwidget.h \
    src/Widgets/googlecalendarwidget.h

FORMS += \
    src/Dialogs/Clothing/addclothing.ui \
    src/Dialogs/Clothing/addclothtype.ui \
    src/Dialogs/Clothing/askclothing.ui \
    src/Dialogs/Clothing/reportclothing.ui \
    src/Dialogs/General/about.ui \
    src/Dialogs/General/askinstructions.ui \
    src/Dialogs/General/datainspectordialog.ui \
    src/Dialogs/General/rules.ui \
    src/Dialogs/General/timeadd.ui \
    src/Dialogs/Management/askpunishment.ui \
    src/Dialogs/Management/assignments.ui \
    src/Dialogs/Management/changemerits.ui \
    src/Dialogs/Management/changestatus.ui \
    src/Dialogs/Management/deleteassignments.ui \
    src/Dialogs/Management/joblaunch.ui \
    src/Dialogs/Management/listflags.ui \
    src/Dialogs/Management/newassignmentpopup.ui \
    src/Dialogs/Management/selectpopup.ui \
    src/Dialogs/Management/selectpunishment.ui \
    src/Dialogs/Management/setflags.ui \
    src/Dialogs/Tasks/detention.ui \
    src/Dialogs/Tasks/linewriter.ui \
    src/Dialogs/Tasks/pointcameradialog.ui \
    src/Dialogs/Tasks/posecameradialog.ui \
    src/Dialogs/Tasks/questiondialog.ui \
    src/Gui/cyberdom.ui \
    src/Gui/mainwindow.ui \
    src/Widgets/calendarview.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Unit test target
unit_test.target = runtests
unit_test.commands = cd tests && qmake6 tests.pro && $(MAKE)
QMAKE_EXTRA_TARGETS += unit_test
