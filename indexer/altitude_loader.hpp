#pragma once
#include "indexer/feature_altitude.hpp"
#include "indexer/mwm_set.hpp"

#include "coding/memory_region.hpp"

#include <memory>
#include <string>
#include <vector>

#include "3party/succinct/rs_bit_vector.hpp"

class DataSource;

namespace feature
{
// @TODO(bykoianko) |m_altitudeAvailability| and |m_featureTable| are saved without
// taking into account endianness. It should be fixed. The plan is
// * to use one bit form AltitudeHeader::m_version for keeping information about endianness. (Zero
//   should be used for big endian.)
// * to check the endianness of the reader and the bit while reading and to use an appropriate
//   methods for reading.
class AltitudeLoader
{
public:
  AltitudeLoader(DataSource const & dataSource, MwmSet::MwmId const & mwmId);

  /// \returns altitude of feature with |featureId|. All items of the returned vector are valid
  /// or the returned vector is empty.
  TAltitudes const & GetAltitudes(uint32_t featureId, size_t pointCount);

  bool HasAltitudes() const;

  void ClearCache() { m_cache.clear(); }

private:
  std::unique_ptr<CopiedMemoryRegion> m_altitudeAvailabilityRegion;
  std::unique_ptr<CopiedMemoryRegion> m_featureTableRegion;

  succinct::rs_bit_vector m_altitudeAvailability;
  succinct::elias_fano m_featureTable;

  std::unique_ptr<FilesContainerR::TReader> m_reader;
  std::map<uint32_t, TAltitudes> m_cache;
  AltitudeHeader m_header;
  std::string m_countryFileName;
  MwmSet::MwmHandle m_handle;
};
}  // namespace feature
