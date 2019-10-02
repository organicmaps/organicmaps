#pragma once

#include "storage/storage_defines.hpp"

#include "platform/country_defines.hpp"

namespace storage
{
class QueuedCountry
{
public:
  QueuedCountry(CountryId const & m_countryId, MapFileType type);

  void SetFileType(MapFileType type);
  MapFileType GetFileType() const;

  CountryId const & GetCountryId() const;
  bool operator==(CountryId const & countryId) const;

private:
  CountryId m_countryId;
  MapFileType m_fileType;
};
}  // namespace storage
