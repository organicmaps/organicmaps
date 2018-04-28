#include "partners_api/partners.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"

#include "base/string_utils.hpp"

#include "std/target_os.hpp"

#include <algorithm>
#include <vector>
#include <utility>

std::vector<PartnerInfo> const kPartners = {
  PartnerInfo(1, "Megabonus.Ru", true /* m_hasButton */,
              "e0c1d4d9c88c4670b9541116bcb75c5f" /* m_iosBannerPlacementId */,
              "2bab47102d38485996788ab9b602ce2c" /* m_androidBannerPlacementId */),

  PartnerInfo(2, "LuggageHero"),

  PartnerInfo(3, "BurgerKing", true /* m_hasButton */)
};

namespace
{
int constexpr kFakePartnerIndex = -1;
PartnerInfo const kFakePartner(kFakePartnerIndex, {});
}  // namespace

PartnerInfo::PartnerInfo(int partnerIndex, std::string && name, bool hasButton,
                         std::string && iosBannerPlacementId, std::string && androidBannerPlacementId)
  : m_partnerIndex(partnerIndex)
  , m_type("partner" + strings::to_string(partnerIndex))
  , m_name(std::move(name))
  , m_hasButton(hasButton)
  , m_iosBannerPlacementId(std::move(iosBannerPlacementId))
  , m_androidBannerPlacementId(std::move(androidBannerPlacementId))
{}

PartnerInfo::PartnerInfo(int partnerIndex, std::string && name, bool hasButton)
  : PartnerInfo(partnerIndex, std::move(name), hasButton, {}, {})
{}

PartnerInfo::PartnerInfo(int partnerIndex, std::string && name)
  : PartnerInfo(partnerIndex, std::move(name), false /* hasButton */)
{}

std::string PartnerInfo::GetBannerPlacementId() const
{
#if defined(OMIM_OS_IPHONE)
  return m_iosBannerPlacementId;
#elif defined(OMIM_OS_ANDROID)
  return m_androidBannerPlacementId;
#endif
  return {};
}

PartnerChecker::PartnerChecker()
{
  Classificator const & c = classif();
  for (auto const & p : kPartners)
    m_types.push_back(c.GetTypeByPath({"sponsored", p.m_type}));
}

int PartnerChecker::GetPartnerIndex(FeatureType const & ft) const
{
  auto const types = feature::TypesHolder(ft);
  int index = 0;
  CHECK_EQUAL(kPartners.size(), m_types.size(), ());
  for (auto t : m_types)
  {
    if (std::find(types.begin(), types.end(), PrepareToMatch(t, 2 /* level */)) != types.end())
      return kPartners[index].m_partnerIndex;
    index++;
  }
  return kFakePartnerIndex;
}

PartnerInfo const & GetPartnerByIndex(int partnerIndex)
{
  auto const it = std::find_if(kPartners.cbegin(), kPartners.cend(),
                               [partnerIndex](PartnerInfo const & partherInfo)
  {
    return partnerIndex == partherInfo.m_partnerIndex;
  });

  if (it == kPartners.cend())
    return kFakePartner;
  return *it;
}
