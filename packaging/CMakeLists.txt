include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if (NOT DEFINED tsp_INSTALL_CMAKEDIR)
   set(tsp_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/tsp"
       CACHE STRING "Path to tsp CMake files")
endif ()

install(TARGETS tsp EXPORT tsp_Targets
        RUNTIME COMPONENT tsp_Runtime
        LIBRARY COMPONENT tsp_Runtime
        NAMELINK_COMPONENT tsp_Development
        ARCHIVE COMPONENT tsp_Development
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(DIRECTORY "${tsp_SOURCE_DIR}/include/" 
        TYPE INCLUDE
        COMPONENT tsp_Development)

if (BUILD_SHARED_LIBS)
    set(type shared)
    message(STATUS "Building tsp shared lib")
else ()
    set(type static)
    message(STATUS "Building tsp static lib")
endif ()

install(EXPORT tsp_Targets
        DESTINATION "${tsp_INSTALL_CMAKEDIR}"
        NAMESPACE tsp::
        FILE tsp-${type}-targets.cmake
        COMPONENT tsp_Development)

write_basic_package_version_file(
    tspConfigVersion.cmake
    COMPATIBILITY SameMajorVersion)

install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/tspConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/tspConfigVersion.cmake"
        DESTINATION "${tsp_INSTALL_CMAKEDIR}"
        COMPONENT tsp_Development)
