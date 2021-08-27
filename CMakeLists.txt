cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 11)

include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.23.312.tar.gz"
    SHA1 "42311324f6b859983d833cffc2151d0b86b8bea4"
)

project(dehash)

if(APPLE)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()

include("cmake/BuildInfo.cmake")

find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIR})

hunter_add_package(Boost COMPONENTS program_options)
find_package(Boost CONFIG REQUIRED program_options)

add_executable(dehash src/main.cpp src/cfg.cpp src/decoder.cpp src/device_pool.cpp src/device.cpp src/device_cl.cpp)
target_link_libraries(dehash ${OpenCL_LIBRARY} Boost::program_options)
