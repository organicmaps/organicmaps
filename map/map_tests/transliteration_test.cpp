#include "testing/testing.hpp"

#include "coding/multilang_utf8_string.hpp"
#include "coding/transliteration.hpp"

#include "platform/platform.hpp"

// This test is inside of the map_tests because it uses Platform for obtaining the resource directory.
UNIT_TEST(Transliteration_CompareSamples)
{
  Transliteration & translit = Transliteration::GetInstance();
  translit.Init(GetPlatform().ResourcesDir());

  TEST_EQUAL("rì běn yǔ", translit.Transliterate("日本語", StringUtf8Multilang::GetLangIndex("ja")), ());
  TEST_EQUAL("ạlʿrbyẗ", translit.Transliterate("العربية", StringUtf8Multilang::GetLangIndex("ar")), ());
  TEST_EQUAL("Russkiy", translit.Transliterate("Русский", StringUtf8Multilang::GetLangIndex("ru")), ());
  TEST_EQUAL("zhōng wén", translit.Transliterate("中文", StringUtf8Multilang::GetLangIndex("zh")), ());
  TEST_EQUAL("Byelaruskaya", translit.Transliterate("Беларуская", StringUtf8Multilang::GetLangIndex("be")), ());
  TEST_EQUAL("kartuli", translit.Transliterate("ქართული", StringUtf8Multilang::GetLangIndex("ka")), ());
  TEST_EQUAL("hangug-eo", translit.Transliterate("한국어", StringUtf8Multilang::GetLangIndex("ko")), ());
  TEST_EQUAL("‘vryt", translit.Transliterate("עברית", StringUtf8Multilang::GetLangIndex("he")), ());
  TEST_EQUAL("Ellēniká", translit.Transliterate("Ελληνικά", StringUtf8Multilang::GetLangIndex("el")), ());
  TEST_EQUAL("pīn yīn", translit.Transliterate("拼音", StringUtf8Multilang::GetLangIndex("zh_pinyin")), ());
  TEST_EQUAL("thịy", translit.Transliterate("ไทย", StringUtf8Multilang::GetLangIndex("th")), ());
  TEST_EQUAL("Srpski", translit.Transliterate("Српски", StringUtf8Multilang::GetLangIndex("sr")), ());
  TEST_EQUAL("Ukrayinsʹka", translit.Transliterate("Українська", StringUtf8Multilang::GetLangIndex("uk")), ());
  TEST_EQUAL("fạrsy̰", translit.Transliterate("فارسی", StringUtf8Multilang::GetLangIndex("fa")), ());
  TEST_EQUAL("Hayerēn", translit.Transliterate("Հայերէն", StringUtf8Multilang::GetLangIndex("hy")), ());
  TEST_EQUAL("kannaḍa", translit.Transliterate("ಕನ್ನಡ", StringUtf8Multilang::GetLangIndex("kn")), ());
  TEST_EQUAL("āmarinya", translit.Transliterate("አማርኛ", StringUtf8Multilang::GetLangIndex("am")), ());
  TEST_EQUAL("katakana", translit.Transliterate("カタカナ", StringUtf8Multilang::GetLangIndex("ja_kana")), ());
  TEST_EQUAL("Bŭlgarski", translit.Transliterate("Български", StringUtf8Multilang::GetLangIndex("bg")), ());
  TEST_EQUAL("Qazaq", translit.Transliterate("Қазақ", StringUtf8Multilang::GetLangIndex("kk")), ());
  TEST_EQUAL("Mongol hel", translit.Transliterate("Монгол хэл", StringUtf8Multilang::GetLangIndex("mn")), ());
  TEST_EQUAL("Makedonski", translit.Transliterate("Македонски", StringUtf8Multilang::GetLangIndex("mk")), ());
  TEST_EQUAL("hindī", translit.Transliterate("हिन्दी", StringUtf8Multilang::GetLangIndex("hi")), ());
}
