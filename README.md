# tsp_cpp

A C++ implementation of the TSP environment.

## Include to Your Project: CMake

### FetchContent
```shell
include(FetchContent)
# ...
FetchContent_Declare(tsp
    GIT_REPOSITORY https://github.com/tuero/tsp_cpp.git
    GIT_TAG master
)

# make available
FetchContent_MakeAvailable(tsp)
link_libraries(tsp)
```

### Git Submodules
```shell
# assumes project is cloned into external/tsp_cpp
add_subdirectory(external/tsp_cpp)
link_libraries(tsp)
```

