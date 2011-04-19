#include "../../testing/testing.hpp"

#include "../../base/logging.hpp"
#include "../../base/pseudo_random.hpp"

#include "../base64.hpp"

UNIT_TEST(Base64_Encode)
{
  TEST_EQUAL(base64::encode("Hello, world!"), "SGVsbG8sIHdvcmxkITAw", ());
  TEST_EQUAL(base64::encode(""), "", ());
  TEST_EQUAL(base64::encode("$"), "JDAw", ());
  TEST_EQUAL(base64::encode("MapsWithMe is an offline maps application for any device in the world."),
    "TWFwc1dpdGhNZSBpcyBhbiBvZmZsaW5lIG1hcHMgYXBwbGljYXRpb24gZm9yIGFueSBkZXZpY2Ug"
    "aW4gdGhlIHdvcmxkLjAw", ());
}

UNIT_TEST(Base64_Decode)
{
  TEST_EQUAL(base64::decode("SGVsbG8sIHdvcmxkIQ"), "Hello, world!", ());
  TEST_EQUAL(base64::decode(""), "", ());
  TEST_EQUAL(base64::decode("JA"), "$", ());
  TEST_EQUAL(base64::decode("TWFwc1dpdGhNZSBpcyBhbiBvZmZsaW5lIG1hcHMgYXBwbGljYXRpb24gZm9yIGFueSBkZXZpY2Ug"
             "aW4gdGhlIHdvcmxkLg"),
             "MapsWithMe is an offline maps application for any device in the world.", ());
}

UNIT_TEST(Base64_QualityTest)
{
  size_t const NUMBER_OF_TESTS = 10000;
  LCG32 generator(NUMBER_OF_TESTS);
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
      randomBytes.push_back(static_cast<char>(generator.Generate()));
    }
    string const result = base64::encode(randomBytes);
    TEST_GREATER_OR_EQUAL(result.size(), randomBytes.size(),
        (randomBytes, result));
    for (size_t i = 0; i < result.size(); ++i)
      TEST_NOT_EQUAL(result[i], 0, ());
  }
}
