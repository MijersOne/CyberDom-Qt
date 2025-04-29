cmake_minimum_required(VERSION 3.14)

project(CyberDom LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the required Qt packages
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

# Define source files
set(SOURCES
    main.cpp
    cyberdom.cpp
    addclothing.cpp
    addclothtype.cpp
    about.cpp
    askclothing.cpp
    askinstructions.cpp
    askpunishment.cpp
    changemerits.cpp
    changestatus.cpp
    clothingitem.cpp
    deleteassignments.cpp
    joblaunch.cpp
    listflags.cpp
    mainwindow.cpp
    newassignmentpopup.cpp
    reportclothing.cpp
    rules.cpp
    scriptparser.cpp
    selectpopup.cpp
    selectpunishment.cpp
    setflags.cpp
    timeadd.cpp
    assignments.cpp
)

# Define header files
set(HEADERS
    cyberdom.h
    addclothing.h
    addclothtype.h
    about.h
    askclothing.h
    askinstructions.h
    askpunishment.h
    changemerits.h
    changestatus.h
    clothingitem.h
    deleteassignments.h
    joblaunch.h
    listflags.h
    mainwindow.h
    newassignmentpopup.h
    reportclothing.h
    rules.h
    scriptparser.h
    selectpopup.h
    selectpunishment.h
    setflags.h
    timeadd.h
    assignments.h
)

# Define UI files
set(UIS
    about.ui
    addclothing.ui
    addclothtype.ui
    askclothing.ui
    askinstructions.ui
    askpunishment.ui
    assignments.ui
    changemerits.ui
    changestatus.ui
    cyberdom.ui
    deleteassignments.ui
    joblaunch.ui
    listflags.ui
    mainwindow.ui
    newassignmentpopup.ui
    reportclothing.ui
    rules.ui
    selectpopup.ui
    selectpunishment.ui
    setflags.ui
    timeadd.ui
)

# Create executable
add_executable(CyberDom 
    ${SOURCES}
    ${HEADERS}
    ${UIS}
)

# Link Qt libraries
target_link_libraries(CyberDom PRIVATE 
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

# Handle resources if any
# qt_add_resources(CyberDom "resources.qrc")

# Install directives (optional)
install(TARGETS CyberDom
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)