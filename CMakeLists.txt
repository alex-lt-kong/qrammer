cmake_minimum_required(VERSION 3.16)
project(Qrammer VERSION 7.3.19.0918 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Gui Multimedia Network Sql Widgets Charts)
find_package(spdlog CONFIG REQUIRED)

include(cmake/compiler-flags.cmake)
include(cmake/sanitizers.cmake)
include(cmake/git.cmake)

qt_standard_project_setup()

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/resources/res.win.rc")

qt_add_executable(qrammer WIN32 MACOSX_BUNDLE
    src/qrammer/main.cpp

    src/qrammer/window/overview.cpp
    src/qrammer/window/overview.h
    src/qrammer/window/overview.ui

    src/qrammer/window/cramming_reminder.cpp
    src/qrammer/window/cramming_reminder.h
    src/qrammer/window/cramming_reminder.ui

    src/qrammer/window/cramming.cpp
    src/qrammer/window/cramming.h
    src/qrammer/window/cramming.ui

    src/qrammer/window/manage_db.cpp
    src/qrammer/window/manage_db.h
    src/qrammer/window/manage_db.ui


    src/qrammer/db.h                  src/qrammer/db.cpp
    src/qrammer/snapshot.h            src/qrammer/snapshot.cpp
    src/qrammer/global_variables.h    src/qrammer/global_variables.cpp

    src/qrammer/utils.h               src/qrammer/utils.cpp

    resources/res.qrc

    cmake/compiler-flags.cmake
    cmake/sanitizers.cmake
    cmake/git.cmake

    ./src/qrammer/dto/knowledge_unit.h
    ./src/qrammer/dto/category.h



    ${app_icon_resource_windows}
)

target_compile_definitions(qrammer PRIVATE
    QT_DEPRECATED_WARNINGS
    "-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\""
)

target_link_libraries(qrammer PRIVATE
    Qt::Charts
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Network
    Qt6::Sql
    Qt::Widgets
    spdlog::spdlog
)
