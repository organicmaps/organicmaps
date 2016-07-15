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
  TEST(!EditableMapObject::ValidateBuildingLevels("005"), ());
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
  TEST(!EditableMapObject::ValidateWebsite(".qwerty."), ());
  TEST(!EditableMapObject::ValidateWebsite("w..com"), ());
}

UNIT_TEST(EditableMapObject_ValidateEmail)
{
  TEST(EditableMapObject::ValidateEmail(""), ());
  TEST(EditableMapObject::ValidateEmail("e@ma.il"), ());
  TEST(EditableMapObject::ValidateEmail("e@ma.i.l"), ());
  TEST(EditableMapObject::ValidateEmail("e-m.ail@dot.com.gov"), ());
  TEST(EditableMapObject::ValidateEmail("#$%&'*+-/=?^`_{}|~.@dot.qw.com.gov"), ());

  TEST(!EditableMapObject::ValidateEmail("e.ma.il"), ());
  TEST(!EditableMapObject::ValidateEmail("e@ma@il"), ());
  TEST(!EditableMapObject::ValidateEmail("e@ma@i.l"), ());
  TEST(!EditableMapObject::ValidateEmail("e@mail"), ());
  TEST(!EditableMapObject::ValidateEmail("@email.a"), ());
  TEST(!EditableMapObject::ValidateEmail("emai.l@"), ());
  TEST(!EditableMapObject::ValidateEmail("emai@l."), ());
  TEST(!EditableMapObject::ValidateEmail("e mai@l.com"), ());
  TEST(!EditableMapObject::ValidateEmail("emai@.l"), ());
  TEST(!EditableMapObject::ValidateEmail("emai@_l.ab"), ());
  TEST(!EditableMapObject::ValidateEmail("emai@l_.ab"), ());
  TEST(!EditableMapObject::ValidateEmail("email@e#$%&'*+-/=?^`_{}|~.com"), ());
}

UNIT_TEST(EditableMapObject_CanUseAsDefaultName)
{
  EditableMapObject emo;
  
  TEST(EditableMapObject::CanUseAsDefaultName(StringUtf8Multilang::GetLangIndex("de"), emo.GetName(), {"de", "fr"}),
       ("Check possibility to use Mwm language code"));
  TEST(EditableMapObject::CanUseAsDefaultName(StringUtf8Multilang::GetLangIndex("fr"), emo.GetName(), {"de", "fr"}),
       ("Check possibility to use Mwm language code"));
  TEST(EditableMapObject::CanUseAsDefaultName(StringUtf8Multilang::GetLangIndex("int_name"), emo.GetName(), {"de", "fr"}),
       ("Check possibility to use international language code"));

  TEST(!EditableMapObject::CanUseAsDefaultName(100, emo.GetName(), {"de", "fr"}),
       ("Incorrect language code is not supported"));
  TEST(!EditableMapObject::CanUseAsDefaultName(StringUtf8Multilang::GetLangIndex("en"), emo.GetName(), {"abcd"}),
       ("Incorrect Mwm language name is not supported"));
  TEST(!EditableMapObject::CanUseAsDefaultName(StringUtf8Multilang::GetLangIndex("en"), emo.GetName(), {"de", "fr"}),
       ("Can not to use language which not Mwm language or international"));
  TEST(!EditableMapObject::CanUseAsDefaultName(StringUtf8Multilang::GetLangIndex("ru"), emo.GetName(), {"de", "fr"}),
       ("Check possibility to use user`s language code"));

  // Trying to use language codes in reverse priority.
  StringUtf8Multilang names;
  names.AddString(StringUtf8Multilang::GetLangIndex("int_name"), "international name");
  emo.SetName(names);

  TEST(EditableMapObject::CanUseAsDefaultName(StringUtf8Multilang::GetLangIndex("int_name"), emo.GetName(), {"de", "fr"}),
       ("It is possible to fix typo for international language"));

  names.AddString(StringUtf8Multilang::GetLangIndex("fr"), "second mwm language");
  emo.SetName(names);

  TEST(EditableMapObject::CanUseAsDefaultName(StringUtf8Multilang::GetLangIndex("fr"), emo.GetName(), {"de", "fr"}),
       ("It is possible to fix typo"));
  TEST(!EditableMapObject::CanUseAsDefaultName(StringUtf8Multilang::GetLangIndex("int_name"), emo.GetName(), {"de", "fr"}),
       ("Name on language with high priority was already entered"));

  names.AddString(StringUtf8Multilang::GetLangIndex("de"), "first mwm language");
  emo.SetName(names);

  TEST(EditableMapObject::CanUseAsDefaultName(StringUtf8Multilang::GetLangIndex("de"), emo.GetName(), {"de", "fr"}),
       ("It is possible to fix typo"));
  TEST(!EditableMapObject::CanUseAsDefaultName(StringUtf8Multilang::GetLangIndex("fr"), emo.GetName(), {"de", "fr"}),
       ("Name on language with high priority was already entered"));
  TEST(!EditableMapObject::CanUseAsDefaultName(StringUtf8Multilang::GetLangIndex("int_name"), emo.GetName(), {"de", "fr"}),
       ("Name on language with high priority was already entered"));
}

UNIT_TEST(EditableMapObject_GetNamesDataSource)
{
  EditableMapObject emo;
  StringUtf8Multilang names;
  names.AddString(0, "Default name");
  names.AddString(1, "Eng name");
  names.AddString(7, "Int name");
  names.AddString(6, "De name");
  names.AddString(8, "Ru name");
  names.AddString(9, "Sv name");
  names.AddString(12, "By name");
  names.AddString(14, "Ko name");
  names.AddString(20, "It name");
  emo.SetName(names);

  auto const namesDataSource =
      EditableMapObject::GetNamesDataSource(emo.GetName(), {"de", "fr"}, "ko");

  TEST(namesDataSource.names.size() == 9, ("All names except the default should be pushed into "
                                           "data source plus empty mandatory names"));
  TEST(namesDataSource.mandatoryNamesCount == 4,
       ("Mandatory names count should be equal == Mwm languages + user`s language + international "
        "language"));
  TEST(namesDataSource.names[0].m_code == 6 /*de*/,
       ("Deutsch name should be first as first language in Mwm"));
  TEST(namesDataSource.names[1].m_code == 3 /*fr*/,
       ("French name should be second as second language in Mwm"));
  TEST(namesDataSource.names[2].m_code == 14 /*ko*/,
       ("Korean name should be third because user`s language should be followed by Mwm languages"));
  TEST(namesDataSource.names[3].m_code == 7 /*int*/,
       ("International name should be fourth because International language should be followed by "
        "user`s language"));

  {
    auto const namesDataSource =
        EditableMapObject::GetNamesDataSource(emo.GetName(), {"int_name"}, "int_name");
    TEST(namesDataSource.names.size() == 8,
         ("All names except the default should be pushed into data source"));
    TEST(namesDataSource.mandatoryNamesCount == 1,
         ("Mandatory names count should be equal == Mwm languages + user`s language + "
          "international language. Excluding repetiton"));
  }
  {
    auto const namesDataSource =
        EditableMapObject::GetNamesDataSource(emo.GetName(), {"fr", "fr"}, "fr");
    TEST(namesDataSource.names.size() == 9, ("All names except the default should be pushed into "
                                             "data source plus empty mandatory names"));
    TEST(namesDataSource.mandatoryNamesCount == 2,
         ("Mandatory names count should be equal == Mwm languages + user`s language + "
          "international language. Excluding repetiton"));
  }
}
}  // namespace
