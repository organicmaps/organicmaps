#include "testing/testing.hpp"

#include "indexer/editable_map_object.hpp"

namespace
{
using osm::EditableMapObject;

UNIT_TEST(EditableMapObject_SetWebsite)
{
  EditableMapObject emo;
  emo.SetWebsite("https://some.thing.org");
  TEST_EQUAL(emo.GetWebsite(), "https://some.thing.org", ());

  emo.SetWebsite("http://some.thing.org");
  TEST_EQUAL(emo.GetWebsite(), "http://some.thing.org", ());

  emo.SetWebsite("some.thing.org");
  TEST_EQUAL(emo.GetWebsite(), "http://some.thing.org", ());

  emo.SetWebsite("");
  TEST_EQUAL(emo.GetWebsite(), "", ());
}

UNIT_TEST(EditableMapObject_ValidateBuildingLevels)
{
  TEST(EditableMapObject::ValidateBuildingLevels(""), ());
  TEST(EditableMapObject::ValidateBuildingLevels("7"), ());
  TEST(EditableMapObject::ValidateBuildingLevels("17"), ());
  TEST(EditableMapObject::ValidateBuildingLevels("25"), ());
  TEST(!EditableMapObject::ValidateBuildingLevels("0"), ());
  TEST(!EditableMapObject::ValidateBuildingLevels("26"), ());
  TEST(!EditableMapObject::ValidateBuildingLevels("22a"), ());
  TEST(!EditableMapObject::ValidateBuildingLevels("a22"), ());
  TEST(!EditableMapObject::ValidateBuildingLevels("2a22"), ());
  TEST(!EditableMapObject::ValidateBuildingLevels("ab"), ());
  TEST(!EditableMapObject::ValidateBuildingLevels(
      "2345534564564453645534545345534564564453645"), ());
}

UNIT_TEST(EditableMapObject_ValidateHouseNumber)
{
  TEST(EditableMapObject::ValidateHouseNumber(""), ());
  TEST(EditableMapObject::ValidateHouseNumber("qwer7ty"), ());
  TEST(EditableMapObject::ValidateHouseNumber("12345678"), ());

  // House number must contain at least one number.
  TEST(!EditableMapObject::ValidateHouseNumber("qwerty"), ());
  // House number is too long.
  TEST(!EditableMapObject::ValidateHouseNumber("1234567890123456"), ());
}

UNIT_TEST(EditableMapObject_ValidateFlats)
{
  TEST(EditableMapObject::ValidateFlats(""), ());
  TEST(EditableMapObject::ValidateFlats("123"), ());
  TEST(EditableMapObject::ValidateFlats("123a"), ());
  TEST(EditableMapObject::ValidateFlats("a"), ());
  TEST(EditableMapObject::ValidateFlats("123-456;a-e"), ());
  TEST(EditableMapObject::ValidateFlats("123-456"), ());
  TEST(EditableMapObject::ValidateFlats("123-456; 43-45"), ());
  TEST(!EditableMapObject::ValidateFlats("123-456, 43-45"), ());
  TEST(!EditableMapObject::ValidateFlats("234-234 124"), ());
  TEST(!EditableMapObject::ValidateFlats("123-345-567"), ());
  TEST(!EditableMapObject::ValidateFlats("234-234;234("), ());
  TEST(!EditableMapObject::ValidateFlats("-;"), ());
}

// See search_tests/postcodes_matcher_test.cpp
// UNIT_TEST(EditableMapObject_ValidatePostCode)
// {
// }

UNIT_TEST(EditableMapObject_ValidatePhone)
{
  TEST(EditableMapObject::ValidatePhone(""), ());
  TEST(EditableMapObject::ValidatePhone("+7 000 000 00 00"), ());
  TEST(EditableMapObject::ValidatePhone("+7 (000) 000 00 00"), ());
  TEST(EditableMapObject::ValidatePhone("+7 0000000000"), ());
  TEST(EditableMapObject::ValidatePhone("+7 0000 000 000"), ());
  TEST(EditableMapObject::ValidatePhone("8 0000-000-000"), ());

  TEST(EditableMapObject::ValidatePhone("000 00 00"), ());
  TEST(EditableMapObject::ValidatePhone("000 000 00"), ());
  TEST(EditableMapObject::ValidatePhone("+00 0000 000 000"), ());

  TEST(!EditableMapObject::ValidatePhone("+00 0000 000 0000 000"), ());
  TEST(!EditableMapObject::ValidatePhone("00 00"), ());
  TEST(!EditableMapObject::ValidatePhone("acb"), ());
  TEST(!EditableMapObject::ValidatePhone("000 000 00b"), ());
}

UNIT_TEST(EditableMapObject_ValidateWebsite)
{
  TEST(EditableMapObject::ValidateWebsite(""), ());
  TEST(EditableMapObject::ValidateWebsite("qwe.rty"), ());

  TEST(!EditableMapObject::ValidateWebsite("qwerty"), ());
  TEST(!EditableMapObject::ValidateWebsite(".qwerty"), ());
  TEST(!EditableMapObject::ValidateWebsite("qwerty."), ());
  TEST(!EditableMapObject::ValidateWebsite(".qwerty."), ());
}

UNIT_TEST(EditableMapObject_ValidateEmail)
{
  TEST(EditableMapObject::ValidateEmail(""), ());
  TEST(EditableMapObject::ValidateEmail("e@ma.il"), ());
  TEST(EditableMapObject::ValidateEmail("e@ma.i.l"), ());

  TEST(!EditableMapObject::ValidateEmail("e.ma.il"), ());
  TEST(!EditableMapObject::ValidateEmail("e@ma@il"), ());
  TEST(!EditableMapObject::ValidateEmail("e@ma@i.l"), ());
  TEST(!EditableMapObject::ValidateEmail("e@mail"), ());
}
}  // namespace
