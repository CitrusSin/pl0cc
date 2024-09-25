cmake_minimum_required(VERSION 3.23)
project(pl0cc)

set(CMAKE_CXX_STANDARD 17)

include_directories(./include)

add_executable(
        pl0cc
        main.cpp
        regex.cpp
        deterministic_automaton.cpp
        nondeterministic_automaton.cpp
        regex_parse.cpp
        lexer.cpp
        include/deterministic_automaton.hpp
        include/lexer.hpp
        include/nondeterministic_automaton.hpp
        include/regex.hpp
        include/regex_parse.hpp
)