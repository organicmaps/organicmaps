#pragma once

#include "indexer/ftypes_matcher.hpp"

#include <cstdint>
#include <string>

struct PartnerInfo
{
  int const m_partnerIndex;
  std::string const m_type;
  std::string const m_name;
  bool const m_hasButton = false;
  std::string const m_defaultBannerUrl;
  int64_t const m_minMapVersion = 0;
  std::string const m_iosBannerPlacementId;
  std::string const m_androidBannerPlacementId;

  PartnerInfo(int partnerIndex, std::string && name, bool hasButton,
              std::string && defaultBannerUrl, uint64_t minMapVersion,
              std::string && iosBannerPlacementId,
              std::string && androidBannerPlacementId);

  PartnerInfo(int partnerIndex, std::string && name, bool hasButton,
              std::string && defaultBannerUrl, uint64_t minMapVersion);

  PartnerInfo(int partnerIndex, std::string && name, bool hasButton,
              std::string && defaultBannerUrl);

  PartnerInfo(int partnerIndex, std::string && name, bool hasButton);

  PartnerInfo(int partnerIndex, std::string && name);

  std::string const & GetBannerPlacementId() const;
};

class PartnerChecker : public ftypes::BaseChecker
{
protected:
  PartnerChecker();

public:
  int GetPartnerIndex(FeatureType & ft) const;
  bool IsFakeObject(FeatureType & ft) const;

  DECLARE_CHECKER_INSTANCE(PartnerChecker);
};

extern std::vector<PartnerInfo> const & GetPartners();

extern PartnerInfo const & GetPartnerByIndex(int partnerIndex);
