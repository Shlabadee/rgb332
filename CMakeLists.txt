cmake_minimum_required(VERSION 3.10)
project(RGB332)

set(CMAKE_C_FLAGS_DEBUG "-g3")
set(CMAKE_C_FLAGS_RELEASE "-O2 -s")

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})

set(SOURCES_COMMON
    ezpng.c
    magic_number.c
    argparse.c
    legal.c
)

set(SOURCES_ENCODER
    encoder.c
    ${SOURCES_COMMON}
)

set(SOURCES_DECODER
    decoder.c
    ${SOURCES_COMMON}
)

add_executable(rgb332enc ${SOURCES_ENCODER})
add_executable(rgb332dec ${SOURCES_DECODER})

target_link_libraries(rgb332enc ${PNG_LIBRARIES})
target_link_libraries(rgb332dec ${PNG_LIBRARIES})
