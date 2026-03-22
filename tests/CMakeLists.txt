add_executable(
    unit_memcc_sbblock
    sbblock.c
)
target_link_libraries(
    unit_memcc_sbblock PRIVATE
    memcc::memcc
)