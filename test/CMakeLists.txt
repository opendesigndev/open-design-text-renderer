cmake_minimum_required(VERSION 3.13.5)

project(open-design-text-renderer LANGUAGES C CXX)
set(TEST_NAME "${PROJECT_NAME}-test")

# Google Test
enable_testing()
find_package(GTest REQUIRED)

set(TEXT_RENDERER_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(TEXT_RENDERER_DIR ${TEXT_RENDERER_TEST_DIR}/..)

# Test header files
set(TEST_HEADER_FILES
    ${TEXT_RENDERER_TEST_DIR}/src/TextRendererApiTests.h
)

# Test source files
set(TEST_SOURCE_FILES
    ${TEXT_RENDERER_TEST_DIR}/src/main.cpp
    ${TEXT_RENDERER_TEST_DIR}/src/TextRendererApiTests.cpp
)

# Add executables
add_executable(${TEST_NAME} ${TEST_HEADER_FILES} ${TEST_SOURCE_FILES})

# Required Open Design internal libraries
set(INTERNAL_LIBRARY_DEPENDENCIES
    open-design-text-renderer
    liboctopus
    ode-essentials
    ode-renderer
)

target_link_libraries(${TEST_NAME} PRIVATE
    GTest::gtest GTest::gtest_main ${INTERNAL_LIBRARY_DEPENDENCIES})

target_include_directories(${TEST_NAME} PUBLIC
    ${TEXT_RENDERER_DIR}/src/)

# Add the test
add_test(${TEST_NAME} ${TEST_NAME})

set_target_properties(${TEST_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS NO)

# Define a value for testing Octopus files directory
target_compile_definitions(${TEST_NAME} PRIVATE TESTING_OCTOPUS_DIR="${TEXT_RENDERER_TEST_DIR}/test-files/")

target_compile_options(${TEST_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
    /W4 # Warning level 4
    /WX # Treat warnings as errors
    >
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
    -Wall
    -Wextra
    # -Werror # Treat warnings as errors
    -Wshadow
    -pedantic
    >)
