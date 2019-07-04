#include "testing/testing.hpp"

#include "generator/translation.hpp"

#include "coding/transliteration.hpp"

#include "platform/platform.hpp"

#include <string>
#include <utility>
#include <vector>

using namespace generator;

// Transliteration tests ---------------------------------------------------------------------------
using Translations = std::vector<std::pair<std::string, std::string>>;
bool TestTransliteration(Translations const & translations,
                         std::string const & expectedTransliteration, std::string const & lang)
{
  StringUtf8Multilang name;
  for (auto const & langAndTranslation : translations)
  {
    name.AddString(langAndTranslation.first, langAndTranslation.second);
  }
  return GetTranslatedOrTransliteratedName(name, StringUtf8Multilang::GetLangIndex(lang)) ==
         expectedTransliteration;
}

UNIT_TEST(Transliteration)
{
  Transliteration & translit = Transliteration::Instance();
  translit.Init(GetPlatform().ResourcesDir());

  Translations const scotlandTranslations = {
      {"default", "Scotland"},  {"be", "Шатландыя"},  {"cs", "Skotsko"},   {"cy", "Yr Alban"},
      {"da", "Skotland"},       {"de", "Schottland"}, {"eo", "Skotlando"}, {"es", "Escocia"},
      {"eu", "Eskozia"},        {"fi", "Skotlanti"},  {"fr", "Écosse"},    {"ga", "Albain"},
      {"gd", "Alba"},           {"hr", "Škotska"},    {"ia", "Scotia"},    {"io", "Skotia"},
      {"ja", "スコットランド"}, {"ku", "Skotland"},   {"lfn", "Scotland"}, {"nl", "Schotland"},
      {"pl", "Szkocja"},        {"ru", "Шотландия"},  {"sco", "Scotland"}, {"sk", "Škótsko"},
      {"sr", "Шкотска"},        {"sv", "Skottland"},  {"tok", "Sukosi"},   {"tzl", "Escot"},
      {"uk", "Шотландія"},      {"vo", "Skotän"},     {"zh", "苏格兰"}};

  Translations const michiganTranslations = {
      {"default", "Michigan"}, {"ar", "ميشيغان"},    {"az", "Miçiqan"},   {"be", "Мічыган"},
      {"bg", "Мичиган"},       {"br", "Michigan"},   {"en", "Michigan"},  {"eo", "Miĉigano"},
      {"es", "Míchigan"},      {"fa", "میشیگان"},    {"haw", "Mikikana"}, {"he", "מישיגן"},
      {"hy", "Միչիգան"},       {"ja", "ミシガン州"}, {"ko", "미시간"},    {"lt", "Mičiganas"},
      {"lv", "Mičigana"},      {"nv", "Míshigin"},   {"pl", "Michigan"},  {"ru", "Мичиган"},
      {"sr", "Мичиген"},       {"ta", "மிச்சிகன்"},    {"th", "รัฐมิชิแกน"},   {"tl", "Misigan"},
      {"uk", "Мічиган"},       {"yi", "מישיגן"},     {"zh", "密歇根州"}};

  TEST(TestTransliteration(scotlandTranslations, "Scotland", "en"), ());
  TEST(TestTransliteration(michiganTranslations, "Michigan", "en"), ());
  TEST(TestTransliteration(scotlandTranslations, "Шотландия", "ru"), ());
  TEST(TestTransliteration(michiganTranslations, "Мичиган", "ru"), ());
}
