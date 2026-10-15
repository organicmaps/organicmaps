#include "map/everywhere_search_callback.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <numeric>

namespace search
{
namespace
{
// Countries, states, cities, towns and villages are looked for by their names, wherever they are.
bool IsPlace(Result const & result)
{
  return result.GetResultType() != Result::Type::Feature ||
         ftypes::IsLocalityChecker::Instance().GetType(result.GetFeatureType()) != ftypes::LocalityType::None;
}
}  // namespace

void OrderNearestFirst(m2::PointD const & pivot, std::function<bool(Result const &)> const & isOnTop, Results & results,
                       std::vector<ProductInfo> & productInfo)
{
  size_t const count = results.GetCount();
  ASSERT_EQUAL(productInfo.size(), count, ());
  // The results staying first are nearer than any.
  std::vector<double> distances(count, -1);
  for (size_t i = 0; i < count; ++i)
  {
    auto const & result = results[i];
    ASSERT_EQUAL(result.GetPositionInResults(), static_cast<int32_t>(i), ());
    if (!result.IsSuggest() && result.HasPoint() && !isOnTop(result))
      distances[i] = mercator::DistanceOnEarth(pivot, result.GetFeatureCenter());
  }
  std::vector<size_t> order(count);
  std::iota(order.begin(), order.end(), 0);
  std::stable_sort(order.begin(), order.end(),
                   [&distances](size_t a, size_t b) { return distances[a] < distances[b]; });

  std::vector<size_t> place(count);
  for (size_t i = 0; i < count; ++i)
    place[order[i]] = i;
  results.SortBy([&place](Result const & a, Result const & b)
  { return place[a.GetPositionInResults()] < place[b.GetPositionInResults()]; });
  std::vector<ProductInfo> ordered;
  ordered.reserve(count);
  for (size_t const i : order)
    ordered.push_back(productInfo[i]);
  productInfo = std::move(ordered);
}

EverywhereSearchCallback::EverywhereSearchCallback(Delegate & delegate, OnResults onResults,
                                                   std::optional<m2::PointD> const & pivot)
  : m_delegate(delegate)
  , m_onResults(std::move(onResults))
  , m_pivot(pivot)
{
  CHECK(m_onResults, ());
}

void EverywhereSearchCallback::operator()(Results const & results)
{
  size_t const prevSize = m_productInfo.size();
  size_t const currSize = results.GetCount();
  ASSERT_LESS_OR_EQUAL(prevSize, currSize, ());

  for (size_t i = prevSize; i < currSize; ++i)
    m_productInfo.push_back(m_delegate.GetProductInfo(results[i]));

  // The core adds the results batch by batch: a near one found later is put first.
  Results ordered = results;
  std::vector<ProductInfo> productInfo = m_productInfo;
  if (m_pivot)
    OrderNearestFirst(*m_pivot, IsPlace, ordered, productInfo);

  m_delegate.RunUITask(
      [onResults = m_onResults, results = std::move(ordered), productInfo = std::move(productInfo)]() mutable
  { onResults(std::move(results), std::move(productInfo)); });
}
}  // namespace search
