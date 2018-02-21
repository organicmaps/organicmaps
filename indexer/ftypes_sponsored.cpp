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

std::string GetPartnerNameByIndex(int partnerIndex)
{
  static std::vector<std::string> kIds = {PARTNER1_NAME, PARTNER2_NAME, PARTNER3_NAME,
                                          PARTNER4_NAME, PARTNER5_NAME};
  if (partnerIndex < 0 || partnerIndex >= kIds.size())
    return {};
  return kIds[partnerIndex];
}

bool IsPartnerButtonExist(int partnerIndex)
{
  static std::vector<bool> kButtons = {PARTNER1_HAS_BUTTON, PARTNER2_HAS_BUTTON, PARTNER3_HAS_BUTTON,
                                       PARTNER4_HAS_BUTTON, PARTNER5_HAS_BUTTON};
  if (partnerIndex < 0 || partnerIndex >= kButtons.size())
    return false;
  return kButtons[partnerIndex];
}
