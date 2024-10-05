cmake_minimum_required(VERSION 3.23)
project(pl0cc)

set(CMAKE_CXX_STANDARD 17)

aux_source_directory(src SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})
