# dehash (work in progress...)

## Features
* decode using multiple gpu devices in parellel
* support OpenCL/CUDA devices
* support MD5 hash algorithm (//TODO: ...)
* support customisable decode pattern

## Build

### Dependencies
- cmake
- hunter

### Linux
```
cmake -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build
```

### Windows
```
cmake -B build -G "Visual Studio 16 2019" -A x64
cmake --build build --config Release
```
