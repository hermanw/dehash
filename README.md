# dehash

## Features
* decode using multiple gpu devices in parellel
* support OpenCL/CUDA devices
* support MD5 hash algorithm
* support customisable decode pattern

## Build

### Dependencies
- cmake
- hunter

### Linux/Mac
```
cmake -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build
```

### Windows
```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
