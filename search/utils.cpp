#include "search/utils.hpp"

#include "search/categories_cache.hpp"
#include "search/features_filter.hpp"
#include "search/geometry_cache.hpp"
#include "search/mwm_context.hpp"

#include "indexer/data_source.hpp"

#include "base/cancellable.hpp"

#include <cctype>
#include <utility>

using namespace std;

namespace search
{
vector<uint32_t> GetCategoryTypes(string const & name, string const & locale,
                                  CategoriesHolder const & categories)
{
  vector<uint32_t> types;

  int8_t const code = CategoriesHolder::MapLocaleToInteger(locale);
  Locales locales;
  locales.Insert(static_cast<uint64_t>(code));

  auto const tokens = NormalizeAndTokenizeString(name);

  FillCategories(QuerySliceOnRawStrings<vector<strings::UniString>>(tokens, {} /* prefix */),
                 locales, categories, types);

  base::SortUnique(types);
  return types;
}

void ForEachOfTypesInRect(DataSource const & dataSource, vector<uint32_t> const & types,
                          m2::RectD const & pivot, FeatureIndexCallback const & fn)
{
  vector<shared_ptr<MwmInfo>> infos;
  dataSource.GetMwmsInfo(infos);

  base::Cancellable const cancellable;
  CategoriesCache cache(types, cancellable);
  auto pivotRectsCache = PivotRectsCache(1 /* maxNumEntries */, cancellable,
                                         max(pivot.SizeX(), pivot.SizeY()) /* maxRadiusMeters */);

  for (auto const & info : infos)
  {
    if (!pivot.IsIntersect(info->m_bordersRect))
      continue;

    auto handle = dataSource.GetMwmHandleById(MwmSet::MwmId(info));
    auto & value = *handle.GetValue();
    if (!value.HasSearchIndex())
      continue;

    MwmContext const mwmContext(move(handle));
    auto features = cache.Get(mwmContext);

    auto const pivotFeatures = pivotRectsCache.Get(mwmContext, pivot, scales::GetUpperScale());
    ViewportFilter const filter(pivotFeatures, 0 /* threshold */);
    features = filter.Filter(features);
    MwmSet::MwmId mwmId(info);
    features.ForEach([&fn, &mwmId](uint64_t bit) {
      fn(FeatureID(mwmId, base::asserted_cast<uint32_t>(bit)));
    });
  }
}

bool IsCategorialRequestFuzzy(string const & query, string const & categoryName)
{
  auto const & catHolder = GetDefaultCategories();
  auto const types = GetCategoryTypes(categoryName, "en", catHolder);

  auto const queryTokens = NormalizeAndTokenizeString(query);

  bool isCategorialRequest = false;
  for (auto const type : types)
  {
    if (isCategorialRequest)
      return true;

    catHolder.ForEachNameByType(type, [&](CategoriesHolder::Category::Name const & categorySynonym)
    {
      if (isCategorialRequest)
        return;

      auto const categoryTokens = NormalizeAndTokenizeString(categorySynonym.m_name);
      for (size_t start = 0; start < queryTokens.size(); ++start)
      {
        bool found = true;
        for (size_t i = 0; i < categoryTokens.size() && start + i < queryTokens.size(); ++i)
        {
          if (queryTokens[start + i] != categoryTokens[i])
          {
            found = false;
            break;
          }
        }
        if (found)
        {
          isCategorialRequest = true;
          break;
        }
      }
    });
  }

  return isCategorialRequest;
}
}  // namespace search
