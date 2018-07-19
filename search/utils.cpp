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

namespace
{
vector<strings::UniString> const kAllowedMisprints = {
    strings::MakeUniString("ckq"),
    strings::MakeUniString("eyjiu"),
    strings::MakeUniString("gh"),
    strings::MakeUniString("pf"),
    strings::MakeUniString("vw"),
    strings::MakeUniString("ао"),
    strings::MakeUniString("еиэ"),
    strings::MakeUniString("шщ"),
};
}  // namespace

namespace search
{
size_t GetMaxErrorsForToken(strings::UniString const & token)
{
  bool const digitsOnly = all_of(token.begin(), token.end(), ::isdigit);
  if (digitsOnly)
    return 0;
  if (token.size() < 4)
    return 0;
  if (token.size() < 8)
    return 1;
  return 2;
}

strings::LevenshteinDFA BuildLevenshteinDFA(strings::UniString const & s)
{
  // In search we use LevenshteinDFAs for fuzzy matching. But due to
  // performance reasons, we limit prefix misprints to fixed set of substitutions defined in
  // kAllowedMisprints and skipped letters.
  return strings::LevenshteinDFA(s, 1 /* prefixSize */, kAllowedMisprints, GetMaxErrorsForToken(s));
}

vector<uint32_t> GetCategoryTypes(string const & name, string const & locale,
                                  CategoriesHolder const & categories)
{
  vector<uint32_t> types;

  int8_t const code = CategoriesHolder::MapLocaleToInteger(locale);
  Locales locales;
  locales.Insert(static_cast<uint64_t>(code));

  vector<strings::UniString> tokens;
  SplitUniString(search::NormalizeAndSimplifyString(name), MakeBackInsertFunctor(tokens),
                 search::Delimiters());

  FillCategories(QuerySliceOnRawStrings<vector<strings::UniString>>(tokens, {} /* prefix */),
                 locales, categories, types);

  my::SortUnique(types);
  return types;
}

MwmSet::MwmHandle FindWorld(DataSource const & dataSource,
                            vector<shared_ptr<MwmInfo>> const & infos)
{
  MwmSet::MwmHandle handle;
  for (auto const & info : infos)
  {
    if (info->GetType() == MwmInfo::WORLD)
    {
      handle = dataSource.GetMwmHandleById(MwmSet::MwmId(info));
      break;
    }
  }
  return handle;
}

MwmSet::MwmHandle FindWorld(DataSource const & dataSource)
{
  vector<shared_ptr<MwmInfo>> infos;
  dataSource.GetMwmsInfo(infos);
  return FindWorld(dataSource, infos);
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
    auto & value = *handle.GetValue<MwmValue>();
    if (!value.HasSearchIndex())
      continue;

    MwmContext const mwmContext(move(handle));
    auto features = cache.Get(mwmContext);

    auto const pivotFeatures = pivotRectsCache.Get(mwmContext, pivot, scales::GetUpperScale());
    ViewportFilter const filter(pivotFeatures, 0 /* threshold */);
    features = filter.Filter(features);
    MwmSet::MwmId mwmId(info);
    features.ForEach([&fn, &mwmId](uint64_t bit) {
      fn(FeatureID(mwmId, ::base::asserted_cast<uint32_t>(bit)));
    });
  }
}
}  // namespace search
