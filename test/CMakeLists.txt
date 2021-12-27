
add_executable(tests test.c)
#target_include_directories(ucryptlib PUBLIC ./include)
target_link_libraries(tests PRIVATE ucryptlib)
add_test(NAME testlib COMMAND tests)

#add_executable(int_test test.c)
#add_executable(test_mul test_mul.c)

#add_test(int_test2 int_test)
#add_test(test_mul2 test_mul)

#target_link_libraries(test_ucrypt ucrypt)

