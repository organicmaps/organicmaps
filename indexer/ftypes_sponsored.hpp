#pragma once

#include "indexer/ftypes_matcher.hpp"

#include <string>

namespace ftypes
{
class BaseSponsoredChecker : public BaseChecker
{
protected:
  explicit BaseSponsoredChecker(std::string const & sponsoredType);
};

#define SPONSORED_CHECKER(ClassName, Category) \
  class ClassName : public BaseSponsoredChecker { \
  ClassName() : BaseSponsoredChecker(Category) {} \
  public: DECLARE_CHECKER_INSTANCE(ClassName); };

// Checkers.
SPONSORED_CHECKER(IsBookingChecker, "booking");
SPONSORED_CHECKER(IsOpentableChecker, "opentable");
SPONSORED_CHECKER(IsViatorChecker, "viator");
SPONSORED_CHECKER(IsHolidayChecker, "holiday");

#undef SPONSORED_CHECKER

class SponsoredPartnerChecker : public BaseChecker
{
protected:
  SponsoredPartnerChecker();

public:
  int GetPartnerIndex(FeatureType const & ft) const;

  DECLARE_CHECKER_INSTANCE(SponsoredPartnerChecker);
};
}  // namespace ftypes

extern std::string GetPartnerNameByIndex(int partnerIndex);
extern bool IsPartnerButtonExist(int partnerIndex);
