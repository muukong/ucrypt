cmake_minimum_required(VERSION 3.20)
project(ucrypt C)

set(CMAKE_C_STANDARD 99)

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Add library build target
add_subdirectory(src)

# Add testing targets
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1) # Disable CTest's default targets
    include(CTest)

    add_subdirectory(test)
endif()

