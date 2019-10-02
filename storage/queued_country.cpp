#include "storage/queued_country.hpp"

#include "base/assert.hpp"

namespace storage
{
QueuedCountry::QueuedCountry(CountryId const & countryId, MapFileType type)
  : m_countryId(countryId), m_fileType(type)
{
  ASSERT(IsCountryIdValid(GetCountryId()), ("Only valid countries may be downloaded."));
}

void QueuedCountry::SetFileType(MapFileType type)
{
  m_fileType = type;
}

MapFileType QueuedCountry::GetFileType() const
{
  return m_fileType;
}

CountryId const & QueuedCountry::GetCountryId() const
{
  return m_countryId;
}

bool QueuedCountry::operator==(CountryId const & countryId) const
{
  return m_countryId == countryId;
}
}  // namespace storage
