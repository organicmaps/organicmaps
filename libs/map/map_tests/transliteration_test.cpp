#include "testing/testing.hpp"

#include "indexer/feature_meta.hpp"
#include "indexer/feature_utils.hpp"

#include "coding/string_utf8_multilang.hpp"
#include "coding/transliteration.hpp"

#include "platform/platform.hpp"

namespace
{
void TestTransliteration(Transliteration const & translit, std::string const & locale, std::string const & original,
                         std::string const & expected)
{
  std::string out;
  translit.Transliterate(original, StringUtf8Multilang::GetLangIndex(locale), out);
  TEST_EQUAL(expected, out, ());
}

void TestFeatureNames(feature::NameParamsIn const & in, std::string_view primary, int8_t primaryLang,
                      std::string_view secondary = {},
                      int8_t secondaryLang = StringUtf8Multilang::kUnsupportedLanguageCode)
{
  feature::NameParamsOut out;
  feature::GetPreferredNames(in, out);
  TEST_EQUAL(out.GetPrimary(), primary, (in.deviceLang));
  TEST_EQUAL(out.primaryLang, primaryLang, (in.deviceLang));
  TEST_EQUAL(out.secondary, secondary, (in.deviceLang));
  TEST_EQUAL(out.secondaryLang, secondaryLang, (in.deviceLang));

  feature::GetReadableName(in, out);
  TEST_EQUAL(out.GetPrimary(), primary, (in.deviceLang));
  TEST_EQUAL(out.primaryLang, primaryLang, (in.deviceLang));
  TEST(out.secondary.empty(), ());
  TEST_EQUAL(out.secondaryLang, StringUtf8Multilang::kUnsupportedLanguageCode, ());
}
}  // namespace

// This test is inside of the map_tests because it uses Platform for obtaining the resource directory.
UNIT_TEST(Transliteration_CompareSamples)
{
  Transliteration & translit = Transliteration::Instance();
  translit.Init(GetPlatform().ResourcesDir());

  TestTransliteration(translit, "ar", "العربية", "alrbyt");
  TestTransliteration(translit, "ru", "Русский", "Russkiy");
  TestTransliteration(translit, "zh", "中文", "zhong wen");
  TestTransliteration(translit, "be", "Беларуская", "Byelaruskaya");
  TestTransliteration(translit, "ka", "ქართული", "kartuli");
  TestTransliteration(translit, "ko", "한국어", "hangug-eo");
  TestTransliteration(translit, "he", "עברית", "bryt");
  TestTransliteration(translit, "el", "Ελληνικά", "Ellenika");
  TestTransliteration(translit, "zh_pinyin", "拼音", "pin yin");
  TestTransliteration(translit, "th", "ไทย", "");  // Thai-Latin transliteration is off.
  TestTransliteration(translit, "sr", "Српски", "Srpski");
  TestTransliteration(translit, "uk", "Українська", "Ukrayinska");
  TestTransliteration(translit, "fa", "فارسی", "farsy");
  TestTransliteration(translit, "hy", "Հայերէն", "Hayeren");
  TestTransliteration(translit, "am", "አማርኛ", "amarinya");
  TestTransliteration(translit, "ja_kana", "カタカナ", "katakana");
  TestTransliteration(translit, "ja_kana", "ひらがな", "hiragana");
  TestTransliteration(translit, "ja_kana", "カタカナ ひらがな", "katakana hiragana");
  TestTransliteration(translit, "bg", "Български", "Bulgarski");
  TestTransliteration(translit, "kk", "Қазақ", "Qazaq");
  TestTransliteration(translit, "mn", "Монгол хэл", "Mongol hel");
  TestTransliteration(translit, "mk", "Македонски", "Makedonski");
  TestTransliteration(translit, "hi", "हिन्दी", "hindi");
}

UNIT_TEST(Transliteration_FeatureNames)
{
  Transliteration::Instance().Init(GetPlatform().ResourcesDir());

  struct Sample
  {
    std::string language;
    std::string_view original;
    std::string_view latin;
  };
  Sample const samples[] = {
      {"zh", "鼓东路", "gu dong lù"},
      {"ru", "Тверская улица", "Tverskaya ulitsa"},
      {"zh", "A–B", "A–B"},  // ICU can return an unchanged name; do not duplicate it.
  };

  for (auto const & sample : samples)
  {
    feature::RegionData regionData;
    regionData.SetLanguages({sample.language});
    for (auto const nameLang : {StringUtf8Multilang::kDefaultCode, StringUtf8Multilang::GetLangIndex(sample.language)})
    {
      StringUtf8Multilang names;
      names.AddString(nameLang, sample.original);
      bool const duplicate = sample.original == sample.latin;
      for (auto const mapLang : {"en", "it"})
      {
        TestFeatureNames({names, regionData, mapLang, true /* allowTranslit */}, sample.latin,
                         StringUtf8Multilang::kEnglishCode, duplicate ? std::string_view{} : sample.original,
                         duplicate ? StringUtf8Multilang::kUnsupportedLanguageCode : nameLang);
      }
    }
  }
}

UNIT_TEST(Transliteration_FeatureNamesFallbacks)
{
  Transliteration::Instance().Init(GetPlatform().ResourcesDir());

  feature::RegionData regionData;
  regionData.SetLanguages({"zh"});
  StringUtf8Multilang names;
  names.AddString("default", "鼓东路");

  TestFeatureNames({names, regionData, "en", false /* allowTranslit */}, "鼓东路", StringUtf8Multilang::kDefaultCode);
  TestFeatureNames({names, regionData, "zh", true /* allowTranslit */}, "鼓东路", StringUtf8Multilang::kDefaultCode);

  for (auto const lang : {"en", "int_name", "it"})
  {
    auto translatedNames = names;
    translatedNames.AddString(lang, "Gudong Road");
    for (bool const allowTranslit : {false, true})
    {
      TestFeatureNames({translatedNames, regionData, "it", allowTranslit}, "Gudong Road",
                       StringUtf8Multilang::GetLangIndex(lang), "鼓东路", StringUtf8Multilang::kDefaultCode);
    }
  }

  names.Clear();
  names.AddString("default", "Gudong Road");
  TestFeatureNames({names, regionData, "en", true /* allowTranslit */}, "Gudong Road",
                   StringUtf8Multilang::kDefaultCode);

  feature::RegionData japan;
  japan.SetLanguages({"ja"});
  names.Clear();
  names.AddString("default", "かずら橋");
  TestFeatureNames({names, japan, "en", true /* allowTranslit */}, "かずら橋", StringUtf8Multilang::kDefaultCode);
}
