#include "printf.h"
#include "fixed_size_serializer.h"

#include "../3party/gtest/gtest.h"
#include "../3party/gtest/gtest-main.h"

using bricks::strings::Printf;
using bricks::strings::FixedSizeSerializer;
using bricks::strings::PackToString;
using bricks::strings::UnpackFromString;

TEST(StringPrintf, SmokeTest) {
  EXPECT_EQ("Test: 42, 'Hello', 0000ABBA", Printf("Test: %d, '%s', %08X", 42, "Hello", 0xabba));
}

TEST(FixedSizeSerializer, UInt16) {
  EXPECT_EQ(5, FixedSizeSerializer<uint16_t>::size_in_bytes);
  // Does not fit signed 16-bit, requires unsigned.
  EXPECT_EQ("54321", FixedSizeSerializer<uint16_t>::PackToString(54321));
  EXPECT_EQ(54321, FixedSizeSerializer<uint16_t>::UnpackFromString("54321"));
}

TEST(FixedSizeSerializer, UInt32) {
  EXPECT_EQ(10, FixedSizeSerializer<uint32_t>::size_in_bytes);
  // Does not fit signed 32-bit, requires unsigned.
  EXPECT_EQ("3987654321", FixedSizeSerializer<uint32_t>::PackToString(3987654321));
  EXPECT_EQ(3987654321, FixedSizeSerializer<uint32_t>::UnpackFromString("3987654321"));
}

TEST(FixedSizeSerializer, UInt64) {
  EXPECT_EQ(20, FixedSizeSerializer<uint64_t>::size_in_bytes);
  uint64_t magic = 1e19;
  magic += 42;
  // Does not fit signed 64-bit.
  EXPECT_EQ("10000000000000000042", FixedSizeSerializer<uint64_t>::PackToString(magic));
  EXPECT_EQ(magic, FixedSizeSerializer<uint64_t>::UnpackFromString("10000000000000000042"));
}

TEST(FixedSizeSerializer, ImplicitSyntax) {
  {
    uint32_t x;
    EXPECT_EQ(42u, UnpackFromString("42", x));
  }
  {
    uint16_t x;
    EXPECT_EQ(10000u, UnpackFromString("10000", x));
  }
  {
    uint16_t x = 42;
    EXPECT_EQ("00042", PackToString(x));
  }
  {
    uint64_t x = static_cast<int64_t>(1e18);
    EXPECT_EQ("01000000000000000000", PackToString(x));
  }
}
