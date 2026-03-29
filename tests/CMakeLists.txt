add_executable(
    unit_memcc_dfstack
    dfstack.c
)
target_link_libraries(
    unit_memcc_dfstack PRIVATE
    memcc::memcc
)
target_compile_definitions(unit_memcc_dfstack PRIVATE
    MEMCC_DEBUG
    MEMCC_ZERO_ON_ALLOC
    MEMCC_ZERO_ON_FREE
)
add_executable(
    unit_memcc_nmstack
    nmstack.c
)
target_link_libraries(
    unit_memcc_nmstack PRIVATE
    memcc::memcc
)
target_compile_definitions(unit_memcc_nmstack PRIVATE
    MEMCC_DEBUG
    MEMCC_ZERO_ON_ALLOC
    MEMCC_ZERO_ON_FREE
)
add_executable(
    unit_memcc_sbblock
    sbblock.c
)
target_link_libraries(
    unit_memcc_sbblock PRIVATE
    memcc::memcc
)
target_compile_definitions(unit_memcc_sbblock PRIVATE
    MEMCC_DEBUG
    MEMCC_ZERO_ON_ALLOC
    MEMCC_ZERO_ON_FREE
)