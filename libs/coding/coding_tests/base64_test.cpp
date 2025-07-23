#include "testing/testing.hpp"

#include "coding/base64.hpp"

using namespace base64;

UNIT_TEST(Base64_Smoke)
{
  char const * bytes[] = {"H", "He", "Hel", "Hell", "Hello", "Hello,", "Hello, ", "Hello, World!"};
  char const * encoded[] = {
      "SA==", "SGU=", "SGVs", "SGVsbA==", "SGVsbG8=", "SGVsbG8s", "SGVsbG8sIA==", "SGVsbG8sIFdvcmxkIQ=="};

  TEST_EQUAL(ARRAY_SIZE(bytes), ARRAY_SIZE(encoded), ());

  for (size_t i = 0; i < ARRAY_SIZE(bytes); ++i)
  {
    TEST_EQUAL(Encode(bytes[i]), encoded[i], ());
    TEST_EQUAL(Decode(encoded[i]), bytes[i], ());
    TEST_EQUAL(Decode(Encode(bytes[i])), bytes[i], ());
    TEST_EQUAL(Encode(Decode(encoded[i])), encoded[i], ());
  }

  char const * str = "MapsWithMe is the offline maps application for any device in the world.";
  char const * encStr =
      "TWFwc1dpdGhNZSBpcyB0aGUgb2ZmbGluZSBtYXBzIGFwcGxpY2F0aW9uIGZvciBhbnkgZGV2aWNlIGluIHRoZSB3b3JsZC4=";
  TEST_EQUAL(Encode(str), encStr, ());
  TEST_EQUAL(Decode(encStr), str, ());
}
