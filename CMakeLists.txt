cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 11)

include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.23.312.tar.gz"
    SHA1 "42311324f6b859983d833cffc2151d0b86b8bea4"
)

project(dehash)

option(USE_OPENCL "Build with OpenCL" ON)
option(USE_CUDA "Build with CUDA" OFF)

if (USE_OPENCL)
    add_definitions(-DUSE_OPENCL)
    find_package(OpenCL REQUIRED)
    include_directories(${OpenCL_INCLUDE_DIR})
    add_subdirectory(src/opencl)
endif()

if(APPLE)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()

include("cmake/BuildInfo.cmake")


hunter_add_package(Boost COMPONENTS program_options)
find_package(Boost CONFIG REQUIRED program_options)

file(GLOB sources "src/*.cpp")
add_executable(dehash ${sources})
target_link_libraries(dehash Boost::program_options)
if (USE_OPENCL)
    target_link_libraries(dehash device_cl)
endif()

