cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 11)

include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.24.3.tar.gz"
    SHA1 "10738b59e539818a01090e64c2d09896247530c7"
)

project(dehash)

option(USE_OPENCL "Build with OpenCL" ON)
option(USE_CUDA "Build with CUDA" ON)

if (USE_OPENCL)
    add_definitions(-DUSE_OPENCL)
    add_subdirectory(src/opencl)
endif()
if (USE_CUDA)
    add_definitions(-DUSE_CUDA)
    enable_language(CUDA)
    add_subdirectory(src/cuda)
endif()

if(APPLE)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()

include("cmake/BuildInfo.cmake")


hunter_add_package(Boost COMPONENTS program_options)
find_package(Boost CONFIG REQUIRED program_options)

include_directories(src)
file(GLOB sources "src/*.cpp")
add_executable(dehash ${sources})

target_link_libraries(dehash Boost::program_options)
if (USE_OPENCL)
    target_link_libraries(dehash device_cl)
endif()
if (USE_CUDA)
    target_link_libraries(dehash device_cu)
endif()
