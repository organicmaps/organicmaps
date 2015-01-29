// TODO(dkorolev): Test ScopeGuard and MakeScopeGuard as well.

#include "util.h"

#include "../3party/gtest/gtest.h"
#include "../3party/gtest/gtest-main.h"

static const char global_string[] = "magic";

TEST(Util, CompileTimeStringLength) {
  const char local_string[] = "foo";
  static const char local_static_string[] = "blah";
  EXPECT_EQ(3u, bricks::CompileTimeStringLength(local_string));
  EXPECT_EQ(4u, bricks::CompileTimeStringLength(local_static_string));
  EXPECT_EQ(5u, bricks::CompileTimeStringLength(global_string));
}
