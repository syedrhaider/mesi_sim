cmake_minimum_required(VERSION 3.10)
project(mesi_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_executable(mesi_sim
    src/main.cpp
    src/config.cpp
    src/memory_system.cpp
    src/coherence.cpp
    src/trace.cpp
    src/stats.cpp
)

target_include_directories(mesi_sim PRIVATE include)

# Copy default config + sample trace next to the built binary so
# `./mesi_sim` works out of the box from the build directory too.
configure_file(${CMAKE_SOURCE_DIR}/sim.cfg ${CMAKE_BINARY_DIR}/sim.cfg COPYONLY)
file(COPY ${CMAKE_SOURCE_DIR}/traces DESTINATION ${CMAKE_BINARY_DIR})
