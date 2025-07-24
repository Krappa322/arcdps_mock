cmake_minimum_required(VERSION 3.28)

project(ArcdpsMockProgram CXX)

add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)

find_package(magic_enum CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(${PROJECT_NAME})

# Use -MT / -MTd runtime library
set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_compile_definitions(
        ${PROJECT_NAME}
        PUBLIC
        NOMINMAX
        MAGIC_ENUM_RANGE_MAX=256
)

# add general sources
target_sources(${PROJECT_NAME} PUBLIC
        FILE_SET HEADERS
        FILES
        CombatMock.h
)

target_sources(${PROJECT_NAME}
        PRIVATE
        main.cpp
        CombatMock.cpp
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_link_libraries(${PROJECT_NAME} PUBLIC magic_enum::magic_enum)
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PUBLIC imgui::imgui)
target_link_libraries(${PROJECT_NAME} PUBLIC Version.lib d3d11.lib Winmm.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE ArcdpsMock)
target_link_libraries(${PROJECT_NAME} PRIVATE ArcdpsMockLog)
