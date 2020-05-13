cmake_minimum_required (VERSION 3.1)
project (cmft)

set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/dependency)
include_directories(${PROJECT_SOURCE_DIR}/src/cmft)

add_subdirectory(src)

