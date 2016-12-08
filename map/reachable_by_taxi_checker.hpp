#pragma once

#include "indexer/ftypes_matcher.hpp"

#include "base/macros.hpp"

namespace place_page
{
class IsReachableByTaxiChecker : public ftypes::BaseChecker
{
public:
  static IsReachableByTaxiChecker const & Instance();
  bool IsMatched(uint32_t type) const override;

private:
  IsReachableByTaxiChecker();
  DISALLOW_COPY_AND_MOVE(IsReachableByTaxiChecker);
};
}  // namespace place_page
