cmake_minimum_required(VERSION 3.23)
project(pl0cc)

set(CMAKE_CXX_STANDARD 17)

add_executable(
        pl0cc
        main.cpp
        regex.cpp
        deterministic_automaton.cpp
        nondeterministic_automaton.cpp
        regex_parse.cpp
        lexer.cpp
        deterministic_automaton.hpp
        lexer.hpp
        nondeterministic_automaton.hpp
        regex.hpp
        regex_parse.hpp
        token_storage.cpp
)