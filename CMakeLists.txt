cmake_minimum_required(VERSION 3.28)

option(BUILD_TOOLS "Build the Collector and Mock" ON)

# features that are installed via vcpkg
if (BUILD_TOOLS)
    list(APPEND VCPKG_MANIFEST_FEATURES "build-tools")
endif ()

project(ArcpdsMockDummy)

# add project for xevtc (export xevtc.h via vcpkg)
add_subdirectory(xevtc)

if (BUILD_TOOLS)
    # add project for log
    add_subdirectory(log)

    add_subdirectory(arcdps_mock)
    add_subdirectory(arcdps_collector)
endif ()

include(library_target.cmake)
