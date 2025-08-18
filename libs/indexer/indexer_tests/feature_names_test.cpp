#include "testing/testing.hpp"

#include "indexer/feature_meta.hpp"
#include "indexer/feature_utils.hpp"

#include "coding/string_utf8_multilang.hpp"

#include <string>

namespace feature_names_test
{
using namespace std;

using StrUtf8 = StringUtf8Multilang;

void GetPreferredNames(feature::RegionData const & regionData, StrUtf8 const & src, int8_t deviceLang,
                       bool allowTranslit, std::string & primary, std::string & secondary)
{
  feature::NameParamsIn in{src, regionData, deviceLang, allowTranslit};
  feature::NameParamsOut out;
  feature::GetPreferredNames(in, out);
  primary = out.GetPrimary();
  secondary = out.secondary;
}

UNIT_TEST(GetPrefferedNames)
{
  feature::RegionData regionData;
  regionData.SetLanguages({"de", "ko"});

  int8_t deviceLang = StrUtf8::GetLangIndex("ru");
  string primary, secondary;
  bool const allowTranslit = false;

  {
    StrUtf8 src;
    src.AddString("fr", "fr name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "", ());
    TEST_EQUAL(secondary, "", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "default name", ());
    TEST_EQUAL(secondary, "", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("en", "en name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "en name", ());
    TEST_EQUAL(secondary, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "ru name", ());
    TEST_EQUAL(secondary, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "same name");
    src.AddString("en", "en name");
    src.AddString("ru", "same name");
    src.AddString("int_name", "int name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "same name", ());
    TEST_EQUAL(secondary, "", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");
    src.AddString("int_name", "int name");
    src.AddString("de", "de name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "ru name", ());
    TEST_EQUAL(secondary, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");
    src.AddString("int_name", "int name");
    src.AddString("de", "de name");
    src.AddString("ko", "ko name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "ru name", ());
    TEST_EQUAL(secondary, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");
    src.AddString("de", "de name");
    src.AddString("ko", "ko name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "int name", ());
    TEST_EQUAL(secondary, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");
    src.AddString("de", "de name");
    src.AddString("ko", "ko name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "int name", ());
    TEST_EQUAL(secondary, "", ());
  }

  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("de", "de name");
    src.AddString("ko", "ko name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "en name", ());
    TEST_EQUAL(secondary, "de name", ());
  }

  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("ko", "ko name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "en name", ());
    TEST_EQUAL(secondary, "ko name", ());
  }

  {
    StrUtf8 src;
    src.AddString("en", "en name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "en name", ());
    TEST_EQUAL(secondary, "", ());
  }
  {
    int8_t deviceLang = StrUtf8::GetLangIndex("be");
    StrUtf8 src;
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "ru name", ());
    TEST_EQUAL(secondary, "int name", ());
  }
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"ru"});
    int8_t deviceLang = StrUtf8::GetLangIndex("be");
    StrUtf8 src;
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "ru name", ());
    TEST_EQUAL(secondary, "", ());
  }
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"ru"});
    int8_t deviceLang = StrUtf8::GetLangIndex("be");
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "default name", ());
    TEST_EQUAL(secondary, "", ());
  }
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"ru"});
    int8_t deviceLang = StrUtf8::GetLangIndex("ru");
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("be", "be name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "default name", ());
    TEST_EQUAL(secondary, "", ());
  }
}

UNIT_TEST(GetPrefferedNamesLocal)
{
  string primary, secondary;
  bool const allowTranslit = true;
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"kk", "ru"});

    int8_t deviceLang = StrUtf8::GetLangIndex("ru");

    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("en", "en name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "default name", ());
    TEST_EQUAL(secondary, "", ());
  }
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"kk", "be"});

    int8_t deviceLang = StrUtf8::GetLangIndex("be");

    StrUtf8 src;
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "ru name", ());
    TEST_EQUAL(secondary, "", ());
  }
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"kk", "ru"});

    int8_t deviceLang = StrUtf8::GetLangIndex("ru");

    StrUtf8 src;
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("be", "be name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "int name", ());
    TEST_EQUAL(secondary, "", ());
  }
}

