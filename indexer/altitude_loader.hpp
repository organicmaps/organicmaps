#pragma once
#include "indexer/feature_altitude.hpp"
#include "indexer/index.hpp"

#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

#include "3party/succinct/rs_bit_vector.hpp"

namespace feature
{
class AltitudeLoader
{
public:
  explicit AltitudeLoader(MwmValue const & mwmValue);

  TAltitudes const & GetAltitudes(uint32_t featureId, size_t pointCount) const;
  bool IsAvailable() const;

private:
  vector<char> m_altitudeAvailabilitBuf;
  vector<char> m_featureTableBuf;
  succinct::rs_bit_vector m_altitudeAvailability;
  succinct::elias_fano m_featureTable;
  unique_ptr<FilesContainerR::TReader> m_reader;
  mutable map<uint32_t, TAltitudes> m_cache;
  TAltitudes const m_dummy;
  AltitudeHeader m_header;
};
} // namespace feature
