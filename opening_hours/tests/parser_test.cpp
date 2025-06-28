#include "testing/testing.hpp"

#include "opening_hours/parser.hpp"

namespace om::opening_hours
{
UNIT_TEST(Parser_ParseWeekday)
{
  Parser const p;

  std::unordered_map<std::string_view, Weekday> const expected = {
    {"Monday", Weekday::Monday},     {"Mon", Weekday::Monday},          {"Tuesday", Weekday::Tuesday},
    {"Tue", Weekday::Tuesday},       {"Wednesday", Weekday::Wednesday}, {"Wed", Weekday::Wednesday},
    {"Thursday", Weekday::Thursday}, {"Thu", Weekday::Thursday},        {"Friday", Weekday::Friday},
    {"Fr", Weekday::Friday},         {"Saturday", Weekday::Saturday},   {"Sat", Weekday::Saturday},
    {"Sunday", Weekday::Sunday},     {"Sun", Weekday::Sunday}};

  for (auto const & [str, expectedWeekday] : expected)
    TEST_EQUAL(p.parse(str), expectedWeekday, ("Failed to parse str:", str));

  TEST_EQUAL(p.parse("Some random string"), Weekday::Invalid, ("Failed to parse invalid weekday"));
}
}  // namespace om::opening_hours
