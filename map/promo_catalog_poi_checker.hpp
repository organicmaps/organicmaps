#pragma once

#include "partners_api/promo_catalog_types.hpp"

#include "indexer/ftypes_matcher.hpp"

#include <string>

namespace ftypes
{
class IsPromoCatalogPoiChecker : public BaseChecker
{
public:
  DECLARE_CHECKER_INSTANCE(IsPromoCatalogPoiChecker);

private:
  IsPromoCatalogPoiChecker()
  {
    auto const & types = promo::GetPromoCatalogPoiTypes();
    for (auto const & type : types)
    {
      m_types.push_back(classif().GetTypeByPath(type));
    }
  }
};
}  // namespace ftypes
