#pragma once

#include "indexer/feature_decl.hpp"

#include "base/exception.hpp"
#include "base/newtype.hpp"

#include <cstdint>
#include <string>
#include <vector>

class Index;

namespace openlr
{
NEWTYPE(uint32_t, PartnerSegmentId);

enum class ItemEvaluation
{
  Unevaluated,
  Positive,
  Negative,
  RelPositive,
  RelNegative,
  Ignore,
  NotAValue
};

ItemEvaluation ParseItemEvaluation(std::string const & s);
std::string ToString(ItemEvaluation const e);

struct SampleItem
{
  struct MWMSegment
  {
    MWMSegment(FeatureID const & fid, uint32_t const segId, bool const isForward,
               double const length)
      : m_fid(fid)
      , m_segId(segId)
      , m_isForward(isForward)
      , m_length(length)
    {
    }

    FeatureID const m_fid;
    uint32_t const m_segId;
    bool const m_isForward;
    double const m_length;
  };

  explicit SampleItem(PartnerSegmentId const partnerSegmentId,
                      ItemEvaluation const evaluation = ItemEvaluation::Unevaluated)
    : m_partnerSegmentId(partnerSegmentId)
    , m_evaluation(evaluation)
  {
  }

  static SampleItem Uninitialized() { return {}; }

  PartnerSegmentId m_partnerSegmentId;
  std::vector<MWMSegment> m_segments;
  ItemEvaluation m_evaluation;

private:
  SampleItem() = default;
};

DECLARE_EXCEPTION(SamplePoolLoadError, RootException);
DECLARE_EXCEPTION(SamplePoolSaveError, RootException);

using SamplePool = std::vector<SampleItem>;

SamplePool LoadSamplePool(std::string const & fileName, Index const & index);
void SaveSamplePool(std::string const & fileName, SamplePool const & sample,
                    bool const saveEvaluation);
}  // namespace openlr
