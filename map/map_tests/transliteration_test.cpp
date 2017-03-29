#include "testing/testing.hpp"

#include "coding/multilang_utf8_string.hpp"
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

  TestTransliteration(translit, "ar", "العربية", "ạlʿrbyẗ");
  TestTransliteration(translit, "ru", "Русский", "Russkiy");
  TestTransliteration(translit, "zh", "中文", "zhōng wén");
  TestTransliteration(translit, "be", "Беларуская", "Byelaruskaya");
  TestTransliteration(translit, "ka", "ქართული", "kartuli");
  TestTransliteration(translit, "ko", "한국어", "hangug-eo");
  TestTransliteration(translit, "he", "עברית", "‘vryt");
  TestTransliteration(translit, "el", "Ελληνικά", "Ellēniká");
  TestTransliteration(translit, "zh_pinyin", "拼音", "pīn yīn");
  TestTransliteration(translit, "th", "ไทย", "thịy");
  TestTransliteration(translit, "sr", "Српски", "Srpski");
  TestTransliteration(translit, "uk", "Українська", "Ukrayinsʹka");
  TestTransliteration(translit, "fa", "فارسی", "fạrsy̰");
  TestTransliteration(translit, "hy", "Հայերէն", "Hayerēn");
  TestTransliteration(translit, "kn", "ಕನ್ನಡ", "kannaḍa");
  TestTransliteration(translit, "am", "አማርኛ", "āmarinya");
  TestTransliteration(translit, "ja_kana", "カタカナ", "katakana");
  TestTransliteration(translit, "bg", "Български", "Bŭlgarski");
  TestTransliteration(translit, "kk", "Қазақ", "Qazaq");
  TestTransliteration(translit, "mn", "Монгол хэл", "Mongol hel");
  TestTransliteration(translit, "mk", "Македонски", "Makedonski");
  TestTransliteration(translit, "hi", "हिन्दी", "hindī");
}
