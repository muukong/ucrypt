
add_library(ucrypt STATIC integer.c prime.c rand.c string_util.c)
target_include_directories(ucrypt PUBLIC ../include)
