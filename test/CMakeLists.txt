cmake_minimum_required(VERSION 3.13.5)

project(textify LANGUAGES C CXX)
set(TEST_NAME "${PROJECT_NAME}_Test")

# Google Test
enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Define a value for testing Octopus files directory
add_definitions(-DTESTING_OCTOPUS_DIR="${CMAKE_SOURCE_DIR}/test-files/")

set(TEXTIFY_DIR ${CMAKE_SOURCE_DIR}/textify/)
set(TEXTIFY_TEST_DIR ${TEXTIFY_DIR}/test)

# Test source files
set(TEST_SOURCE_FILES
    ${TEXTIFY_TEST_DIR}/src/main.cpp
    ${TEXTIFY_TEST_DIR}/src/TextifyApiTests.cpp
)

# Add executables
add_executable(${TEST_NAME}
    ${TEST_SOURCE_FILES})

target_link_libraries(${TEST_NAME} PRIVATE
    ${GTEST_BOTH_LIBRARIES})

target_include_directories(${TEST_NAME} PRIVATE
    ${TEXTIFY_DIR}/include/
    ${TEXTIFY_DIR}/src/
    ${TEXTIFY_DIR}/src/vendor/
    ${TEXTIFY_DIR}/../liboctopus/)

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
