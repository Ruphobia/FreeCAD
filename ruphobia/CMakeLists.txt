cmake_minimum_required(VERSION 3.20)
project(Ruphobia CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGLWidgets)
find_package(OpenCASCADE REQUIRED)

add_executable(ruphobia main.cpp)

target_link_libraries(ruphobia
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGLWidgets
)
