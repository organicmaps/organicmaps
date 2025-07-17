#pragma once

#include "map/everywhere_search_params.hpp"
#include "map/search_product_info.hpp"

#include "search/result.hpp"

#include <functional>
#include <vector>

namespace search
{
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
  EverywhereSearchCallback(Delegate & delegate, OnResults onResults);

  void operator()(Results const & results);

private:
  Delegate & m_delegate;
  OnResults m_onResults;
  std::vector<ProductInfo> m_productInfo;
};
}  // namespace search
