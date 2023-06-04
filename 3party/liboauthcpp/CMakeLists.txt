project(oauthcpp)

set(SRC
  include/liboauthcpp/liboauthcpp.h
  src/base64.cpp
  src/HMAC_SHA1.cpp
  src/SHA1.cpp
  src/urlencode.cpp
  src/liboauthcpp.cpp
)

add_library(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME}
  PRIVATE src
  PUBLIC include
)

target_compile_options(${PROJECT_NAME}
  PRIVATE $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wno-shorten-64-to-32>
)
