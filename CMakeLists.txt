cmake_minimum_required(VERSION 3.28)

option(BUILD_TOOLS "Build the Collector and Mock" OFF)

# features that are installed via vcpkg
if (BUILD_TOOLS)
    list(APPEND VCPKG_MANIFEST_FEATURES "build-tools")
endif ()

include(FetchContent)
FetchContent_Declare(my-vcpkg-triplets
        GIT_REPOSITORY "https://github.com/Zinn-o-Matics/my-vcpkg-triplets.git"
        GIT_TAG "master"
)
FetchContent_MakeAvailable(my-vcpkg-triplets)

list(APPEND VCPKG_OVERLAY_TRIPLETS "${my-vcpkg-triplets_SOURCE_DIR}")

project(ArcpdsMockDummy CXX)

# add project for xevtc (export xevtc.h via vcpkg)
add_subdirectory(xevtc)

if (BUILD_TOOLS)
    # add project for log
    add_subdirectory(log)

    add_subdirectory(arcdps_mock)
    add_subdirectory(arcdps_collector)
endif ()

include(library_target.cmake)
