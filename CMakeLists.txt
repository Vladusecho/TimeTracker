cmake_minimum_required(VERSION 3.16)
project(TimeTracker VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    database.cpp
    database.h
    taskdialog.cpp
    taskdialog.h
    taskdialog.ui
    editentrydialog.cpp
    editentrydialog.h
    editentrydialog.ui
    authdialog.cpp
    authdialog.h
    authdialog.ui
    user.h
    passwordmanager.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(TimeTracker MANUAL_FINALIZATION ${PROJECT_SOURCES}
        passwordmanager.h
        user.h
        authdialog.h
        authdialog.cpp)
else()
    if(ANDROID)
        add_library(TimeTracker SHARED ${PROJECT_SOURCES})
    else()
        add_executable(TimeTracker ${PROJECT_SOURCES})
    endif()
endif()

target_link_libraries(TimeTracker PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
)

set_target_properties(TimeTracker PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS TimeTracker
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(TimeTracker)
endif()
