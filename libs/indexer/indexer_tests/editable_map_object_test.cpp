#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/validate_and_format_contacts.hpp"

#include <string>
#include <vector>

namespace editable_map_object_test
{
using namespace std;

using osm::EditableMapObject;

int8_t GetLangCode(char const * ch)
{
  return StringUtf8Multilang::GetLangIndex(ch);
}

struct ExpectedName
{
  string m_lang;
  string m_value;
};

string DebugPrint(ExpectedName const & expectedName)
{
  return expectedName.m_lang + ", " + expectedName.m_value;
}

void CheckExpectations(StringUtf8Multilang const & s, vector<ExpectedName> const & expectations)
{
  size_t counter = 0;
  s.ForEach([&expectations, &counter](int8_t const code, string_view name)
  {
    auto const it = find_if(expectations.begin(), expectations.end(),
                            [&code](ExpectedName const & item) { return GetLangCode(item.m_lang.c_str()) == code; });

    if (it == expectations.end())
      TEST(false, ("Unexpected language code: ", code, ". Expectations: ", expectations));

    TEST_EQUAL(name, it->m_value, ());
    ++counter;
  });

  TEST_EQUAL(counter, expectations.size(),
             ("Unexpected count of names, expected ", expectations.size(), ", but turned out ", counter,
              ". Expectations: ", expectations));
}

UNIT_TEST(EditableMapObject_SetWebsite)
{
  pair<char const *, char const *> arr[] = {
      {"https://some.thing.org", "https://some.thing.org"},
      {"http://some.thing.org", "http://some.thing.org"},
      {"some.thing.org", "http://some.thing.org"},
      {"", ""},
  };

  EditableMapObject emo;
  for (auto const & e : arr)
  {
    emo.SetMetadata(feature::Metadata::FMD_WEBSITE, e.first);
    TEST_EQUAL(emo.GetMetadata(feature::Metadata::FMD_WEBSITE), e.second, ());
  }
}

UNIT_TEST(EditableMapObject_ValidateBuildingLevels)
{
  TEST(EditableMapObject::ValidateBuildingLevels(""), ());
  TEST(EditableMapObject::ValidateBuildingLevels("7"), ());
  TEST(EditableMapObject::ValidateBuildingLevels("17"), ());
  TEST(EditableMapObject::ValidateBuildingLevels("25"), ());
  TEST(!EditableMapObject::ValidateBuildingLevels("0"), ());
  TEST(!EditableMapObject::ValidateBuildingLevels("005"), ());
  TEST(!EditableMapObject::ValidateBuildingLevels(std::to_string(EditableMapObject::kMaximumLevelsEditableByUsers + 1)),
       ());
  TEST(!EditableMapObject::ValidateBuildingLevels("22a"), ());
  TEST(!EditableMapObject::ValidateBuildingLevels("a22"), ());
  TEST(!EditableMapObject::ValidateBuildingLevels("2a22"), ());
  TEST(!EditableMapObject::ValidateBuildingLevels("ab"), ());
  TEST(!EditableMapObject::ValidateBuildingLevels("2345534564564453645534545345534564564453645"), ());
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

UNIT_TEST(EditableMapObject_ValidatePhoneList)
{
  TEST(EditableMapObject::ValidatePhoneList(""), ());
  TEST(EditableMapObject::ValidatePhoneList("+7 000 000 00 00"), ());
  TEST(EditableMapObject::ValidatePhoneList("+7 (000) 000 00 00"), ());
  TEST(EditableMapObject::ValidatePhoneList("+7 0000000000"), ());
  TEST(EditableMapObject::ValidatePhoneList("+7 0000 000 000"), ());
  TEST(EditableMapObject::ValidatePhoneList("8 0000-000-000"), ());

  TEST(EditableMapObject::ValidatePhoneList("000 00 00"), ());
  TEST(EditableMapObject::ValidatePhoneList("000 000 00"), ());
  TEST(EditableMapObject::ValidatePhoneList("+00 0000 000 000"), ());

  TEST(EditableMapObject::ValidatePhoneList("+7 000 000 00 00; +7 000 000 00 00"), ());
  TEST(EditableMapObject::ValidatePhoneList("+7 (000) 000 00 00, +7 (000) 000 00 00"), ());
  TEST(EditableMapObject::ValidatePhoneList("+7 0000000000;+7 0000000000"), ());
  TEST(EditableMapObject::ValidatePhoneList("+7 0000 000 000,+7 0000 000 000"), ());
  TEST(EditableMapObject::ValidatePhoneList("8 0000-000-000; 8 0000-000-000"), ());

  TEST(EditableMapObject::ValidatePhoneList("+7 00 00;7 (0)00 0, 800-00-0; 000000000000000,12345"), ());

  TEST(!EditableMapObject::ValidatePhoneList("+00 0000 000 0000 000"), ());
  TEST(!EditableMapObject::ValidatePhoneList("00 00"), ());
  TEST(!EditableMapObject::ValidatePhoneList("acb"), ());
  TEST(!EditableMapObject::ValidatePhoneList("000 000 00b"), ());
  TEST(!EditableMapObject::ValidatePhoneList(";"), ());
  TEST(!EditableMapObject::ValidatePhoneList(","), ());
  TEST(!EditableMapObject::ValidatePhoneList(";;;;;;"), ());

  // Now it is possible to specify the following incorrect phone numbers.
  // TODO: replace current implementation of ValidatePhoneList by a correct one.
  TEST(EditableMapObject::ValidatePhoneList("7+ 10 10"), ());
  TEST(EditableMapObject::ValidatePhoneList("+7 )10( 10"), ());
  TEST(EditableMapObject::ValidatePhoneList("+7 )10 10"), ());
  TEST(EditableMapObject::ValidatePhoneList("+7 10 (---) 10"), ());
}

UNIT_TEST(EditableMapObject_ValidateWebsite)
{
  TEST(osm::ValidateWebsite(""), ());
  TEST(osm::ValidateWebsite("qwe.rty"), ());
  TEST(osm::ValidateWebsite("http://websit.e"), ());
  TEST(osm::ValidateWebsite("https://websit.e"), ());

  TEST(!osm::ValidateWebsite("qwerty"), ());
  TEST(!osm::ValidateWebsite(".qwerty"), ());
  TEST(!osm::ValidateWebsite("qwerty."), ());
  TEST(!osm::ValidateWebsite(".qwerty."), ());
  TEST(!osm::ValidateWebsite("w..com"), ());
  TEST(!osm::ValidateWebsite("http://.websit.e"), ());
  TEST(!osm::ValidateWebsite("https://.websit.e"), ());
  TEST(!osm::ValidateWebsite("http://"), ());
  TEST(!osm::ValidateWebsite("https://"), ());
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

UNIT_TEST(EditableMapObject_ValidateName)
{
  vector<string> correctNames = {"abc",   "абв",  "ᆺᆯㅕ", "꫞ꪺꫀꪸ",  "a b?c", "a!b.c", "a(b)c",
                                 "a,b.c", "a$bc", "a%bc", "a#bc", "a№bc",  "c&a",   "a[bc"};
  vector<string> incorrectNames = {"a^bc",
                                   "a~bc",
                                   "a§bc",
                                   "a>bc",
                                   "a<bc",
                                   "a{bc",
                                   "*",
                                   "a*bc",
                                   "a=bc",
                                   "a_bc",
                                   "a±bc",
                                   "a\nbc",
                                   "a\tbc",
                                   "a\rbc",
                                   "a\vbc",
                                   "a\fbc",
                                   "a|bc",
                                   "N√",
                                   "Hello World!\U0001F600",
                                   "Exit →",
                                   "∫0dx = C",
                                   "\U0001210A",
                                   "⚠︎",
                                   "⚠️"};

  for (auto const & name : correctNames)
    TEST(EditableMapObject::ValidateName(name), ());

  for (auto const & name : incorrectNames)
    TEST(!EditableMapObject::ValidateName(name), ());
}

UNIT_TEST(EditableMapObject_CanUseAsDefaultName)
{
  EditableMapObject emo;
  vector<int8_t> const nativeMwmLanguages{GetLangCode("de"), GetLangCode("fr")};

  TEST(EditableMapObject::CanUseAsDefaultName(GetLangCode("de"), nativeMwmLanguages),
       ("Check possibility to use Mwm language code"));
  TEST(EditableMapObject::CanUseAsDefaultName(GetLangCode("fr"), nativeMwmLanguages),
       ("Check possibility to use Mwm language code"));
  TEST(!EditableMapObject::CanUseAsDefaultName(GetLangCode("int_name"), nativeMwmLanguages),
       ("Check possibility to use international language code"));
  TEST(!EditableMapObject::CanUseAsDefaultName(100, nativeMwmLanguages), ("Incorrect language code is not supported"));
  TEST(!EditableMapObject::CanUseAsDefaultName(GetLangCode("en"), {GetLangCode("abcd")}),
       ("Incorrect Mwm language name is not supported"));
  TEST(!EditableMapObject::CanUseAsDefaultName(GetLangCode("en"), nativeMwmLanguages),
       ("Can not to use language which not Mwm language or international"));
  TEST(!EditableMapObject::CanUseAsDefaultName(GetLangCode("ru"), nativeMwmLanguages),
       ("Check possibility to use user`s language code"));

  // Trying to use language codes in reverse priority.
  StringUtf8Multilang names;
  names.AddString(GetLangCode("fr"), "second mwm language");
  emo.SetName(names);

  TEST(EditableMapObject::CanUseAsDefaultName(GetLangCode("fr"), nativeMwmLanguages), ("It is possible to fix typo"));

  names.AddString(GetLangCode("de"), "first mwm language");
  emo.SetName(names);

  TEST(EditableMapObject::CanUseAsDefaultName(GetLangCode("de"), nativeMwmLanguages), ("It is possible to fix typo"));
  TEST(EditableMapObject::CanUseAsDefaultName(GetLangCode("fr"), nativeMwmLanguages), ("It is possible to fix typo"));
}

UNIT_TEST(EditableMapObject_GetNamesDataSource)
{
  EditableMapObject emo;
  StringUtf8Multilang names;

  names.AddString(GetLangCode("default"), "Default name");
  names.AddString(GetLangCode("en"), "Eng name");
  names.AddString(GetLangCode("int_name"), "Int name");
  names.AddString(GetLangCode("de"), "De name");
  names.AddString(GetLangCode("ru"), "Ru name");
  names.AddString(GetLangCode("sv"), "Sv name");
  names.AddString(GetLangCode("be"), "By name");
  names.AddString(GetLangCode("ko"), "Ko name");
  names.AddString(GetLangCode("it"), "It name");
  emo.SetName(names);

  vector<int8_t> nativeMwmLanguages = {GetLangCode("de"), GetLangCode("fr")};

  auto const namesDataSource =
      EditableMapObject::GetNamesDataSource(emo.GetNameMultilang(), nativeMwmLanguages, GetLangCode("ko"));

  TEST_EQUAL(namesDataSource.names.size(), 9, ("All names including the default should be pushed into data source"));
  TEST_EQUAL(namesDataSource.mandatoryNamesCount, 1, ("Mandatory names count should always be 1"));
  TEST_EQUAL(namesDataSource.names[0].m_code, GetLangCode("default"), ("Default is always first in the list"));

  {
    vector<int8_t> nativeMwmLanguages = {GetLangCode("de"), GetLangCode("fr")};

    auto const namesDataSource =
        EditableMapObject::GetNamesDataSource(emo.GetNameMultilang(), nativeMwmLanguages, GetLangCode("fr"));
    TEST_EQUAL(namesDataSource.names.size(), 9, ("All names including the default should be pushed into data source"));
    TEST_EQUAL(namesDataSource.mandatoryNamesCount, 1, ("Mandatory names count should always be 1"));
  }
  {
    vector<int8_t> nativeMwmLanguages = {GetLangCode("fr"), GetLangCode("en")};

    auto const namesDataSource =
        EditableMapObject::GetNamesDataSource(emo.GetNameMultilang(), nativeMwmLanguages, GetLangCode("fr"));
    TEST_EQUAL(namesDataSource.names.size(), 9, ("All names including the default should be pushed into data source"));
    TEST_EQUAL(namesDataSource.mandatoryNamesCount, 1, ("Mandatory names count should always be 1"));
  }
  {
    vector<int8_t> nativeMwmLanguages = {GetLangCode("en"), GetLangCode("en")};

    auto const namesDataSource =
        EditableMapObject::GetNamesDataSource(emo.GetNameMultilang(), nativeMwmLanguages, GetLangCode("en"));
    TEST_EQUAL(namesDataSource.names.size(), 9, ("All names including the default should be pushed into data source"));
    TEST_EQUAL(namesDataSource.mandatoryNamesCount, 1, ("Mandatory names count should always be 1"));
  }
}

namespace
{
void SetTypes(EditableMapObject & emo, std::initializer_list<base::StringIL> types)
{
  auto const & cl = classif();
  feature::TypesHolder holder;
  for (auto const & t : types)
    holder.Add(cl.GetTypeByPath(t));
  emo.SetTypes(holder);
}
}  // namespace

UNIT_TEST(EditableMapObject_SetInternet)
{
  classificator::Load();

  EditableMapObject emo;
  auto const wifiType = classif().GetTypeByPath({"internet_access", "wlan"});
  emo.SetType(wifiType);

  auto types = emo.GetTypes();
  TEST(types.Has(wifiType), ());

  auto const setInternetAndCheck = [wifiType](EditableMapObject & emo, feature::Internet internet, bool hasWifi)
  {
    emo.SetInternet(internet);

    TEST_EQUAL(emo.GetInternet(), internet, ());
    auto const & types = emo.GetTypes();
    TEST_EQUAL(types.Has(wifiType), hasWifi, ());
  };

  setInternetAndCheck(emo, feature::Internet::No, false);
  setInternetAndCheck(emo, feature::Internet::Yes, false);
  setInternetAndCheck(emo, feature::Internet::Wired, false);
  setInternetAndCheck(emo, feature::Internet::Wlan, true);
  setInternetAndCheck(emo, feature::Internet::Unknown, false);
  setInternetAndCheck(emo, feature::Internet::Terminal, false);

  EditableMapObject bunkerEmo;
  SetTypes(bunkerEmo, {{"military", "bunker"}});
  types = bunkerEmo.GetTypes();
  TEST(!types.Has(wifiType), ());

  setInternetAndCheck(bunkerEmo, feature::Internet::Wlan, true);
  setInternetAndCheck(bunkerEmo, feature::Internet::No, false);
  setInternetAndCheck(bunkerEmo, feature::Internet::Wlan, true);
  setInternetAndCheck(bunkerEmo, feature::Internet::Yes, false);
  setInternetAndCheck(bunkerEmo, feature::Internet::Wlan, true);
  setInternetAndCheck(bunkerEmo, feature::Internet::Wired, false);
  setInternetAndCheck(bunkerEmo, feature::Internet::Wlan, true);
  setInternetAndCheck(bunkerEmo, feature::Internet::Unknown, false);
  setInternetAndCheck(bunkerEmo, feature::Internet::Wlan, true);
  setInternetAndCheck(bunkerEmo, feature::Internet::Terminal, false);
  setInternetAndCheck(bunkerEmo, feature::Internet::Wlan, true);
  setInternetAndCheck(bunkerEmo, feature::Internet::Wlan, true);
}

UNIT_TEST(EditableMapObject_RemoveBlankNames)
{
  auto const getCountOfNames = [](StringUtf8Multilang const & names)
  {
    size_t counter = 0;
    names.ForEach([&counter](int8_t, string_view) { ++counter; });
    return counter;
  };

  StringUtf8Multilang name;

  name.AddString(GetLangCode("default"), "Default name");
  name.AddString(GetLangCode("ru"), "Ru name");
  name.AddString(GetLangCode("en"), "En name");
  name.AddString(GetLangCode("de"), "De name");

  EditableMapObject emo;
  emo.SetName(name);
  emo.RemoveBlankNames();

  TEST_EQUAL(getCountOfNames(emo.GetNameMultilang()), 4, ());

  name.Clear();

  name.AddString(GetLangCode("default"), "");
  name.AddString(GetLangCode("ru"), "Ru name");
  name.AddString(GetLangCode("en"), "En name");
  name.AddString(GetLangCode("de"), "");

  emo.SetName(name);
  emo.RemoveBlankNames();

  TEST_EQUAL(getCountOfNames(emo.GetNameMultilang()), 2, ());

  name.Clear();

  name.AddString(GetLangCode("default"), "Default name");
  name.AddString(GetLangCode("ru"), "");
  name.AddString(GetLangCode("en"), "");
  name.AddString(GetLangCode("de"), "");

  emo.SetName(name);
  emo.RemoveBlankNames();

  TEST_EQUAL(getCountOfNames(emo.GetNameMultilang()), 1, ());

  name.Clear();

  name.AddString(GetLangCode("default"), "");
  name.AddString(GetLangCode("ru"), "");
  name.AddString(GetLangCode("en"), "");
  name.AddString(GetLangCode("de"), "De name");

  emo.SetName(name);
  emo.RemoveBlankNames();

  TEST_EQUAL(getCountOfNames(emo.GetNameMultilang()), 1, ());
}

UNIT_TEST(EditableMapObject_FromFeatureType)
{
  classificator::Load();

  EditableMapObject emo;
  SetTypes(emo, {{"amenity", "cafe"}});

  emo.SetHouseNumber("1");

  StringUtf8Multilang names;
  names.AddString(GetLangCode("default"), "Default name");
  names.AddString(GetLangCode("ru"), "Ru name");
  emo.SetName(names);

  emo.SetMetadata(feature::Metadata::FMD_WEBSITE, "https://some.thing.org");
  emo.SetInternet(feature::Internet::Wlan);

  emo.SetPointType();
  emo.SetMercator(m2::PointD(1.0, 1.0));

  auto ft = FeatureType::CreateFromMapObject(emo);
  EditableMapObject emo2;
  emo2.SetFromFeatureType(*ft);

  TEST(emo.GetTypes().Equals(emo2.GetTypes()), ());

  TEST_EQUAL(emo.GetNameMultilang(), emo2.GetNameMultilang(), ());
  TEST_EQUAL(emo.GetHouseNumber(), emo2.GetHouseNumber(), ());
  TEST_EQUAL(emo.GetMercator(), emo2.GetMercator(), ());
  TEST_EQUAL(emo.GetMetadata(feature::Metadata::FMD_WEBSITE), emo2.GetMetadata(feature::Metadata::FMD_WEBSITE), ());
  TEST_EQUAL(emo.GetInternet(), emo2.GetInternet(), ());

  TEST(emo.IsPointType(), ());
  TEST(emo2.IsPointType(), ());
}

UNIT_TEST(EditableMapObject_GetLocalizedAllTypes)
{
  classificator::Load();

  {
    EditableMapObject emo;
    SetTypes(emo, {{"amenity", "fuel"}, {"shop"}, {"building"}, {"toilets", "yes"}});
    TEST_EQUAL(emo.GetLocalizedAllTypes(true), "amenity-fuel • shop", ());
    TEST_EQUAL(emo.GetLocalizedAllTypes(false), "shop", ());
  }

  {
    EditableMapObject emo;
    SetTypes(emo, {{"amenity", "shelter"}, {"amenity", "bench"}, {"highway", "bus_stop"}});
    TEST_EQUAL(emo.GetLocalizedAllTypes(true), "highway-bus_stop • amenity-shelter • amenity-bench", ());
    TEST_EQUAL(emo.GetLocalizedAllTypes(false), "amenity-shelter • amenity-bench", ());
  }

  {
    EditableMapObject emo;
    SetTypes(emo, {{"leisure", "pitch"}, {"sport", "soccer"}});
    TEST_EQUAL(emo.GetLocalizedAllTypes(true), "sport-soccer • leisure-pitch", ());
    TEST_EQUAL(emo.GetLocalizedAllTypes(false), "leisure-pitch", ());
  }

  {
    EditableMapObject emo;
    SetTypes(emo, {{"craft", "key_cutter"}});
    TEST_EQUAL(emo.GetLocalizedAllTypes(true), "craft-key_cutter", ());
    TEST_EQUAL(emo.GetLocalizedAllTypes(false), "", ());
  }

  {
    EditableMapObject emo;
    SetTypes(emo, {{"amenity", "parking_entrance"}, {"barrier", "gate"}});
    TEST_EQUAL(emo.GetLocalizedAllTypes(true), "barrier-gate • amenity-parking_entrance", ());
    TEST_EQUAL(emo.GetLocalizedAllTypes(false), "amenity-parking_entrance", ());
  }

  {
    EditableMapObject emo;
    SetTypes(emo, {{"barrier", "gate"}});
    TEST_EQUAL(emo.GetLocalizedAllTypes(true), "barrier-gate", ());
    TEST_EQUAL(emo.GetLocalizedAllTypes(false), "", ());
  }

  {
    EditableMapObject emo;
    SetTypes(emo, {{"entrance", "main"}});
    TEST_EQUAL(emo.GetLocalizedAllTypes(true), "entrance-main", ());
    TEST_EQUAL(emo.GetLocalizedAllTypes(false), "", ());
  }

  {
    EditableMapObject emo;
    SetTypes(emo, {{"entrance", "main"}, {"barrier", "gate"}});
    TEST_EQUAL(emo.GetLocalizedAllTypes(true), "barrier-gate", ());
    TEST_EQUAL(emo.GetLocalizedAllTypes(false), "", ());
  }

  {
    EditableMapObject emo;
    SetTypes(emo, {{"amenity"}});
    TEST_EQUAL(emo.GetLocalizedAllTypes(true), "amenity", ());
    TEST_EQUAL(emo.GetLocalizedAllTypes(false), "", ());
  }

  {
    EditableMapObject emo;
    SetTypes(emo, {{"shop"}});
    TEST_EQUAL(emo.GetLocalizedAllTypes(true), "shop", ());
    TEST_EQUAL(emo.GetLocalizedAllTypes(false), "", ());
  }

  {
    EditableMapObject emo;
    SetTypes(emo, {{"tourism", "artwork"}, {"amenity"}});
    TEST_EQUAL(emo.GetLocalizedAllTypes(true), "tourism-artwork", ());
    TEST_EQUAL(emo.GetLocalizedAllTypes(false), "", ());
  }
}

}  // namespace editable_map_object_test
