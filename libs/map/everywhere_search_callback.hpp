#pragma once

#include "map/everywhere_search_params.hpp"
#include "map/search_product_info.hpp"

#include "search/result.hpp"

#include "geometry/point2d.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace search
{
/// Orders the results nearest to |pivot| first. The suggestions, the results without a point and the results
/// |isOnTop| tells stay first, in their order. The core weighs the distance little: a street of the same name in a
/// bigger town far away is ranked above the near one.
void OrderNearestFirst(m2::PointD const & pivot, std::function<bool(Result const &)> const & isOnTop, Results & results,
                       std::vector<ProductInfo> & productInfo);

/// @brief An on-results-callback that should be used for search over all maps.
/// @note NOT thread safe.
class EverywhereSearchCallback
{
public:
  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual void RunUITask(std::function<void()> fn) = 0;
    virtual ProductInfo GetProductInfo(Result const & result) const = 0;
  };

  using OnResults = EverywhereSearchParams::OnResults;
  /// \param pivot the results are ordered nearest to it first, the places they are in stay first.
  EverywhereSearchCallback(Delegate & delegate, OnResults onResults, std::optional<m2::PointD> const & pivot);

  void operator()(Results const & results);

private:
  Delegate & m_delegate;
  OnResults m_onResults;
  std::optional<m2::PointD> m_pivot;
  std::vector<ProductInfo> m_productInfo;
};
}  // namespace search
