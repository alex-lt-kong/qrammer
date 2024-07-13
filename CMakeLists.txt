cmake_minimum_required(VERSION 3.16)
project(Qrammer VERSION 7.3.19.0918 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Multimedia Network Sql Widgets)
find_package(spdlog CONFIG REQUIRED)

qt_standard_project_setup()

qt_add_executable(qrammer WIN32 MACOSX_BUNDLE
    src/qrammer/bmbox.cpp
    src/qrammer/bmbox.h
    src/qrammer/bmbox.ui

    src/qrammer/downloader.cpp
    src/qrammer/downloader.h

    src/qrammer/main.cpp
    src/qrammer/window_overview.cpp
    src/qrammer/window_overview.h
    src/qrammer/window_overview.ui

    src/qrammer/msgbox.cpp
    src/qrammer/msgbox.h
    src/qrammer/msgbox.ui

    src/qrammer/window_cramming.cpp
    src/qrammer/window_cramming.h
    src/qrammer/window_cramming.ui

    src/qrammer/snapshot.cpp
    src/qrammer/snapshot.h

    resources/qrammer.ico
    resources/qrammer-db-tool.ico

    src/qrammer/knowledge_unit.h

    src/qrammer/global_variables.h
    src/qrammer/global_variables.cpp
)
target_compile_definitions(qrammer PRIVATE
    QT_DEPRECATED_WARNINGS
)

target_link_libraries(qrammer PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Network
    Qt::Sql
    Qt::Widgets
    spdlog::spdlog
)
