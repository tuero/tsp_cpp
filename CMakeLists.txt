cmake_minimum_required (VERSION 3.12)

project(tsp)

# Sources
set(TSP_SOURCES
    src/definitions.h
    src/tsp_base.cpp 
    src/tsp_base.h 
)

# Build library
add_library(tsp STATIC ${TSP_SOURCES})
target_compile_features(tsp PUBLIC cxx_std_17)
target_include_directories(tsp PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# Build tests
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    option(BUILD_TESTS "Build the unit tests" OFF)
    if (${BUILD_TESTS})
        enable_testing()
        add_subdirectory(test)
    endif()
endif()
