#include "storage/queued_country.hpp"

#include "base/assert.hpp"

namespace storage
{
QueuedCountry::QueuedCountry(CountryId const & countryId, MapOptions opt)
  : m_countryId(countryId), m_downloadingType(opt)
{
  ASSERT(IsCountryIdValid(GetCountryId()), ("Only valid countries may be downloaded."));
}

void QueuedCountry::SetDownloadingType(MapOptions type)
{
  m_downloadingType = type;
}

MapOptions QueuedCountry::GetDownloadingType() const
{
  return m_downloadingType;
}
}  // namespace storage
