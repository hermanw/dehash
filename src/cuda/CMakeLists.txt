include_directories(..)
include_directories(../algorithm)

add_library(device_cu STATIC device_cu.cu enumerator_cu.cu)
target_compile_options(device_cu PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:
    -gencode=arch=compute_61,code=sm_61
    -gencode=arch=compute_75,code=sm_75
    >)
