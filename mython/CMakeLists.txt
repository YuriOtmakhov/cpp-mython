cmake_minimum_required(VERSION 3.11)

project(Myton CXX)
set(CMAKE_CXX_STANDARD 17)

set(LEXER_FILES lexer.h lexer.cpp)
set(RUNTIME_FILES runtime.h runtime.cpp)
set(PARSE_FILES parse.h statement.h parse.cpp statement.cpp)

set(TEST_FILES lexer_test_open.cpp parse_test.cpp runtime_test.cpp statement_test.cpp test_runner_p.h)

add_executable(myton_interpreter main.cpp ${LEXER_FILES} ${RUNTIME_FILES} ${PARSE_FILES} ${TEST_FILES})
