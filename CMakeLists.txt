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

# Trained network embedded into the binary by incbin.
set(NET_FILE "${CMAKE_SOURCE_DIR}/net.bin")

target_compile_definitions(red_bean PRIVATE NET_PATH="${NET_FILE}")

# Rebuild nnue.cpp when the net changes. LTO is off for this one file because
# it breaks incbin's inline-asm path lookup.
set_source_files_properties(src/nnue.cpp PROPERTIES
    OBJECT_DEPENDS "${NET_FILE}"
    COMPILE_OPTIONS "-fno-lto")

target_compile_options(red_bean PRIVATE -march=native -fconstexpr-steps=1000000000 -O3 -flto)