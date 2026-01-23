cmake_minimum_required(VERSION 3.20)
project(Ruphobia CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGLWidgets SvgWidgets)
find_package(OpenCASCADE REQUIRED)
find_package(libzip REQUIRED)

add_custom_target(build_number ALL
    ${CMAKE_COMMAND}
        -DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DBIN_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/increment_build.cmake
)

add_executable(ruphobia main.cpp resources.qrc)
add_dependencies(ruphobia build_number)
target_include_directories(ruphobia PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(ruphobia
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::SvgWidgets
    libzip::zip
)
