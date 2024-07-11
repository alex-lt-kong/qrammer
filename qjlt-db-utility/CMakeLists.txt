cmake_minimum_required(VERSION 3.16)
project(qjlt-db-utility VERSION 7.3.19.0918 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Sql)
find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(qjlt-db-utility WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
)
target_compile_definitions(qjlt-db-utility PRIVATE
    QT_DEPRECATED_WARNINGS
)

target_link_libraries(qjlt-db-utility PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Sql
)


# Resources:
set(res_resource_files
    "Utility.ico"
)

qt_add_resources(qjlt-db-utility "res"
    PREFIX
        "/"
    FILES
        ${res_resource_files}
)

if((QT_VERSION_MAJOR GREATER 4))
    target_link_libraries(qjlt-db-utility PRIVATE
        Qt::Widgets
    )
endif()

install(TARGETS qjlt-db-utility
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET qjlt-db-utility
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
