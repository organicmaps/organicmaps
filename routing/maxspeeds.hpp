#pragma once

#include "routing_common/maxspeed_conversion.hpp"

#include "indexer/mwm_set.hpp"

#include "coding/memory_region.hpp"
#include "coding/reader.hpp"
#include "coding/simple_dense_coding.hpp"

#include <memory>
#include <vector>

#include "3party/succinct/elias_fano.hpp"

namespace routing
{
class MaxspeedsSerializer;

class Maxspeeds
{
  friend class MaxspeedsSerializer;
public:
  /// \returns false if there's no maxspeeds (forward or bidirectional) and true otherwise.
  bool IsEmpty() const;

  /// \returns Maxspeed for feature id |fid|. If there's no Maxspeed value for |fid|
  /// returns an invalid Maxspeed value.
  Maxspeed GetMaxspeed(uint32_t fid) const;

  using ReaderT = ModelReaderPtr;
  void Load(ReaderT const & reader);

private:
  bool HasForwardMaxspeed(uint32_t fid) const;
  bool HasBidirectionalMaxspeed(uint32_t fid) const;

  // Feature IDs (compressed rank bit-vector).
  std::unique_ptr<CopiedMemoryRegion> m_forwardMaxspeedTableRegion;
  succinct::elias_fano m_forwardMaxspeedsTable;

  // Forward speeds (compressed uint8_t vector).
  std::unique_ptr<CopiedMemoryRegion> m_forwardMaxspeedRegion;
  coding::SimpleDenseCoding m_forwardMaxspeeds;

  std::vector<FeatureMaxspeed> m_bidirectionalMaxspeeds;
};

std::unique_ptr<Maxspeeds> LoadMaxspeeds(MwmSet::MwmHandle const & handle);
}  // namespace routing
