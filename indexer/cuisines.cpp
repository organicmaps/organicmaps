#include "indexer/cuisines.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

using namespace std;

namespace osm
{
// static
Cuisines & Cuisines::Instance()
{
  static Cuisines instance;
  return instance;
}

namespace
{
void InitializeCuisinesForLocale(platform::TGetTextByIdPtr & ptr, string const & lang)
{
  if (!ptr || ptr->GetLocale() != lang)
    ptr = GetTextByIdFactory(platform::TextSource::Cuisines, lang);
  CHECK(ptr, ("Error loading cuisines translations for", lang, "language."));
}

string TranslateImpl(platform::TGetTextByIdPtr const & ptr, string const & key)
{
  ASSERT(ptr, ("ptr should be initialized before calling this function."));
  return ptr->operator()(key);
}
}  // namespace

void Cuisines::Parse(string const & osmRawCuisinesTagValue, vector<string> & outCuisines)
{
  strings::Tokenize(osmRawCuisinesTagValue, ";", MakeBackInsertFunctor(outCuisines));
}

void Cuisines::ParseAndLocalize(string const & osmRawCuisinesTagValue, vector<string> & outCuisines,
                                string const & lang)
{
  Parse(osmRawCuisinesTagValue, outCuisines);
  InitializeCuisinesForLocale(m_translations, lang);
  for (auto & cuisine : outCuisines)
  {
    string tr = TranslateImpl(m_translations, cuisine);
    if (!tr.empty())
      cuisine = move(tr);
  }
}

string Cuisines::Translate(string const & singleOsmCuisine, string const & lang)
{
  ASSERT(singleOsmCuisine.find(';') == string::npos,
         ("Please call Parse method for raw OSM cuisine string."));
  InitializeCuisinesForLocale(m_translations, lang);
  return TranslateImpl(m_translations, singleOsmCuisine);
}

AllCuisines Cuisines::AllSupportedCuisines(string const & lang)
{
  InitializeCuisinesForLocale(m_translations, lang);
  return m_translations->GetAllSortedTranslations();
}
}  // namespace osm
