cmake_minimum_required(VERSION 3.14)
project(amka_consumer CXX)

find_package(amka 0.4 REQUIRED)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE amka::amka)

enable_testing()
add_test(NAME consumer.smoke COMMAND consumer)
