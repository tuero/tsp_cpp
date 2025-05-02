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

## Installing Python Bindings
```shell
git clone https://github.com/tuero/tsp_cpp.git
pip install ./tsp_cpp
```

If you get a `GLIBCXX_3.4.XX not found` error at runtime, 
then you most likely have an older `libstdc++` in your virtual environment `lib/` 
which is taking presidence over your system version.
Either correct your `$PATH`, or update your virtual environment `libstdc++`.

For example, if using anaconda
```shell
conda install conda-forge::libstdcxx-ng
```

## Level Format
Levels are expected to be formatted as `|` delimited strings, where the first 2 entries are the rows/columns of the level,
then the following `rows * cols` entries are the element ID (see `Element` in `definitions.h`).
