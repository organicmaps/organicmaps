#pragma once
#include "indexer/feature_altitude.hpp"
#include "indexer/index.hpp"

#include "coding/dd_vector.hpp"

namespace feature
{
class AltitudeLoader
{
public:
  AltitudeLoader(MwmValue const * mwmValue);
  Altitudes GetAltitudes(uint32_t featureId) const;

private:
  struct TAltitudeIndexEntry
  {
    uint32_t featureId;
    feature::TAltitude beginAlt;
    feature::TAltitude endAlt;
  };

  unique_ptr<DDVector<TAltitudeIndexEntry, FilesContainerR::TReader>> m_idx;
};
} // namespace feature
