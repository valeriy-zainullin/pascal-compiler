cmake_minimum_required(VERSION 3.12)
project(PascalInterpreter)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1) # For clang-format.

set(CMAKE_CXX_STANDARD 20)
add_compile_options("-g")
add_compile_options(-fsanitize=address,undefined)
add_link_options(-fsanitize=address,undefined)


find_package(FLEX  2.6 REQUIRED)
find_package(BISON 2.6 REQUIRED)

BISON_TARGET(
    MyParser

    parsing/parser.y
    ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
    COMPILE_FLAGS --graph
    DEFINES_FILE     ${CMAKE_CURRENT_BINARY_DIR}/parser.hh
)

FLEX_TARGET(
    MyScanner
    parsing/scanner.l
    ${CMAKE_CURRENT_BINARY_DIR}/scanner.cpp
)

ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

add_executable(
    pascal

    main.cpp
    driver.cpp
    ast/ast.cpp
    ${BISON_MyParser_OUTPUTS}
    ${FLEX_MyScanner_OUTPUTS}
)

add_custom_target(test COMMAND pascal ${CMAKE_CURRENT_LIST_DIR}/test.pas)

target_include_directories(pascal PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_BINARY_DIR})
