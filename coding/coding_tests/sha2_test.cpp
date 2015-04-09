#include "base/SRC_FIRST.hpp"
#include "testing/testing.hpp"

#include "coding/sha2.hpp"

UNIT_TEST(Sha2_256)
{
  TEST_EQUAL(sha2::digest256("Hello, world!"),
             "315F5BDB76D078C43B8AC0064E4A0164612B1FCE77C869345BFC94C75894EDD3", ());
  TEST_EQUAL(sha2::digest256(""),
             "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855", ());
  char const zero[] = "\x3e\x23\xe8\x16\x00\x39\x59\x4a\x33\x89\x4f\x65\x64\xe1\xb1\x34\x8b\xbd\x7a\x00\x88\xd4\x2c\x4a\xcb\x73\xee\xae\xd5\x9c\x00\x9d";
  TEST_EQUAL(sha2::digest256("b", false),
             string(zero, ARRAY_SIZE(zero) - 1), ());
}
