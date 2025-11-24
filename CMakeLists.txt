cmake_minimum_required(VERSION 3.16)
project(bosbase-c LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../cpp-sdk ${CMAKE_CURRENT_BINARY_DIR}/cpp-sdk)

add_library(bosbase_c
    src/bosbase_c.cpp
)

target_include_directories(bosbase_c
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../cpp-sdk/include
)

target_link_libraries(bosbase_c PUBLIC bosbase)
