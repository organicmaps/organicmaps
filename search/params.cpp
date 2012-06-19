#include "params.hpp"

#include "../coding/multilang_utf8_string.hpp"


namespace search
{

SearchParams::SearchParams()
: m_mode(All),
  m_inputLanguageCode(StringUtf8Multilang::UNSUPPORTED_LANGUAGE_CODE),
  m_validPos(false)
{
}

void SearchParams::SetNearMeMode(bool b)
{
  m_mode = (b ? NearMe : All);
}

void SearchParams::SetPosition(double lat, double lon)
{
  m_lat = lat;
  m_lon = lon;
  m_validPos = true;
}

void SearchParams::SetInputLanguage(string const & language)
{
  // @TODO take into an account zh_pinyin, ko_rm and ja_rm
  size_t const delimPos = language.find_first_of("-_");

  m_inputLanguageCode = StringUtf8Multilang::GetLangIndex(
        delimPos == string::npos ? language : language.substr(0, delimPos));
}

bool SearchParams::IsNearMeMode() const
{
  // this mode is valid only with correct My Position
  return (m_mode == NearMe && m_validPos);
}

bool SearchParams::IsLanguageValid() const
{
  return (m_inputLanguageCode != StringUtf8Multilang::UNSUPPORTED_LANGUAGE_CODE);
}

bool SearchParams::IsEqualCommon(SearchParams const & rhs) const
{
  return (m_query == rhs.m_query && m_mode == rhs.m_mode &&
          m_inputLanguageCode == rhs.m_inputLanguageCode &&
          m_validPos == rhs.m_validPos);
}

} // namespace search
