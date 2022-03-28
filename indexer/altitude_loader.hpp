#pragma once
#include "indexer/feature_altitude.hpp"
#include "indexer/mwm_set.hpp"

#include "coding/memory_region.hpp"

#include "geometry/point_with_altitude.hpp"

#include <memory>
#include <string>
#include <vector>

#include "3party/succinct/rs_bit_vector.hpp"

class DataSource;

namespace feature
{
class AltitudeLoaderBase
{
public:
  AltitudeLoaderBase(DataSource const & dataSource, MwmSet::MwmId const & mwmId);

  /// \returns altitude of feature with |featureId|. All items of the returned vector are valid
  /// or the returned vector is empty.
  geometry::Altitudes GetAltitudes(uint32_t featureId, size_t pointCount);

  bool HasAltitudes() const;

private:
  std::unique_ptr<CopiedMemoryRegion> m_altitudeAvailabilityRegion;
  std::unique_ptr<CopiedMemoryRegion> m_featureTableRegion;

  succinct::rs_bit_vector m_altitudeAvailability;
  succinct::elias_fano m_featureTable;

  std::unique_ptr<FilesContainerR::TReader> m_reader;
  AltitudeHeader m_header;
  std::string m_countryFileName;
  MwmSet::MwmHandle m_handle;
};

class AltitudeLoaderCached : public AltitudeLoaderBase
{
public:
  AltitudeLoaderCached(DataSource const & dataSource, MwmSet::MwmId const & mwmId)
    : AltitudeLoaderBase(dataSource, mwmId)
  {
  }

  /// \returns altitude of feature with |featureId|. All items of the returned vector are valid
  /// or the returned vector is empty.
  geometry::Altitudes const & GetAltitudes(uint32_t featureId, size_t pointCount);

  void ClearCache() { m_cache.clear(); }

private:
  std::map<uint32_t, geometry::Altitudes> m_cache;
};
}  // namespace feature
