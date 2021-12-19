cmake_minimum_required(VERSION 3.20)
project(ucrypt C)

set(CMAKE_C_STANDARD 99)

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    #add_compile_options(-Wall -Wextra -pedantic -Werror)
    add_compile_options(-Wall -Wextra -pedantic)
endif()

add_executable(ucrypt main.c src/integer.h src/integer.c src/ucalloc.h src/string_util.h src/string_util.c src/prime.c src/prime.h src/rand.h src/rand.c)
