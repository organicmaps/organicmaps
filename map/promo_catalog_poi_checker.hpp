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

#define PROMO_CATALOG_CHECKER(ClassName, TypesGetter) \
    class ClassName : public IsPromoCatalogPoiChecker \
    {                                                 \
    public:                                           \
      DECLARE_CHECKER_INSTANCE(ClassName);            \
    private:                                          \
      ClassName()                                     \
        : IsPromoCatalogPoiChecker(TypesGetter)       \
      {}                                              \
    };

PROMO_CATALOG_CHECKER(IsPromoCatalogSightseeingsChecker, promo::GetPromoCatalogSightseeingsTypes())
PROMO_CATALOG_CHECKER(IsPromoCatalogOutdoorChecker, promo::GetPromoCatalogOutdoorTypes())
}  // namespace ftypes
