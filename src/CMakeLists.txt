
add_library(ucryptlib STATIC integer.c prime.c rand.c string_util.c ucstr.c)
target_include_directories(ucryptlib PUBLIC ../include)
