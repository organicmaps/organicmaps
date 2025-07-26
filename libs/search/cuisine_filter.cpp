#include "search/cuisine_filter.hpp"

#include "indexer/cuisines.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "platform/mwm_traits.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <algorithm>

using namespace std;

namespace search
{
namespace cuisine_filter
{
// Description -------------------------------------------------------------------------------------
Description::Description(FeatureType & ft)
{
  m_types.clear();
  ft.ForEachType([this](uint32_t t)
  {
    if (ftypes::IsCuisineChecker::Instance()(t))
      m_types.push_back(t);
  });
}

CuisineFilter::ScopedFilter::ScopedFilter(MwmSet::MwmId const & mwmId, Descriptions const & descriptions,
                                          vector<uint32_t> const & types)
  : m_mwmId(mwmId)
  , m_descriptions(descriptions)
  , m_types(types)
{
  sort(m_types.begin(), m_types.end());
}

bool CuisineFilter::ScopedFilter::Matches(FeatureID const & fid) const
{
  if (fid.m_mwmId != m_mwmId)
    return false;

  auto it = lower_bound(m_descriptions.begin(), m_descriptions.end(), make_pair(fid.m_index, Description{}),
                        [](pair<uint32_t, Description> const & lhs, pair<uint32_t, Description> const & rhs)
  { return lhs.first < rhs.first; });
  if (it == m_descriptions.end() || it->first != fid.m_index)
    return false;

  for (auto const t : it->second.m_types)
    if (binary_search(m_types.begin(), m_types.end(), t))
      return true;
  return false;
}

// CuisineFilter ------------------------------------------------------------------------------------
CuisineFilter::CuisineFilter(FoodCache & food) : m_food(food) {}

unique_ptr<CuisineFilter::ScopedFilter> CuisineFilter::MakeScopedFilter(MwmContext const & context,
                                                                        vector<uint32_t> const & types)
{
  if (types.empty())
    return {};
  return make_unique<ScopedFilter>(context.GetId(), GetDescriptions(context), types);
}

void CuisineFilter::ClearCaches()
{
  m_descriptions.clear();
}

CuisineFilter::Descriptions const & CuisineFilter::GetDescriptions(MwmContext const & context)
{
  auto const & mwmId = context.GetId();
  auto const it = m_descriptions.find(mwmId);
  if (it != m_descriptions.end())
    return it->second;

  auto & value = context.m_value;
  version::MwmTraits mwmTraits(value.GetMwmVersion());

  auto const food = m_food.Get(context);
  auto & descriptions = m_descriptions[mwmId];
  food.ForEach([&descriptions, &context](uint64_t bit)
  {
    auto const id = base::asserted_cast<uint32_t>(bit);
    auto ft = context.GetFeature(id);
    if (ft)
      descriptions.emplace_back(id, Description(*ft));
  });
  return descriptions;
}
}  // namespace cuisine_filter
}  // namespace search
