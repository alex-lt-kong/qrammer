cmake_minimum_required(VERSION 3.16)
project(qjlt VERSION 7.3.19.0918 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Multimedia Network Sql Widgets)

qt_standard_project_setup()

qt_add_executable(qjlt WIN32 MACOSX_BUNDLE
    bmbox.cpp bmbox.h bmbox.ui
    downloader.cpp downloader.h
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    msgbox.cpp msgbox.h msgbox.ui
    practicewindow.cpp practicewindow.h practicewindow.ui
    snapshot.cpp snapshot.h
)
target_compile_definitions(qjlt PRIVATE
    QT_DEPRECATED_WARNINGS
)

target_link_libraries(qjlt PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Network
    Qt::Sql
    Qt::Widgets
)


# Resources:
set(res_resource_files
    "Main.ico"
)

qt_add_resources(qjlt "res"
    PREFIX
        "/"
    FILES
        ${res_resource_files}
)
set(res1_resource_files

)

qt_add_resources(qjlt "res1"
    PREFIX
        "/fonts"
    FILES
        ${res1_resource_files}
)

install(TARGETS qjlt
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET qjlt
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
