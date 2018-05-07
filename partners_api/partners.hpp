#pragma once

#include "indexer/ftypes_matcher.hpp"

#include <string>

struct PartnerInfo
{
  int const m_partnerIndex;
  std::string const m_type;
  std::string const m_name;
  bool const m_hasButton = false;
  std::string const m_iosBannerPlacementId;
  std::string const m_androidBannerPlacementId;

  PartnerInfo(int partnerIndex, std::string && name, bool hasButton,
              std::string && iosBannerPlacementId, std::string && androidBannerPlacementId);

  PartnerInfo(int partnerIndex, std::string && name, bool hasButton);

  PartnerInfo(int partnerIndex, std::string && name);

  std::string const & GetBannerPlacementId() const;
};

class PartnerChecker : public ftypes::BaseChecker
{
protected:
  PartnerChecker();

public:
  int GetPartnerIndex(FeatureType const & ft) const;

  DECLARE_CHECKER_INSTANCE(PartnerChecker);
};

extern std::vector<PartnerInfo> const & GetPartners();

extern PartnerInfo const & GetPartnerByIndex(int partnerIndex);
