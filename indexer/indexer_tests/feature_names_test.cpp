#include "testing/testing.hpp"

#include "indexer/feature_utils.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"
#include "platform/preferred_languages.hpp"

using namespace platform;

namespace
{
using StrUtf8 = StringUtf8Multilang;

UNIT_TEST(GetPrefferedNames)
{
  feature::RegionData regionData;
  regionData.SetLanguages({"de", "ko"});

  int8_t deviceLang = StrUtf8::GetLangIndex("ru");
  string primary, secondary;

  {
    StrUtf8 src;
    src.AddString("fr", "fr name");

    feature::GetPreferredNames(regionData, src, deviceLang, primary, secondary);

    TEST_EQUAL(primary, "", ());
    TEST_EQUAL(secondary, "", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");

    feature::GetPreferredNames(regionData, src, deviceLang, primary, secondary);

    TEST_EQUAL(primary, "default name", ());
    TEST_EQUAL(secondary, "", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("en", "en name");

    feature::GetPreferredNames(regionData, src, deviceLang, primary, secondary);

    TEST_EQUAL(primary, "en name", ());
    TEST_EQUAL(secondary, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    feature::GetPreferredNames(regionData, src, deviceLang, primary, secondary);

    TEST_EQUAL(primary, "ru name", ());
    TEST_EQUAL(secondary, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "same name");
    src.AddString("en", "en name");
    src.AddString("ru", "same name");
    src.AddString("int_name", "int name");

    feature::GetPreferredNames(regionData, src, deviceLang, primary, secondary);

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

    feature::GetPreferredNames(regionData, src, deviceLang, primary, secondary);

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

    feature::GetPreferredNames(regionData, src, deviceLang, primary, secondary);

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

    feature::GetPreferredNames(regionData, src, deviceLang, primary, secondary);

    TEST_EQUAL(primary, "int name", ());
    TEST_EQUAL(secondary, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");
    src.AddString("de", "de name");
    src.AddString("ko", "ko name");

    feature::GetPreferredNames(regionData, src, deviceLang, primary, secondary);

    TEST_EQUAL(primary, "int name", ());
    TEST_EQUAL(secondary, "", ());
  }

  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("de", "de name");
    src.AddString("ko", "ko name");

    feature::GetPreferredNames(regionData, src, deviceLang, primary, secondary);

    TEST_EQUAL(primary, "en name", ());
    TEST_EQUAL(secondary, "de name", ());
  }

  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("ko", "ko name");

    feature::GetPreferredNames(regionData, src, deviceLang, primary, secondary);

    TEST_EQUAL(primary, "en name", ());
    TEST_EQUAL(secondary, "ko name", ());
  }

  {
    StrUtf8 src;
    src.AddString("en", "en name");

    feature::GetPreferredNames(regionData, src, deviceLang, primary, secondary);

    TEST_EQUAL(primary, "en name", ());
    TEST_EQUAL(secondary, "", ());
  }
}

UNIT_TEST(GetReadableName)
{
  feature::RegionData regionData;
  regionData.SetLanguages({"de", "ko"});

  int8_t deviceLang = StrUtf8::GetLangIndex("ru");
  string name;

  {
    StrUtf8 src;
    src.AddString("fr", "fr name");

    feature::GetReadableName(regionData, src, deviceLang, name);

    TEST_EQUAL(name, "", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");

    feature::GetReadableName(regionData, src, deviceLang, name);

    TEST_EQUAL(name, "ko name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");

    feature::GetReadableName(regionData, src, deviceLang, name);

    TEST_EQUAL(name, "de name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    src.AddString("default", "default name");

    feature::GetReadableName(regionData, src, deviceLang, name);

    TEST_EQUAL(name, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    src.AddString("default", "default name");
    src.AddString("en", "en name");

    feature::GetReadableName(regionData, src, deviceLang, name);

    TEST_EQUAL(name, "en name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");

    feature::GetReadableName(regionData, src, deviceLang, name);

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

    feature::GetReadableName(regionData, src, deviceLang, name);

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

    feature::GetReadableName(regionData, src, deviceLang, name);

    TEST_EQUAL(name, "de name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");

    feature::GetReadableName(regionData, src, deviceLang, name);

    TEST_EQUAL(name, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");

    feature::GetReadableName(regionData, src, deviceLang, name);

    TEST_EQUAL(name, "int name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("en", "en name");

    feature::GetReadableName(regionData, src, deviceLang, name);

    TEST_EQUAL(name, "en name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");

    feature::GetReadableName(regionData, src, deviceLang, name);

    TEST_EQUAL(name, "ko name", ());
  }
}
}  // namespace
