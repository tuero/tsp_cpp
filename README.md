# tsp_cpp

A C++ implementation of the TSP environment.

## Include to Your Project: CMake

### VCPKG
`tsp_cpp` is not part of the official registry for vcpkg,
but is supported in my personal registry [here](https://github.com/tuero/vcpkg-registry).
To add `tuero/vcpkg-registry` as a git registry to your vcpkg project:
```json
"registries": [
...
{
    "kind": "git",
    "repository": "https://github.com/tuero/vcpkg-registry",
    "reference": "master",
    "baseline": "<COMMIT_SHA>",
    "packages": ["tsp"]
}
]
...
```
where `<COMMIT_SHA>` is the 40-character git commit sha in the registry's repository (you can find 
this by clicking on the latest commit [here](https://github.com/tuero/vcpkg-registry) and looking 
at the URL.


Then in your project cmake:
```cmake
cmake_minimum_required(VERSION 3.25)
project(my_project LANGUAGES CXX)

find_package(tsp CONFIG REQUIRED)
add_executable(main main.cpp)
target_link_libraries(main PRIVATE tsp::tsp)
```

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

```shell
python scenario_create.py --export_path=. --num_train=20000 --num_test=100 --map_size=10 --num_cities=12 --add_walls
```
