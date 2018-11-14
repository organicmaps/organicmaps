#pragma once

#include "routing_common/maxspeed_conversion.hpp"

#include "indexer/mwm_set.hpp"

#include "coding/file_container.hpp"
#include "coding/memory_region.hpp"
#include "coding/reader.hpp"
#include "coding/simple_dense_coding.hpp"

#include <cstdint>
#include <memory>
#include <vector>

#include "3party/succinct/elias_fano.hpp"

class DataSource;

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

private:
  bool HasForwardMaxspeed(uint32_t fid) const;
  bool HasBidirectionalMaxspeed(uint32_t fid) const;

  std::unique_ptr<CopiedMemoryRegion> m_forwardMaxspeedTableRegion;
  // |m_forwardMaxspeed| contains true for feature ids which have only forward (single directional)
  // maxspeed.
  succinct::elias_fano m_forwardMaxspeedsTable;
  std::unique_ptr<CopiedMemoryRegion> m_forwardMaxspeedRegion;
  coding::SimpleDenseCoding m_forwardMaxspeeds;
  std::vector<FeatureMaxspeed> m_bidirectionalMaxspeeds;
};

void LoadMaxspeeds(FilesContainerR::TReader const & reader, Maxspeeds & maxspeeds);
std::unique_ptr<Maxspeeds> LoadMaxspeeds(DataSource const & dataSource, MwmSet::MwmHandle const & handle);
}  // namespace routing
