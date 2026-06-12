cmake_minimum_required(VERSION 3.16)
project(tomogichi-qt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2)
find_package(KF6Kirigami REQUIRED)

# CLI engine code (no Qt deps)
set(ENGINE_DIR "${CMAKE_SOURCE_DIR}/../src")
set(ENGINE_SOURCES
    ${ENGINE_DIR}/core/engine.cpp
    ${ENGINE_DIR}/data/storage.cpp
)

set(SOURCES
    src/main.cpp
    src/backend.cpp
    ${ENGINE_SOURCES}
)

set(HEADERS
    src/backend.h
    ${ENGINE_DIR}/core/engine.h
    ${ENGINE_DIR}/data/storage.h
    ${ENGINE_DIR}/core/types.h
    ${ENGINE_DIR}/json.hpp
)

qt_add_executable(tomogichi-qt ${SOURCES} ${HEADERS})

target_include_directories(tomogichi-qt PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${ENGINE_DIR}
)

target_link_libraries(tomogichi-qt PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    KF6::Kirigami
)

# QML files
qt_add_qml_module(tomogichi-qt
    URI "org.tomogichi.qt"
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/main_minimal.qml
        qml/TestWindow.qml
        qml/GuildPage.qml
        qml/PersonPage.qml
        qml/TimerPage.qml
        qml/StatsPage.qml
        qml/TodayPage.qml
        qml/SettingsPage.qml
        qml/DiaryPage.qml
)

# Install
install(TARGETS tomogichi-qt DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES org.tomogichi.qt.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES ../CHANGELOG.md DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/tomogichi RENAME changelog.md)
