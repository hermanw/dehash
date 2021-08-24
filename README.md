# dehash (work in progress...)

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
