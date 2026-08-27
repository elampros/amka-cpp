# Pin the dialect hard: default is C++17 — no GNU extensions, no accidental
# C++20-isms (e.g. constexpr std::array comparisons). CI overrides the standard
# to prove the library also builds cleanly under C++20/23.
set(AMKA_TEST_CXX_STANDARD 17 CACHE STRING "C++ standard used to build the amka tests")

function(amka_pin_cxx17 target)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD ${AMKA_TEST_CXX_STANDARD}
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

# Catch2 must be built with the same C++ standard as the tests: templates like
# Catch::StringMaker<std::string_view> exist in the compiled library only when
# Catch2 itself sees C++17+. Otherwise the link fails (e.g. AppleClang, whose
# default dialect predates C++17).
set(CMAKE_CXX_STANDARD ${AMKA_TEST_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

FetchContent_MakeAvailable(Catch2)

add_executable(amka_runtime_tests runtime_tests.cpp generator_tests.cpp)
target_link_libraries(amka_runtime_tests PRIVATE amka::amka Catch2::Catch2WithMain)
amka_pin_cxx17(amka_runtime_tests)
add_test(NAME amka.runtime COMMAND amka_runtime_tests)
