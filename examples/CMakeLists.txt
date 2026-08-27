add_executable(amka-check amka_check.cpp)
target_link_libraries(amka-check PRIVATE amka::amka)
set_target_properties(amka-check PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

add_executable(amka-gen amka_gen.cpp)
target_link_libraries(amka-gen PRIVATE amka::amka)
set_target_properties(amka-gen PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
