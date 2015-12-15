#include "search/v2/features_filter.hpp"

#include "search/dummy_rank_table.hpp"
#include "search/retrieval.hpp"

#include "indexer/index.hpp"
#include "indexer/scales.hpp"

namespace search
{
namespace v2
{
FeaturesFilter::FeaturesFilter(my::Cancellable const & cancellable)
  : m_maxNumResults(0)
  , m_scale(scales::GetUpperScale())
  , m_cacheIsValid(false)
  , m_value(nullptr)
  , m_cancellable(cancellable)
{
}

void FeaturesFilter::SetValue(MwmValue * value, MwmSet::MwmId const & id)
{
  if (m_value == value && m_id == id)
    return;
  m_value = value;
  m_id = id;
  m_cacheIsValid = false;
}

void FeaturesFilter::SetViewport(m2::RectD const & viewport)
{
  if (viewport == m_viewport)
    return;
  m_viewport = viewport;
  m_cacheIsValid = false;
}

void FeaturesFilter::SetMaxNumResults(size_t maxNumResults) { m_maxNumResults = maxNumResults; }

void FeaturesFilter::SetScale(int scale)
{
  if (m_scale == scale)
    return;
  m_scale = scale;
  m_cacheIsValid = false;
}

bool FeaturesFilter::NeedToFilter(vector<uint32_t> const & features) const
{
  return features.size() > m_maxNumResults;
}

void FeaturesFilter::UpdateCache()
{
  if (m_cacheIsValid)
    return;

  if (!m_value)
  {
    m_featuresCache.reset();
  }
  else
  {
    m_featuresCache =
        Retrieval::RetrieveGeometryFeatures(*m_value, m_cancellable, m_viewport, m_scale);
  }
  m_cacheIsValid = true;
}
}  // namespace v2
}  // namespace search
