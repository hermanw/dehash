find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIR})
include_directories(..)

add_library(device_cl STATIC device_cl.cpp enumerator_cl.cpp)
target_link_libraries(device_cl ${OpenCL_LIBRARY})
