#pragma once
#include "indexer/feature_altitude.hpp"
#include "indexer/index.hpp"

#include "3party/succinct/rs_bit_vector.hpp"

#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace feature
{
class AltitudeLoader
{
public:
  explicit AltitudeLoader(MwmValue const * mwmValue);

  TAltitudes GetAltitude(uint32_t featureId, size_t pointCount) const;
  bool IsAvailable() const;

private:
  vector<char> m_altitudeAvailabilitBuf;
  vector<char> m_featureTableBuf;
  unique_ptr<succinct::rs_bit_vector> m_altitudeAvailability;
  unique_ptr<succinct::elias_fano> m_featureTable;
  unique_ptr<FilesContainerR::TReader> m_reader;
  AltitudeHeader m_header;
};
} // namespace feature
