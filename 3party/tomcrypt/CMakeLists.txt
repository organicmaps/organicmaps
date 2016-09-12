cmake_minimum_required(VERSION 3.2)

project(tomcrypt C CXX)

add_compile_options(
    "-Wall"
    "-pedantic"
)

add_definitions(-DLTC_NO_ROLC)

include_directories(src/headers)

set(
    SRC
    src/hashes/sha2/sha256.c
    src/headers/tomcrypt.h
    src/headers/tomcrypt_misc.h
    src/misc/base64/base64_decode.c
    src/misc/base64/base64_encode.c
    src/misc/crypt/crypt_argchk.c 
)

add_library(tomcrypt ${SRC})
set_target_properties(tomcrypt PROPERTIES ENABLE_EXPORTS ON)
