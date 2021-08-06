cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 11)

include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.23.312.tar.gz"
    SHA1 "42311324f6b859983d833cffc2151d0b86b8bea4"
)

project(dm)

if(APPLE)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()

find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIR})

hunter_add_package(Boost COMPONENTS program_options)
find_package(Boost CONFIG REQUIRED program_options)

add_executable(dm dm.cpp cfg.cpp decoder.cpp kernel.cpp)
target_link_libraries(dm ${OpenCL_LIBRARY} Boost::program_options)
