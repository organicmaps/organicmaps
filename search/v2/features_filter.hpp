#pragma once

#include "indexer/mwm_set.hpp"

#include "coding/compressed_bit_vector.hpp"

#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"

#include "std/algorithm.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

class MwmValue;

namespace search
{
namespace v2
{
class FeaturesFilter
{
public:
  FeaturesFilter(my::Cancellable const & cancellable);

  void SetValue(MwmValue * value, MwmSet::MwmId const & id);
  void SetViewport(m2::RectD const & viewport);
  void SetMaxNumResults(size_t maxNumResults);
  void SetScale(int scale);

  bool NeedToFilter(vector<uint32_t> const & features) const;

  template <typename TFn>
  void Filter(vector<uint32_t> const & features, TFn && fn)
  {
    using TRankAndFeature = pair<uint8_t, uint32_t>;
    using TComparer = std::greater<TRankAndFeature>;

    UpdateCache();

    if (!m_featuresCache || m_featuresCache->PopCount() == 0)
      return;
    ASSERT(m_featuresCache.get(), ());

    // Emit all features from the viewport.
    for (uint32_t feature : features)
    {
      if (m_featuresCache->GetBit(feature))
        fn(feature);
    }
  }

private:
  void UpdateCache();

  m2::RectD m_viewport;
  size_t m_maxNumResults;
  int m_scale;

  unique_ptr<coding::CompressedBitVector> m_featuresCache;
  bool m_cacheIsValid;

  MwmValue * m_value;
  MwmSet::MwmId m_id;
  my::Cancellable const & m_cancellable;
};
}  // namespace v2
}  // namespace search
