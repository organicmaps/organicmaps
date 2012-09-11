#include "params.hpp"

#include "../coding/multilang_utf8_string.hpp"


namespace search
{

SearchParams::SearchParams()
: m_inputLanguageCode(StringUtf8Multilang::UNSUPPORTED_LANGUAGE_CODE),
  m_validPos(false), m_mode(0)
{
}

void SearchParams::SetResetMode(bool b)
{
  if (b) m_mode |= ResetBit;
  else m_mode &= (~ResetBit);
}

bool SearchParams::IsResetMode() const
{
  return ((m_mode & ResetBit) != 0);
}

void SearchParams::SetNearMeMode(bool b)
{
  if (b) m_mode |= NearMeBit;
  else m_mode &= (~NearMeBit);
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

bool SearchParams::IsNearMeMode() const
{
  // this mode is valid only with correct My Position
  return (((m_mode & NearMeBit) != 0) && m_validPos);
}

bool SearchParams::IsLanguageValid() const
{
  return (m_inputLanguageCode != StringUtf8Multilang::UNSUPPORTED_LANGUAGE_CODE);
}

bool SearchParams::IsEqualCommon(SearchParams const & rhs) const
{
  // do not compare m_mode
  return (m_query == rhs.m_query &&
          m_inputLanguageCode == rhs.m_inputLanguageCode &&
          m_validPos == rhs.m_validPos);
}

string DebugPrint(SearchParams const & params)
{
  string s("search::SearchParams: ");
  s = s + "Query = " + params.m_query;
  return s;
}

} // namespace search