void GetReadableName(feature::RegionData const & regionData, StrUtf8 const & src, int8_t deviceLang, bool allowTranslit,
                     std::string & name)
{
  feature::NameParamsIn in{src, regionData, deviceLang, allowTranslit};
  feature::NameParamsOut out;
  feature::GetPreferredNames(in, out);
  name = out.GetPrimary();
}

UNIT_TEST(GetReadableName)
{
  feature::RegionData regionData;
  regionData.SetLanguages({"de", "ko"});

  int8_t deviceLang = StrUtf8::GetLangIndex("ru");
  bool const allowTranslit = false;
  string name;

  {
    StrUtf8 src;
    src.AddString("fr", "fr name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "ko name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "de name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    src.AddString("default", "default name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    src.AddString("default", "default name");
    src.AddString("en", "en name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "en name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "int name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");
    src.AddString("ru", "ru name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "ru name", ());
  }

  deviceLang = StrUtf8::GetLangIndex("de");

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "de name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "int name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("en", "en name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "en name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "ko name", ());
  }
  {
    int8_t deviceLang = StrUtf8::GetLangIndex("be");
    StrUtf8 src;
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "ru name", ());
  }
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"ru"});
    int8_t deviceLang = StrUtf8::GetLangIndex("be");
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "default name", ());
  }
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"ru"});
    int8_t deviceLang = StrUtf8::GetLangIndex("ru");
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("be", "be name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "default name", ());
  }
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"ru"});
    int8_t deviceLang = StrUtf8::GetLangIndex("ru");
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("be", "be name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "en name", ());
  }
}

/*
UNIT_TEST(GetNameForSearchOnBooking)
{
  {
    StrUtf8 src;
    feature::RegionData regionData;
    string result;
    auto lang = feature::GetNameForSearchOnBooking(regionData, src, result);
    TEST_EQUAL(lang, StrUtf8::kUnsupportedLanguageCode, ());
    TEST(result.empty(), ());
  }
  {
    StrUtf8 src;
    src.AddString("default", "default name");
    feature::RegionData regionData;
    string result;
    auto lang = feature::GetNameForSearchOnBooking(regionData, src, result);
    TEST_EQUAL(lang, StrUtf8::kDefaultCode, ());
    TEST_EQUAL(result, "default name", ());
  }
  {
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("ko", "ko name");
    src.AddString("en", "en name");
    feature::RegionData regionData;
    regionData.SetLanguages({"ko", "en"});
    string result;
    auto lang = feature::GetNameForSearchOnBooking(regionData, src, result);
    TEST_EQUAL(lang, StrUtf8::kDefaultCode, ());
    TEST_EQUAL(result, "default name", ());
  }
  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("ko", "ko name");
    feature::RegionData regionData;
    regionData.SetLanguages({"ko"});
    string result;
    auto lang = feature::GetNameForSearchOnBooking(regionData, src, result);
    TEST_EQUAL(lang, StrUtf8::GetLangIndex("ko"), ());
    TEST_EQUAL(result, "ko name", ());
  }
  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    feature::RegionData regionData;
    regionData.SetLanguages({"de", "ko"});
    string result;
    auto lang = feature::GetNameForSearchOnBooking(regionData, src, result);
    TEST_EQUAL(lang, StrUtf8::GetLangIndex("de"), ());
    TEST_EQUAL(result, "de name", ());
  }
  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("ko", "ko name");
    feature::RegionData regionData;
    regionData.SetLanguages({"de", "fr"});
    string result;
    auto lang = feature::GetNameForSearchOnBooking(regionData, src, result);
    TEST_EQUAL(lang, StrUtf8::GetLangIndex("en"), ());
    TEST_EQUAL(result, "en name", ());
  }
}
*/
}  // namespace feature_names_test
