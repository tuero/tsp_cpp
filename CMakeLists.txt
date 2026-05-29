cmake_minimum_required (VERSION 3.23)

project(tsp VERSION 1.0.4)

include(FetchContent)

option(BUILD_PYTHON_MODULE "Build the python library wrapper" OFF)


# Let tsp_SHARED_LIBS override BUILD_SHARED_LIBS
if(DEFINED tsp_SHARED_LIBS)
    set(BUILD_SHARED_LIBS "${tsp_SHARED_LIBS}")
endif()


# Required for python modules
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# Sources
set(TSP_SOURCES
    include/tsp/definitions.h
    include/tsp/tsp.h
    include/tsp/tsp_base.h
    src/tsp_base.cpp 
)

# CPP library
add_library(tsp)
add_library(tsp::tsp ALIAS tsp)
set_target_properties(tsp PROPERTIES
                      VERSION ${tsp_VERSION}
                      SOVERSION ${tsp_VERSION_MAJOR}
                      POSITION_INDEPENDENT_CODE ON
)
target_include_directories(
    tsp PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_compile_features(tsp PUBLIC cxx_std_20)
target_sources(tsp PRIVATE ${TSP_SOURCES})


# Include the install rules if the user wanted them (included by default when top-level)
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" is_top_level)
option(tsp_INCLUDE_PACKAGING "Include packaging rules for tsp" "${is_top_level}")
if(tsp_INCLUDE_PACKAGING)
    add_subdirectory(packaging)
endif()


# Pybind11
if(BUILD_PYTHON_MODULE)
    include(FetchContent)
    message("Configuring Pybind11")
    FetchContent_Declare(pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11
        GIT_TAG master
        SYSTEM
    )
    FetchContent_MakeAvailable(pybind11)

    # Python module
    pybind11_add_module(pytsp python/pytsp.cpp)
    target_link_libraries(pytsp PRIVATE tsp)
    install(TARGETS pytsp DESTINATION .)
endif()


# Build tests
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    option(BUILD_TESTS "Build the unit tests" OFF)
    if (${BUILD_TESTS})
        enable_testing()
        add_subdirectory(test)
    endif()
endif()
