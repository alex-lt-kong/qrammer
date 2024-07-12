cmake_minimum_required(VERSION 3.16)
project(Qramming VERSION 7.3.19.0918 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Multimedia Network Sql Widgets)
find_package(spdlog CONFIG REQUIRED)

qt_standard_project_setup()

qt_add_executable(qramming WIN32 MACOSX_BUNDLE
    src/qramming/bmbox.cpp     src/qramming/bmbox.h     src/qramming/bmbox.ui
        src/qramming/downloader.cpp     src/qramming/downloader.h
        src/qramming/main.cpp
        src/qramming/window_overview.cpp     src/qramming/window_overview.h     src/qramming/window_overview.ui
        src/qramming/msgbox.cpp     src/qramming/msgbox.h     src/qramming/msgbox.ui
        src/qramming/window_cram.cpp     src/qramming/window_cram.h     src/qramming/window_cram.ui
        src/qramming/snapshot.cpp     src/qramming/snapshot.h
)
target_compile_definitions(qramming PRIVATE
    QT_DEPRECATED_WARNINGS
)

target_link_libraries(qramming PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Network
    Qt::Sql
    Qt::Widgets
    spdlog::spdlog
)


# Resources:
set(res_resource_files
    "src/qramming/Main.ico"
)

qt_add_resources(qramming "res"
    PREFIX
        "/"
    FILES
        ${res_resource_files}
)
set(res1_resource_files

)

qt_add_resources(qramming "res1"
    PREFIX
        "/fonts"
    FILES
        ${res1_resource_files}
)

install(TARGETS qramming
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET qramming
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
