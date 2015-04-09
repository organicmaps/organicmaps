#include "testing/testing.hpp"

#include "coding/url_encode.hpp"

char const * orig1 = "http://google.com/main_index.php";
char const * enc1 = "http%3A%2F%2Fgoogle.com%2Fmain_index.php";
char const * orig2 = "Some File Name.ext";
char const * enc2 = "Some%20File%20Name.ext";
char const * orig3 = "Wow,  two spaces?!";
char const * enc3 = "Wow%2C%20%20two%20spaces%3F%21";
char const * orig4 = "#$%^&@~[]{}()|*+`\"\'";
char const * enc4 = "%23%24%25%5E%26%40~%5B%5D%7B%7D%28%29%7C%2A%2B%60%22%27";

UNIT_TEST(UrlEncode)
{
  TEST_EQUAL(UrlEncode(""), "", ());
  TEST_EQUAL(UrlEncode(" "), "%20", ());
  TEST_EQUAL(UrlEncode("%% "), "%25%25%20", ());
  TEST_EQUAL(UrlEncode("20"), "20", ());
  TEST_EQUAL(UrlEncode("Guinea-Bissau"), "Guinea-Bissau", ());
  TEST_EQUAL(UrlEncode(orig1), enc1, ());
  TEST_EQUAL(UrlEncode(orig2), enc2, ());
  TEST_EQUAL(UrlEncode(orig3), enc3, ());
  TEST_EQUAL(UrlEncode(orig4), enc4, ());
}

UNIT_TEST(UrlDecode)
{
  TEST_EQUAL(UrlDecode(""), "", ());
  TEST_EQUAL(UrlDecode("%20"), " ", ());
  TEST_EQUAL(UrlDecode("%25%25%20"), "%% ", ());
  TEST_EQUAL(UrlDecode("20"), "20", ());
  TEST_EQUAL(UrlDecode("Guinea-Bissau"), "Guinea-Bissau", ());
  TEST_EQUAL(UrlDecode(enc1), orig1, ());
  TEST_EQUAL(UrlDecode(enc2), orig2, ());
  TEST_EQUAL(UrlDecode(enc3), orig3, ());
  TEST_EQUAL(UrlDecode(enc4), orig4, ());
}
