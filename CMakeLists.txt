cmake_minimum_required(VERSION 3.15)

project(fs)

set(CMAKE_C_STANDARD 11)

include_directories(
        include
        dep/sds
        dep/vec
)

set(
        srcs
        src/fs.c
        main.c
        dep/sds/sds.c
        dep/vec/vec.c
)

add_executable(run ${srcs})