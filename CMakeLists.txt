cmake_minimum_required(VERSION 3.16)
project(Qrammer VERSION 7.3.19.0918 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Multimedia Network Sql Widgets)
find_package(spdlog CONFIG REQUIRED)

qt_standard_project_setup()

qt_add_executable(qrammer WIN32 MACOSX_BUNDLE
    src/qrammer/bmbox.cpp     src/qrammer/bmbox.h     src/qrammer/bmbox.ui
        src/qrammer/downloader.cpp     src/qrammer/downloader.h
        src/qrammer/main.cpp
        src/qrammer/window_overview.cpp     src/qrammer/window_overview.h     src/qrammer/window_overview.ui
        src/qrammer/msgbox.cpp     src/qrammer/msgbox.h     src/qrammer/msgbox.ui
        src/qrammer/window_cram.cpp     src/qrammer/window_cram.h     src/qrammer/window_cram.ui
        src/qrammer/snapshot.cpp     src/qrammer/snapshot.h
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


# Resources:
set(res_resource_files
    "src/qrammer/Main.ico"
)

qt_add_resources(qrammer "res"
    PREFIX
        "/"
    FILES
        ${res_resource_files}
)
set(res1_resource_files

)

qt_add_resources(qrammer "res1"
    PREFIX
        "/fonts"
    FILES
        ${res1_resource_files}
)

install(TARGETS qrammer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET qrammer
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
