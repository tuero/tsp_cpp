cmake_minimum_required (VERSION 3.12)

project(tsp)

include(FetchContent)

# Required for python modules
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Pybind11
message("Configuring Pybind11")
FetchContent_Declare(pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    GIT_TAG master
    SYSTEM
)
FetchContent_MakeAvailable(pybind11)

# Sources
set(TSP_SOURCES
    src/definitions.h
    src/tsp_base.cpp 
    src/tsp_base.h 
)

# CPP library
add_library(tsp STATIC ${TSP_SOURCES})
target_compile_features(tsp PUBLIC cxx_std_17)
target_include_directories(tsp PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# Python module
pybind11_add_module(pytsp EXCLUDE_FROM_ALL python/pytsp.cpp)
target_link_libraries(pytsp PRIVATE tsp)
install(TARGETS pytsp DESTINATION .)

# Build tests
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    option(BUILD_TESTS "Build the unit tests" OFF)
    if (${BUILD_TESTS})
        enable_testing()
        add_subdirectory(test)
    endif()
endif()
