#pragma once

#include "storage/storage_defines.hpp"

#include "platform/country_defines.hpp"

namespace storage
{
class QueuedCountry
{
public:
  QueuedCountry(CountryId const & m_countryId, MapOptions opt);

  void SetDownloadingType(MapOptions type);
  MapOptions GetDownloadingType() const;

  CountryId const & GetCountryId() const { return m_countryId; }

  inline bool operator==(CountryId const & countryId) const { return m_countryId == countryId; }

private:
  CountryId m_countryId;
  MapOptions m_downloadingType;
};
}  // namespace storage
