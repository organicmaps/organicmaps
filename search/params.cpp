#include "params.hpp"

#include "../coding/multilang_utf8_string.hpp"


namespace search
{

SearchParams::SearchParams()
: m_inputLanguageCode(StringUtf8Multilang::UNSUPPORTED_LANGUAGE_CODE),
  m_searchMode(ALL), m_forceSearch(false), m_validPos(false)
{
}

void SearchParams::SetPosition(double lat, double lon)
{
  m_lat = lat;
  m_lon = lon;
  m_validPos = true;
}

void SearchParams::SetInputLanguage(string const & language)
{
  /// @todo take into an account zh_pinyin, ko_rm and ja_rm
  size_t const delimPos = language.find_first_of("-_");

  m_inputLanguageCode = StringUtf8Multilang::GetLangIndex(
        delimPos == string::npos ? language : language.substr(0, delimPos));
}

bool SearchParams::IsLanguageValid() const
{
  return (m_inputLanguageCode != StringUtf8Multilang::UNSUPPORTED_LANGUAGE_CODE);
}

bool SearchParams::IsEqualCommon(SearchParams const & rhs) const
{
  return (m_query == rhs.m_query &&
          m_inputLanguageCode == rhs.m_inputLanguageCode &&
          m_validPos == rhs.m_validPos &&
          m_searchMode == rhs.m_searchMode);
}

string DebugPrint(SearchParams const & params)
{
  string s = ("search::SearchParams: ");
  s += "Query = " + params.m_query;
  s += "Mode = " + params.m_searchMode;
  return s;
}

} // namespace search
