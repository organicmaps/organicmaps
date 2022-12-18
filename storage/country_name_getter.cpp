#include "storage/country_name_getter.hpp"

#include "base/assert.hpp"

namespace storage
{
void CountryNameGetter::SetLocale(std::string const & locale)
{
  m_getCurLang = platform::GetTextByIdFactory(platform::TextSource::Countries, locale);
}

void CountryNameGetter::SetLocaleForTesting(std::string const & jsonBuffer, std::string const & locale)
{
  m_getCurLang = platform::ForTestingGetTextByIdFactory(jsonBuffer, locale);
}

std::string CountryNameGetter::Get(std::string const & key) const
{
  ASSERT(!key.empty(), ());

  if (m_getCurLang == nullptr)
    return std::string();

  return (*m_getCurLang)(key);
}

std::string CountryNameGetter::operator()(CountryId const & countryId) const
{
  std::string name = Get(countryId);
  if (name.empty())
    return countryId;

  return name;
}

std::string CountryNameGetter::GetLocale() const
{
  if (m_getCurLang == nullptr)
    return std::string();

  return m_getCurLang->GetLocale();
}
}  // namespace storage
