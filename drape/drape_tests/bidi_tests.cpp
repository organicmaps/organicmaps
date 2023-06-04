#include "testing/testing.hpp"

#include "drape/bidi.hpp"

#include <iostream>


UNIT_TEST(Bidi_Smoke)
{
  std::string base = "\u0686\u0631\u0645\u0647\u064A\u0646";
  strings::UniString in = strings::MakeUniString(base);
  strings::UniString out1 = bidi::log2vis(in);
  std::string out = "\uFEE6\uFEF4\uFEEC\uFEE3\uFEAE\uFB7C";
  strings::UniString out2 = strings::MakeUniString(out);
  TEST_EQUAL(out1, out2, ());
}

UNIT_TEST(Bidi_Combine)
{
  {
    // https://www.w3.org/TR/alreq/#h_ligatures
    strings::UniChar arr[] = { 0x644, 0x627 };
    strings::UniString s(arr, arr + ARRAY_SIZE(arr));

    TEST_EQUAL(bidi::log2vis(s).size(), 1, ());
  }
}

UNIT_TEST(Bidi_Print)
{
  auto const PrintTransform = [](std::string const & utf8)
  {
    auto const s = strings::MakeUniString(utf8);
    for (auto const ucp : s)
      std::cout << std::hex << ucp << ", ";

    std::cout << std::endl;
    for (auto const ucp : bidi::log2vis(s))
      std::cout << std::hex << ucp << ", ";

    std::cout << std::endl;
  };

  PrintTransform("گُلهاالحلّة");
  PrintTransform("ക്ക");
}
