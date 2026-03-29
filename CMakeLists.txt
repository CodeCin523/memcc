cmake_minimum_required(VERSION 3.11)

project(memcc
    VERSION 0.1.1
    LANGUAGES C
)

# Generate compile_commands.json for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# INTERFACE library (header-only)
add_library(memcc INTERFACE)
add_library(memcc::memcc ALIAS memcc)

# Include directories
target_include_directories(memcc
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# C standard
target_compile_features(memcc INTERFACE c_std_11)

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