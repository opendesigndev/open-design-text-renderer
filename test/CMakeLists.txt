cmake_minimum_required(VERSION 3.13.5)

project(textify LANGUAGES C CXX)
set(TEST_NAME "${PROJECT_NAME}_Test")

# Google Test
enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

set(TEXTIFY_DIR ${CMAKE_SOURCE_DIR}/textify/)
set(TEXTIFY_TEST_DIR ${TEXTIFY_DIR}/test)

# Define a value for testing Octopus files directory
add_definitions(-DTESTING_OCTOPUS_DIR="${TEXTIFY_DIR}/test/test-files/")
# Define a value for a directory of fonts
#     TODO: Matus: Make sure this is an acceptable local directory.
add_definitions(-DFONTS_DIR="${CMAKE_SOURCE_DIR}/../../Fonts/")

# Test source files
set(TEST_SOURCE_FILES
    ${TEXTIFY_TEST_DIR}/src/main.cpp
    ${TEXTIFY_TEST_DIR}/src/TextifyApiTests.cpp
)

# Add executables
add_executable(${TEST_NAME} ${TEST_SOURCE_FILES})

# Required Open Design internal libraries
set(INTERNAL_LIBRARY_DEPENDENCIES
    textify
    liboctopus
    ode-essentials
    ode-renderer
)

target_link_libraries(${TEST_NAME} PRIVATE
    ${GTEST_BOTH_LIBRARIES} ${INTERNAL_LIBRARY_DEPENDENCIES})

target_include_directories(${TEST_NAME} PUBLIC
    ${TEXTIFY_DIR}/src/)

# Add the test
add_test(${TEST_NAME} ${TEST_NAME})

set_target_properties(${TEST_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS NO)

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
