#include "search/search_quality/matcher.hpp"

#include "search/feature_loader.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/string_utils.hpp"

#include "geometry/mercator.hpp"

#include "base/control_flow.hpp"
#include "base/stl_add.hpp"

namespace search
{
// static
size_t constexpr Matcher::kInvalidId;

Matcher::Matcher(FeatureLoader & loader) : m_loader(loader) {}

void Matcher::Match(std::vector<Sample::Result> const & golden, std::vector<Result> const & actual,
                    std::vector<size_t> & goldenMatching, std::vector<size_t> & actualMatching)
{
  auto const n = golden.size();
  auto const m = actual.size();

  goldenMatching.assign(n, kInvalidId);
  actualMatching.assign(m, kInvalidId);

  // TODO (@y, @m): use Kuhn algorithm here for maximum matching.
  for (size_t i = 0; i < n; ++i)
  {
    if (goldenMatching[i] != kInvalidId)
      continue;
    auto const & g = golden[i];

    for (size_t j = 0; j < m; ++j)
    {
      if (actualMatching[j] != kInvalidId)
        continue;

      auto const & a = actual[j];
      if (Matches(g, a))
      {
        goldenMatching[i] = j;
        actualMatching[j] = i;
        break;
      }
    }
  }
}

bool Matcher::Matches(Sample::Result const & golden, FeatureType & ft)
{
  static double constexpr kToleranceMeters = 50;

  auto const houseNumber = ft.GetHouseNumber();
  auto const center = feature::GetCenter(ft);

  bool nameMatches = false;
  if (golden.m_name.empty())
  {
    nameMatches = true;
  }
  else
  {
    ft.ForEachName([&golden, &nameMatches](int8_t /* lang */, string const & name) {
      if (NormalizeAndSimplifyString(ToUtf8(golden.m_name)) == NormalizeAndSimplifyString(name))
      {
        nameMatches = true;
        return base::ControlFlow::Break;
      }
      return base::ControlFlow::Continue;
    });
  }

  bool houseNumberMatches = true;
  if (!golden.m_houseNumber.empty() && !houseNumber.empty())
    houseNumberMatches = golden.m_houseNumber == houseNumber;

  return nameMatches && houseNumberMatches &&
         MercatorBounds::DistanceOnEarth(golden.m_pos, center) <
             kToleranceMeters;
}

bool Matcher::Matches(Sample::Result const & golden, search::Result const & actual)
{
  if (actual.GetResultType() != Result::RESULT_FEATURE)
    return false;

  FeatureType ft;
  if (!m_loader.Load(actual.GetFeatureID(), ft))
    return false;

  return Matches(golden, ft);
}
}  // namespace search
