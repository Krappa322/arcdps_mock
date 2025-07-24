cmake_minimum_required(VERSION 3.28)

project(ArcdpsMockCollector CXX)

add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)

find_package(magic_enum CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(ArcdpsExtension CONFIG REQUIRED)

add_library(${PROJECT_NAME} SHARED)

# Use -MT / -MTd runtime library
set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# add general sources
target_sources(${PROJECT_NAME} PUBLIC
        FILE_SET HEADERS
        FILES
        Collector.h
        GUI.h
)

target_sources(${PROJECT_NAME}
        PRIVATE
        dllmain.cpp
        Collector.cpp
        GUI.cpp
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_link_libraries(${PROJECT_NAME} PUBLIC magic_enum::magic_enum)
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PUBLIC imgui::imgui)
target_link_libraries(${PROJECT_NAME} PUBLIC ArcdpsMock)
target_link_libraries(${PROJECT_NAME} PUBLIC ArcdpsMockLog)
