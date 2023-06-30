
add_library(ucrypt STATIC integer.c prime.c rand.c string_util.c rsa.c sha256.c sha.c sha_hmac.c sha224.c)
target_include_directories(ucrypt PUBLIC ../include)
