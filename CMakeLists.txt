cmake_minimum_required(VERSION 3.28)

project(ArcpdsMockDummy)

# add project for log
add_subdirectory(log)

# add project for xevtc (export xevtc.h via vcpkg)
add_subdirectory(xevtc)

# then link those to the arcdps_mock project

add_subdirectory(arcdps_mock)
add_subdirectory(arcdps_collector)
