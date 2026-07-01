cmake_minimum_required(VERSION 3.16)
project(tomogichi-qt VERSION 0.7.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(ENGINE_DIR "${CMAKE_SOURCE_DIR}/../src")

set(SOURCES
    src/main.cpp
    src/backend.cpp
    src/widget_gui.cpp
    ${ENGINE_DIR}/core/engine.cpp
    ${ENGINE_DIR}/data/storage.cpp
)

set(HEADERS
    src/backend.h
    src/widget_gui.h
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
    Qt6::Widgets
)
