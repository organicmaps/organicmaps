#include "testing/testing.hpp"

#include "platform/preferred_languages.hpp"

#include "base/logging.hpp"

#include <cstddef>
#include <string>

UNIT_TEST(LangNormalize_Smoke)
{
  char const * arr1[] = {"en", "en-GB", "zh", "es-SP", "zh-penyn", "en-US", "ru_RU", "es"};
  char const * arr2[] = {"en", "en", "zh", "es", "zh", "en", "ru", "es"};
  static_assert(ARRAY_SIZE(arr1) == ARRAY_SIZE(arr2), "");

  for (size_t i = 0; i < ARRAY_SIZE(arr1); ++i)
    TEST_EQUAL(arr2[i], languages::Normalize(arr1[i]), ());
}

UNIT_TEST(LangGetChineseScript)
{
  using languages::ChineseScript;
  using languages::GetChineseScript;

  TEST_EQUAL(GetChineseScript("zh"), ChineseScript::Simplified, ());
  TEST_EQUAL(GetChineseScript("zh-Hans"), ChineseScript::Simplified, ());
  TEST_EQUAL(GetChineseScript("zh_CN"), ChineseScript::Simplified, ());
  TEST_EQUAL(GetChineseScript("zh-Hans-CN"), ChineseScript::Simplified, ());
  TEST_EQUAL(GetChineseScript("zh-SG"), ChineseScript::Simplified, ());

  TEST_EQUAL(GetChineseScript("zh-Hant"), ChineseScript::Traditional, ());
  TEST_EQUAL(GetChineseScript("zh_TW"), ChineseScript::Traditional, ());
  TEST_EQUAL(GetChineseScript("zh-HK"), ChineseScript::Traditional, ());
  TEST_EQUAL(GetChineseScript("zh-MO"), ChineseScript::Traditional, ());
  TEST_EQUAL(GetChineseScript("ZH-TW"), ChineseScript::Traditional, ());

  // Android resource qualifiers spell the region with an "r" prefix.
  TEST_EQUAL(GetChineseScript("zh-rTW"), ChineseScript::Traditional, ());
  TEST_EQUAL(GetChineseScript("zh_rCN"), ChineseScript::Simplified, ());

  // POSIX locales from $LANG carry a charset suffix and may carry an @modifier.
  TEST_EQUAL(GetChineseScript("zh_TW.UTF-8"), ChineseScript::Traditional, ());
  TEST_EQUAL(GetChineseScript("zh_HK.UTF-8"), ChineseScript::Traditional, ());
  TEST_EQUAL(GetChineseScript("zh_MO.UTF-8"), ChineseScript::Traditional, ());
  TEST_EQUAL(GetChineseScript("zh_CN.UTF-8"), ChineseScript::Simplified, ());

  TEST_EQUAL(GetChineseScript("en"), ChineseScript::NotChinese, ());
  TEST_EQUAL(GetChineseScript("en_UA"), ChineseScript::NotChinese, ());
  TEST_EQUAL(GetChineseScript(""), ChineseScript::NotChinese, ());
  // Primary subtag is matched exactly, so Zhuang is not Chinese.
  TEST_EQUAL(GetChineseScript("zha"), ChineseScript::NotChinese, ());
  // "mo"/"hk" must be whole subtags, not substrings.
  TEST_EQUAL(GetChineseScript("zh-Mong"), ChineseScript::Simplified, ());
  TEST_EQUAL(GetChineseScript("zh-Hank"), ChineseScript::Simplified, ());
}

// Android 14+ stores regional preferences (first day of week, measurement system, temperature
// unit) as Unicode locale extensions, and Locale.toString() surfaces them as
// "en_UA_#u-fw-mon-ms-metric-mu-celsius". The "mon" value must not be read as Macau ("mo").
UNIT_TEST(LangAndroidRegionalPreferences)
{
  using languages::ChineseScript;
  using languages::GetChineseScript;

  TEST_EQUAL(languages::Normalize("en_UA_#u-fw-mon-ms-metric-mu-celsius"), "en", ());
  TEST_EQUAL(languages::GetTwine("en_UA_#u-fw-mon-ms-metric-mu-celsius"), "en", ());

  // Simplified Chinese must survive every regional preference, Monday included.
  for (char const * tag : {"zh_CN_#u-fw-mon", "zh_CN_#u-fw-mon-ms-metric-mu-celsius", "zh_CN_#Hans-u-fw-mon",
                           "zh_CN_#Hans-u-fw-sun", "zh_CN_#Hans"})
  {
    TEST_EQUAL(GetChineseScript(tag), ChineseScript::Simplified, (tag));
    TEST_EQUAL(languages::GetTwine(tag), "zh-Hans", (tag));
  }

  // Traditional Chinese must stay Traditional with the same extensions attached.
  for (char const * tag : {"zh_TW_#Hant-u-fw-mon", "zh_TW_#u-fw-sun", "zh_HK_#Hant-u-fw-mon"})
  {
    TEST_EQUAL(GetChineseScript(tag), ChineseScript::Traditional, (tag));
    TEST_EQUAL(languages::GetTwine(tag), "zh-Hant", (tag));
  }
}

UNIT_TEST(PrefLanguages_Smoke)
{
  std::string s = languages::GetPreferred();
  TEST(!s.empty(), ());
  LOG(LINFO, ("Preferred langs:", s));

  s = languages::GetCurrentOrig();
  TEST(!s.empty(), ());
  LOG(LINFO, ("Current original lang:", s));

  s = languages::GetCurrentNorm();
  TEST(!s.empty(), ());
  LOG(LINFO, ("Current normalized lang:", s));
}
