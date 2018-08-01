#include "indexer/ftypes_sponsored.hpp"

#include "indexer/classificator.hpp"

#include "private.h"

namespace ftypes
{
BaseSponsoredChecker::BaseSponsoredChecker(std::string const & sponsoredType)
{
  m_types.push_back(classif().GetTypeByPath({"sponsored", sponsoredType}));
}
}  // namespace ftypes
