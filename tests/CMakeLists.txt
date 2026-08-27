# Pin the dialect hard: prove the library is clean C++17 — no GNU extensions,
# no accidental C++20-isms (e.g. constexpr std::array comparisons).
function(amka_pin_cxx17 target)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# ---- Compile-time suite: building it IS the test ----------------------------
add_executable(amka_compile_time_tests compile_time_tests.cpp)
target_link_libraries(amka_compile_time_tests PRIVATE amka::amka)
amka_pin_cxx17(amka_compile_time_tests)
add_test(NAME amka.compile_time COMMAND amka_compile_time_tests)

# ---- Runtime suite: Catch2 (dev-only dependency, never shipped) --------------
include(FetchContent)
FetchContent_Declare(Catch2
    URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.11.0.tar.gz)
FetchContent_MakeAvailable(Catch2)

add_executable(amka_runtime_tests runtime_tests.cpp generator_tests.cpp)
target_link_libraries(amka_runtime_tests PRIVATE amka::amka Catch2::Catch2WithMain)
amka_pin_cxx17(amka_runtime_tests)
add_test(NAME amka.runtime COMMAND amka_runtime_tests)
