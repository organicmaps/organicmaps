#include "../../testing/testing.hpp"

#include "../../std/string.hpp"
#include "../../std/vector.hpp"

#include "../preferred_languages.hpp"

namespace languages
{
  void FilterLanguages(vector<string> & langs);
}

UNIT_TEST(LangFilter)
{
  vector<string> v;
  v.push_back("en");
  v.push_back("en-GB");
  v.push_back("zh");
  v.push_back("es-SP");
  v.push_back("zh-penyn");
  v.push_back("en-US");
  v.push_back("ru_RU");
  v.push_back("es");

  languages::FilterLanguages(v);

  vector<string> c;
  c.push_back("en");
  c.push_back("zh");
  c.push_back("es");
  c.push_back("ru");

  TEST_EQUAL(v.size(), c.size(), (v, c));
  for (size_t i = 0; i < c.size(); ++i)
  {
    TEST_EQUAL(c[i], v[i], (v, c));
  }
}

UNIT_TEST(PrefLanguagesSmoke)
{
  string const prefLangs = languages::PreferredLanguages();
  TEST_GREATER(prefLangs.size(), 0, ());
  cout << "Preferred langs: " << prefLangs << endl;
  string const currLang = languages::CurrentLanguage();
  TEST_GREATER(currLang.size(), 0, ());
  cout << "Current lang: " << currLang << endl;
}
