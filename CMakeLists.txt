cmake_minimum_required(VERSION 3.14)

project(amka
    VERSION 0.5.0
    DESCRIPTION "Validation and parsing of the Greek Social Insurance Number (AMKA)"
    LANGUAGES CXX)

# ---- Library: header-only, zero dependencies --------------------------------
add_library(amka INTERFACE)
add_library(amka::amka ALIAS amka)

target_include_directories(amka INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# Minimum dialect for consumers; they may of course build with anything newer.
target_compile_features(amka INTERFACE cxx_std_17)

# ---- Tests & examples: on by default only when we are the top-level project --
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(AMKA_IS_TOP_LEVEL ON)
else()
    set(AMKA_IS_TOP_LEVEL OFF)
endif()

option(AMKA_BUILD_TESTS    "Build amka tests"    ${AMKA_IS_TOP_LEVEL})
option(AMKA_BUILD_EXAMPLES "Build amka examples" ${AMKA_IS_TOP_LEVEL})
option(AMKA_INSTALL        "Generate amka install rules" ${AMKA_IS_TOP_LEVEL})

if(AMKA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(AMKA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ---- Install & find_package support ------------------------------------------
if(AMKA_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(DIRECTORY include/amka
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(TARGETS amka EXPORT amkaTargets)

    install(EXPORT amkaTargets
        NAMESPACE amka::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/amka)

    configure_package_config_file(
        cmake/amkaConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/amkaConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/amka)

    # Header-only => ABI-independent: any architecture may consume it.
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/amkaConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/amkaConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/amkaConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/amka)

    install(FILES LICENSE
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/amka)
endif()
