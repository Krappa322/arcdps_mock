project(ArcdpsMock CXX)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(ArcdpsExtension CONFIG REQUIRED)

add_library(${PROJECT_NAME} STATIC)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Use -MT / -MTd runtime library
set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# add general sources
target_sources(${PROJECT_NAME} PUBLIC
        FILE_SET HEADERS
        FILES
        xevtc/Xevtc.h
        arcdps_mock/CombatMock.h
)

target_sources(${PROJECT_NAME}
        PRIVATE
        arcdps_mock/CombatMock.cpp
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PUBLIC imgui::imgui)
target_link_libraries(${PROJECT_NAME} PUBLIC magic_enum::magic_enum)
target_link_libraries(${PROJECT_NAME} PUBLIC ArcdpsExtension::ArcdpsExtension)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
        EXPORT ArcdpsMockTargets
        FILE_SET HEADERS
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/ArcdpsMockConfig.cmake
        INSTALL_DESTINATION share/${PROJECT_NAME}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ArcdpsMockConfig.cmake
        DESTINATION share/${PROJECT_NAME}
)

install(EXPORT ArcdpsMockTargets
        NAMESPACE ${PROJECT_NAME}::
        FILE ArcdpsMockTargets.cmake
        DESTINATION share/${PROJECT_NAME}
)
