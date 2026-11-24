#pragma once
#include "indexer/feature_altitude.hpp"

#include "coding/files_container.hpp"
#include "coding/memory_region.hpp"

#include "geometry/point_with_altitude.hpp"

#include "3party/succinct/elias_fano.hpp"
#include "3party/succinct/rs_bit_vector.hpp"

#include <memory>
#include <string>

class MwmValue;

namespace feature
{
class AltitudeLoaderBase
{
public:
  explicit AltitudeLoaderBase(MwmValue const & mwmValue);

  struct Result
  {
    geometry::Altitudes m_altitudes;
    bool m_available = false;
  };

  /// Returns one numeric height per point. Availability is false for fallback values.
  Result GetAltitudes(uint32_t featureId, size_t pointCount);

  bool HasAltitudes() const;

private:
  std::unique_ptr<CopiedMemoryRegion> m_altitudeAvailabilityRegion;
  std::unique_ptr<CopiedMemoryRegion> m_featureTableRegion;

  succinct::rs_bit_vector m_altitudeAvailability;
  succinct::elias_fano m_featureTable;

  std::unique_ptr<FilesContainerR::TReader> m_reader;
  AltitudeHeader m_header;
  std::string m_countryFileName;
};

class AltitudeLoaderCached : public AltitudeLoaderBase
{
public:
  explicit AltitudeLoaderCached(MwmValue const & mwmValue) : AltitudeLoaderBase(mwmValue) {}

  /// Returns cached numeric heights, including fallbacks.
  geometry::Altitudes const & GetAltitudes(uint32_t featureId, size_t pointCount);

  void ClearCache() { m_cache.clear(); }

private:
  std::map<uint32_t, geometry::Altitudes> m_cache;
};
}  // namespace feature
