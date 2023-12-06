cmake_minimum_required(VERSION 3.12)

add_compile_options("-g")

project(PascalInterpreter)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1) # For clang-format.

find_package(FLEX  2.6 REQUIRED)
find_package(BISON 2.6 REQUIRED)

set(
    HEADERS

    ${PascalInterpreter_SOURCE_DIR}/driver.hh
    ${PascalInterpreter_SOURCE_DIR}/parser.hh
)

BISON_TARGET(
    MyParser

    parser.y
    ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
    COMPILE_FLAGS --graph
    DEFINES_FILE     ${CMAKE_CURRENT_BINARY_DIR}/parser.hh
)

FLEX_TARGET(
    MyScanner
    scanner.l
    ${CMAKE_CURRENT_BINARY_DIR}/scanner.cpp
)

ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

add_executable(
    pascal

    main.cpp
    driver.cpp
    ast.cpp
    ${BISON_MyParser_OUTPUTS}
    ${FLEX_MyScanner_OUTPUTS}
)

add_custom_target(test COMMAND pascal ${CMAKE_CURRENT_LIST_DIR}/test.pas)

target_include_directories(pascal PRIVATE ${CMAKE_CURRENT_LIST_DIR} -p -s -l ${CMAKE_CURRENT_BINARY_DIR})
