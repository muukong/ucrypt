cmake_minimum_required(VERSION 3.20)
project(ucrypt C)

set(CMAKE_C_STANDARD 99)

add_executable(ucrypt main.c src/integer.h src/integer.c src/ucalloc.h)
