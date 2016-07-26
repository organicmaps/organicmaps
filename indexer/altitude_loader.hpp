#pragma once
#include "indexer/feature_altitude.hpp"
#include "indexer/index.hpp"

#include "coding/memory_region.hpp"

#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

#include "3party/succinct/rs_bit_vector.hpp"

namespace feature
{
class AltitudeLoader
{
public:
  explicit AltitudeLoader(MwmValue const & mwmValue);

  /// \returns altitude of feature with |featureId|. All items of the returned vertor are valid
  /// or the returned vertor is empty.
  TAltitudes const & GetAltitudes(uint32_t featureId, size_t pointCount);

private:
  bool HasAltitudes() const;

  unique_ptr<CopiedMemoryRegion> m_altitudeAvailabilityRegion;
  unique_ptr<CopiedMemoryRegion> m_featureTableRegion;

  succinct::rs_bit_vector m_altitudeAvailability;
  succinct::elias_fano m_featureTable;

  unique_ptr<FilesContainerR::TReader> m_reader;
  map<uint32_t, TAltitudes> m_cache;
  AltitudeHeader m_header;
};
}  // namespace feature
