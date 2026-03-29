cmake_minimum_required(VERSION 3.11)

project(memcc
    VERSION 0.1.1
    LANGUAGES C
)

# Generate compile_commands.json for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# INTERFACE library (header-only)
add_library(memcc INTERFACE
)
add_library(memcc::memcc ALIAS memcc)

# Set version properties
set_target_properties(
    memcc PROPERTIES
        VERSION     ${PROJECT_VERSION}
        SOVERSION   ${PROJECT_VERSION_MAJOR}
)


# C standard
target_compile_features(memcc INTERFACE c_std_11)

# Include directories
target_include_directories(memcc
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ---- Install configuration ----
include(GNUInstallDirs)

install(
    TARGETS memcc
    EXPORT memccTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# ---- CMake package config ----
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "memccConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/memccConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/memccConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/memcc
)

install(
    EXPORT memccTargets
    FILE memccTargets.cmake
    NAMESPACE memcc::
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/memcc
)

if(memcc_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Optional macros can be added per-target in the consuming project
# Example: target_compile_definitions(myapp PRIVATE MEMCC_DEBUG MEMCC_ZERO_ON_ALLOC)

# # Optional install commands
# install(TARGETS memcc EXPORT memccTargets
#     ARCHIVE DESTINATION lib
#     LIBRARY DESTINATION lib
#     RUNTIME DESTINATION bin
# )
# 
# install(DIRECTORY include/ DESTINATION include)
# 
# # Export targets for downstream projects
# install(EXPORT memccTargets
#     FILE memccTargets.cmake
#     NAMESPACE memcc::
#     DESTINATION lib/cmake/memcc
# )