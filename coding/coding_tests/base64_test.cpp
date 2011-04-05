#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"

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
