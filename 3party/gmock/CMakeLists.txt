project(gmock)

include_directories(. gtest gtest/include include)

set(
  SRC
  gtest/src/gtest-all.cc
  gtest/src/gtest-death-test.cc
  gtest/src/gtest-filepath.cc
  gtest/src/gtest-internal-inl.h
  gtest/src/gtest-port.cc
  gtest/src/gtest-printers.cc
  gtest/src/gtest-test-part.cc
  gtest/src/gtest-typed-test.cc
  gtest/src/gtest.cc
  include/gmock/gmock-actions.h
  include/gmock/gmock-cardinalities.h
  include/gmock/gmock-generated-actions.h
  include/gmock/gmock-generated-function-mockers.h
  include/gmock/gmock-generated-matchers.h
  include/gmock/gmock-generated-nice-strict.h
  include/gmock/gmock-matchers.h
  include/gmock/gmock-more-actions.h
  include/gmock/gmock-more-matchers.h
  include/gmock/gmock-spec-builders.h
  include/gmock/gmock.h
  include/gmock/internal/gmock-generated-internal-utils.h
  include/gmock/internal/gmock-internal-utils.h
  include/gmock/internal/gmock-port.h
  src/gmock-all.cc
  src/gmock-cardinalities.cc
  src/gmock-internal-utils.cc
  src/gmock-matchers.cc
  src/gmock-spec-builders.cc
  src/gmock.cc
  src/gmock_main.cc
)

add_library(${PROJECT_NAME} ${SRC})
