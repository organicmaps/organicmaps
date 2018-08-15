#include "partners_api/partners.hpp"
#include "partners_api/partners_list.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"

#include "base/string_utils.hpp"

#include "std/target_os.hpp"

#include <algorithm>
#include <vector>
#include <utility>

namespace
{
int constexpr kFakePartnerIndex = -1;
PartnerInfo const kFakePartner(kFakePartnerIndex, {});
}  // namespace

PartnerInfo::PartnerInfo(int partnerIndex, std::string && name, bool hasButton,
                         std::string && defaultBannerUrl, uint64_t minMapVersion,
                         std::string && iosBannerPlacementId,
                         std::string && androidBannerPlacementId)
  : m_partnerIndex(partnerIndex)
  , m_type("partner" + strings::to_string(partnerIndex))
  , m_name(std::move(name))
  , m_hasButton(hasButton)
  , m_defaultBannerUrl(std::move(defaultBannerUrl))
  , m_minMapVersion(minMapVersion)
  , m_iosBannerPlacementId(std::move(iosBannerPlacementId))
  , m_androidBannerPlacementId(std::move(androidBannerPlacementId))
{}

PartnerInfo::PartnerInfo(int partnerIndex, std::string && name, bool hasButton,
                         std::string && defaultBannerUrl, uint64_t minMapVersion)
  : PartnerInfo(partnerIndex, std::move(name), hasButton,
                std::move(defaultBannerUrl), minMapVersion,
                {} /* m_iosBannerPlacementId */,
                {} /* m_androidBannerPlacementId */)
{}

PartnerInfo::PartnerInfo(int partnerIndex, std::string && name, bool hasButton,
                         std::string && defaultBannerUrl)
  : PartnerInfo(partnerIndex, std::move(name), hasButton, std::move(defaultBannerUrl),
                0 /* m_minMapVersion */)
{}

PartnerInfo::PartnerInfo(int partnerIndex, std::string && name, bool hasButton)
  : PartnerInfo(partnerIndex, std::move(name), hasButton, {} /* m_defaultBannerUrl */)
{}

PartnerInfo::PartnerInfo(int partnerIndex, std::string && name)
  : PartnerInfo(partnerIndex, std::move(name), false /* hasButton */)
{}

std::string const & PartnerInfo::GetBannerPlacementId() const
{
#if defined(OMIM_OS_IPHONE)
  return m_iosBannerPlacementId;
#elif defined(OMIM_OS_ANDROID)
  return m_androidBannerPlacementId;
#endif
  static std::string kEmptyStr;
  return kEmptyStr;
}

PartnerChecker::PartnerChecker()
{
  Classificator const & c = classif();
  for (auto const & p : kPartners)
    m_types.push_back(c.GetTypeByPath({"sponsored", p.m_type}));
}

int PartnerChecker::GetPartnerIndex(FeatureType & ft) const
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

bool PartnerChecker::IsFakeObject(FeatureType & ft) const
{
  // An object is fake one if it contains only sponsored-partnerX types.
  auto const types = feature::TypesHolder(ft);
  for (auto t : types)
  {
    if (std::find(m_types.begin(), m_types.end(), PrepareToMatch(t, 2 /* level */)) == m_types.end())
      return false;
  }
  return true;
}

std::vector<PartnerInfo> const & GetPartners()
{
  return kPartners;
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
