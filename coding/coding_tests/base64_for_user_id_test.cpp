#include "testing/testing.hpp"

#include "coding/base64.hpp"

#include "base/logging.hpp"

#include "std/random.hpp"

using namespace base64_for_user_ids;

UNIT_TEST(Base64_Encode_User_Ids)
{
  TEST_EQUAL(encode("Hello, world!"), "SGVsbG8sIHdvcmxkITAw", ());
  TEST_EQUAL(encode(""), "", ());
  TEST_EQUAL(encode("$"), "JDAw", ());
  TEST_EQUAL(encode("MapsWithMe is an offline maps application for any device in the world."),
    "TWFwc1dpdGhNZSBpcyBhbiBvZmZsaW5lIG1hcHMgYXBwbGljYXRpb24gZm9yIGFueSBkZXZpY2Ug"
    "aW4gdGhlIHdvcmxkLjAw", ());
}

// Removed decode test, because boost behavior is undefined, and we don't actually need
// to decode these oldscool ids in our own code. Our new code should use base64::Encode/Decode instead

UNIT_TEST(Base64_QualityTest_User_Ids)
{
  size_t const NUMBER_OF_TESTS = 10000;
  mt19937 rng(0);
  for (size_t i = 0; i < NUMBER_OF_TESTS; ++i)
  {
    string randomBytes;
    for (size_t j = 0 ; j < 8; ++j)
    {
      if (j == 4)
      {
        randomBytes.push_back('\0');
        continue;
      }
      randomBytes.push_back(static_cast<char>(rng()));
    }
    string const result = encode(randomBytes);
    TEST_GREATER_OR_EQUAL(result.size(), randomBytes.size(),
        (randomBytes, result));
    for (size_t i = 0; i < result.size(); ++i)
      TEST_NOT_EQUAL(result[i], 0, ());
  }
}
