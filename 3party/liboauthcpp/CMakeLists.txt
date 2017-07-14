project(oauthcpp)

include_directories(include src)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(
    "-Wno-shorten-64-to-32"
  )
endif()

set(
  SRC
  include/liboauthcpp/liboauthcpp.h
  src/base64.cpp
  src/HMAC_SHA1.cpp
  src/SHA1.cpp
  src/urlencode.cpp
  src/liboauthcpp.cpp
)

add_library(${PROJECT_NAME} ${SRC})
