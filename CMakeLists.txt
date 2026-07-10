cmake_minimum_required(VERSION 3.10)
project(CVFS VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ─── Core library (no Qt dependency) ───────────────────────────────────
add_library(cvfs_core STATIC
    CVFS.cpp
    persistence.cpp
    cvfs_wrapper.cpp
    directory.cpp
)

target_include_directories(cvfs_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ─── Original standalone build (original code, no persistence) ─────────
add_executable(cvfs_original
    main_original.cpp
    CVFS.cpp
)
target_compile_definitions(cvfs_original PRIVATE CVFS_STANDALONE)

# ─── CLI version with persistence ──────────────────────────────────────
add_executable(cvfs_cli
    main_cli.cpp
)
target_link_libraries(cvfs_cli cvfs_core)

option(CVFS_BUILD_GUI "Build the Qt GUI version" ON)

# ─── GUI version (requires Qt) ─────────────────────────────────────────
if(CVFS_BUILD_GUI)
find_package(Qt6 COMPONENTS Widgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 COMPONENTS Widgets QUIET)
endif()

if(Qt6_FOUND OR Qt5_FOUND)
    set(QT_LIB Qt5::Widgets)
    if(Qt6_FOUND)
        set(QT_LIB Qt6::Widgets)
    endif()

    set(GUI_SOURCES
        main_gui.cpp
        gui/mainwindow.cpp
        gui/filemanagerpanel.cpp
        gui/visualizationpanel.cpp
        gui/visualizationengine.cpp
        gui/filesystemmodel.cpp
        gui/filecontentdialog.cpp
        gui/filepropertiesdialog.cpp
        gui/flowchartwidget.cpp
        gui/systeminfowidget.cpp
    )

    set(GUI_HEADERS
        gui/mainwindow.h
        gui/filemanagerpanel.h
        gui/visualizationpanel.h
        gui/visualizationengine.h
        gui/filesystemmodel.h
        gui/filecontentdialog.h
        gui/filepropertiesdialog.h
        gui/flowchartwidget.h
        gui/systeminfowidget.h
    )

    add_executable(cvfs_gui ${GUI_SOURCES} ${GUI_HEADERS})
    target_link_libraries(cvfs_gui cvfs_core ${QT_LIB})

    set_target_properties(cvfs_gui PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
    )

    message(STATUS "Qt found — building GUI version 'cvfs_gui'")
else()
    message(STATUS "Qt not found — skipping GUI build. Install Qt6 or Qt5.15+ for the GUI.")
endif()
endif()

# ─── Install rules ─────────────────────────────────────────────────────
install(TARGETS cvfs_cli cvfs_original DESTINATION bin)
if(TARGET cvfs_gui)
    install(TARGETS cvfs_gui DESTINATION bin)
endif()
