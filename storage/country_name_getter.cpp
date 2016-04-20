#include "storage/country_name_getter.hpp"

#include "base/assert.hpp"

namespace storage
{

void CountryNameGetter::SetLocale(string const & locale)
{
  m_getCurLang = platform::GetTextByIdFactory(platform::TextSource::Countries, locale);
}

void CountryNameGetter::SetLocaleForTesting(string const & jsonBuffer, string const & locale)
{
  m_getCurLang = platform::ForTestingGetTextByIdFactory(jsonBuffer, locale);
}

string CountryNameGetter::Get(string const & key) const
{
  ASSERT(!key.empty(), ());

  if (m_getCurLang == nullptr)
    return string();

  return (*m_getCurLang)(key);
}

string CountryNameGetter::operator()(TCountryId const & countryId) const
{
  string const name = Get(countryId);
  if (name.empty())
    return countryId;

  return name;
}

string CountryNameGetter::GetLocale() const
{
  if (m_getCurLang == nullptr)
    return string();

  return m_getCurLang->GetLocale();
}

}  // namespace storage
