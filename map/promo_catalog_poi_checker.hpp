#pragma once

#include "partners_api/promo_catalog_types.hpp"

#include "indexer/ftypes_matcher.hpp"

#include <string>

namespace ftypes
{
class IsPromoCatalogPoiChecker : public BaseChecker
{
protected:
  IsPromoCatalogPoiChecker(promo::TypesList const & types)
  {
    for (auto const & type : types)
    {
      m_types.push_back(classif().GetTypeByPath(type));
    }
  }
};

class IsPromoCatalogSightseeingsChecker : public IsPromoCatalogPoiChecker
{
public:
  DECLARE_CHECKER_INSTANCE(IsPromoCatalogSightseeingsChecker);

private:
  IsPromoCatalogSightseeingsChecker()
    : IsPromoCatalogPoiChecker(promo::GetPromoCatalogSightseeingsTypes())
  {}
};
}  // namespace ftypes
