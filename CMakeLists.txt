cmake_minimum_required(VERSION 4.1)
project(coffee_chess_engine VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 

find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
endif()

file(GLOB SOURCES "src/*")

add_executable(red_bean ${SOURCES})

target_compile_options(red_bean PRIVATE -march=native -fconstexpr-steps=100000000 -std=c++17 -O2) #  -O3 -flto  -fconstexpr-steps=100000000 