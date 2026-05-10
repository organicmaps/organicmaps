#include "testing/testing.hpp"

#include <string_view>

namespace message_test
{
enum class Printable
{
  Value
};

std::string_view DebugPrint(Printable value)
{
  switch (value)
  {
  case Printable::Value: return "string_view result";
  }
  UNREACHABLE();
}
}  // namespace message_test

UNIT_TEST(DebugPrint_Char)
{
  char const * s = "Тест";

  char const arr1[] = "Тест";
  char constexpr arr2[] = "Тест";
  char arr3[] = "Тест";

  std::string str(s);
  char * s2 = str.data();

  TEST_EQUAL(s, DebugPrint(s), ());
  TEST_EQUAL(s, DebugPrint(arr1), ());
  TEST_EQUAL(s, DebugPrint(arr2), ());
  TEST_EQUAL(s, DebugPrint(arr3), ());
  TEST_EQUAL(s, DebugPrint(s2), ());
  TEST_EQUAL(s, DebugPrint(str), ());
}

UNIT_TEST(DebugPrint_StringView)
{
  std::string const expected("ab\0cd", 5);
  std::string_view const view(expected.data(), expected.size());

  TEST_EQUAL(DebugPrint(std::string(expected)), expected, ());
  TEST_EQUAL(DebugPrint(view), expected, ());
  TEST_EQUAL(base::Message(std::string(expected)), expected, ());
  TEST_EQUAL(base::Message(view), expected, ());
}

UNIT_TEST(Message_DebugPrintStringViewResult)
{
  TEST_EQUAL(base::Message(message_test::Printable::Value), "string_view result", ());
  TEST_EQUAL(base::Message("prefix", message_test::Printable::Value), "prefix string_view result", ());
}

UNIT_TEST(Message_VariadicArgs)
{
  TEST_EQUAL(base::Message(), "", ());
  TEST_EQUAL(base::Message("only"), "only", ());
  TEST_EQUAL(base::Message("a", "b"), "a b", ());
  TEST_EQUAL(base::Message("a", "b", "c", "d", "e"), "a b c d e", ());
  TEST_EQUAL(base::Message(1, 2, 3), "1 2 3", ());
  TEST_EQUAL(base::Message("count", 42, "items"), "count 42 items", ());
}

UNIT_TEST(Message_NullCharPointer)
{
  char const * p = nullptr;
  TEST_EQUAL(base::Message(p), "NULL", ());
  TEST_EQUAL(base::Message("before", p, "after"), "before NULL after", ());
}

UNIT_TEST(Message_NumericArgs)
{
  TEST_EQUAL(base::Message(0), "0", ());
  TEST_EQUAL(base::Message(-1), "-1", ());
  TEST_EQUAL(base::Message(static_cast<size_t>(99)), "99", ());
  TEST_EQUAL(base::Message(static_cast<int64_t>(-9223372036854775807LL - 1)), "-9223372036854775808", ());
  TEST_EQUAL(base::Message('A'), "A", ());  // char keeps character semantics, not numeric
}

UNIT_TEST(Message_FloatArgs)
{
  // Floats route through base::FloatToString (default precision 6, trailing zeroes stripped)
  // because std::to_chars for float/double isn't available on iOS 15.
  TEST_EQUAL(base::Message(0.0), "0", ());
  TEST_EQUAL(base::Message(3.14), "3.14", ());
  TEST_EQUAL(base::Message(-0.5f), "-0.5", ());
  TEST_EQUAL(base::Message("pi", 3.14, "rad"), "pi 3.14 rad", ());
}
