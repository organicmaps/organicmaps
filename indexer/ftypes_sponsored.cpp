#include "indexer/ftypes_sponsored.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"

#include "base/string_utils.hpp"

#include "private.h"

#include <algorithm>

namespace ftypes
{
BaseSponsoredChecker::BaseSponsoredChecker(std::string const & sponsoredType)
{
  m_types.push_back(classif().GetTypeByPath({"sponsored", sponsoredType}));
}

SponsoredPartnerChecker::SponsoredPartnerChecker()
{
  Classificator const & c = classif();
  for (size_t i = 1; i <= MAX_PARTNERS_COUNT; i++)
    m_types.push_back(c.GetTypeByPath({"sponsored", "partner" + strings::to_string(i)}));
}

int SponsoredPartnerChecker::GetPartnerIndex(FeatureType const & ft) const
{
  auto const types = feature::TypesHolder(ft);
  int index = 0;
  for (auto t : m_types)
  {
    if (std::find(types.begin(), types.end(), PrepareToMatch(t, 2 /* level */)) != types.end())
      return index;
    index++;
  }
  return -1;
}
}  // namespace ftypes
