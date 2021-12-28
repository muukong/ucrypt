
add_compile_options(-O3 -Wall -Wextra -pedantic)

# Copy test data to binary
#file(COPY data DESTINATION .)

configure_file( include/uctest/testdata.h.in ${CMAKE_BINARY_DIR}/config/testdata.h )
include_directories( ${CMAKE_BINARY_DIR}/config/ )

add_executable(tests test.c testcase.c integer/mul.c)
target_include_directories(tests PUBLIC ./include)
target_link_libraries(tests PRIVATE ucryptlib)
add_test(NAME testlib COMMAND tests)
