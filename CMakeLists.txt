cmake_minimum_required(VERSION 3.15)

project(qnx_shm_ LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/shm_region.cpp
    src/publisher.cpp
    src/subscriber.cpp
)

# Build the static library (QNX will generate libqnx_shm_.a)
add_library(qnx_shm_ STATIC ${SOURCES})

# Optionally specify the library output directory
set_target_properties(qnx_shm_ PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib
)
