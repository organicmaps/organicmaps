#include "testing/testing.hpp"

#include "coding/string_utf8_multilang.hpp"
#include "coding/transliteration.hpp"

#include "platform/platform.hpp"

namespace
{
void TestTransliteration(Transliteration const & translit, std::string const & locale,
                         std::string const & original, std::string const & expected)
{
  std::string out;
  translit.Transliterate(original, StringUtf8Multilang::GetLangIndex(locale), out);
  TEST_EQUAL(expected, out, ());
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
  TestTransliteration(translit, "th", "ไทย", ""); // Thai-Latin transliteration is off.
  TestTransliteration(translit, "sr", "Српски", "Srpski");
  TestTransliteration(translit, "uk", "Українська", "Ukrayinska");
  TestTransliteration(translit, "fa", "فارسی", "farsy");
  TestTransliteration(translit, "hy", "Հայերէն", "Hayeren");
  TestTransliteration(translit, "am", "አማርኛ", "amarinya");
  TestTransliteration(translit, "ja_kana", "カタカナ", "katakana");
  TestTransliteration(translit, "bg", "Български", "Bulgarski");
  TestTransliteration(translit, "kk", "Қазақ", "Qazaq");
  TestTransliteration(translit, "mn", "Монгол хэл", "Mongol hel");
  TestTransliteration(translit, "mk", "Македонски", "Makedonski");
  TestTransliteration(translit, "hi", "हिन्दी", "hindi");
}
