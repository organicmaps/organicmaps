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

class IsSponsoredChecker : public BaseChecker
{
private:
  IsSponsoredChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsSponsoredChecker);
};

#define SPONSORED_CHECKER(ClassName, Category) \
  class ClassName : public BaseSponsoredChecker { \
  ClassName() : BaseSponsoredChecker(Category) {} \
  public: DECLARE_CHECKER_INSTANCE(ClassName); };

// Checkers.
SPONSORED_CHECKER(IsBookingChecker, "booking");
SPONSORED_CHECKER(IsOpentableChecker, "opentable");
SPONSORED_CHECKER(IsHolidayChecker, "holiday");
SPONSORED_CHECKER(IsPromoCatalogChecker, "promo_catalog");

#undef SPONSORED_CHECKER
}  // namespace ftypes
