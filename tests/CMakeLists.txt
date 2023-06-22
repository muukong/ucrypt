
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
#set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

#add_compile_options(-O3 -Wall -Wextra -pedantic)

configure_file( include/uctest/testdata.h.in ${CMAKE_BINARY_DIR}/config/testdata.h )
include_directories( ${CMAKE_BINARY_DIR}/config/ )

add_executable(tests test.c testcase.c integer/mul.c integer/div.c integer/expmod.c integer/sqr.c)
target_include_directories(tests PUBLIC ./include)
target_link_libraries(tests PRIVATE ucrypt)
add_test(NAME testlib COMMAND tests)
