#pragma once

#include "search/categories_cache.hpp"
#include "search/mwm_context.hpp"

#include "indexer/mwm_set.hpp"

#include <map>
#include <memory>
#include <utility>
#include <vector>

class FeatureType;

namespace search
{
namespace cuisine_filter
{
struct Description
{
  Description() = default;
  Description(FeatureType & ft);

  std::vector<uint32_t> m_types;
};

class CuisineFilter
{
public:
  using Descriptions = std::vector<std::pair<uint32_t, Description>>;

  class ScopedFilter
  {
  public:
    ScopedFilter(MwmSet::MwmId const & mwmId, Descriptions const & descriptions, std::vector<uint32_t> const & types);

    bool Matches(FeatureID const & fid) const;

  private:
    MwmSet::MwmId const m_mwmId;
    Descriptions const & m_descriptions;
    std::vector<uint32_t> m_types;
  };

  CuisineFilter(FoodCache & food);

  std::unique_ptr<ScopedFilter> MakeScopedFilter(MwmContext const & context, std::vector<uint32_t> const & types);

  void ClearCaches();

private:
  Descriptions const & GetDescriptions(MwmContext const & context);

  FoodCache & m_food;
  std::map<MwmSet::MwmId, Descriptions> m_descriptions;
};
}  // namespace cuisine_filter
}  // namespace search
