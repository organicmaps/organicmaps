#pragma once

#include "platform/safe_callback.hpp"

#include <functional>
#include <string>
#include <vector>

namespace viator
{
class RawApi
{
public:
  /// Returns top <count> products for specified city id.
  static bool GetTopProducts(std::string const & destId, std::string const & currency, int count,
                             std::string & result);
};

struct Product
{
  std::string m_title;
  double m_rating;
  int m_reviewCount;
  std::string m_duration;
  double m_price;
  std::string m_priceFormatted;
  std::string m_currency;
  std::string m_photoUrl;
  std::string m_pageUrl;
};

using GetTop5ProductsCallback =
    platform::SafeCallback<void(std::string const & destId, std::vector<Product> const & products)>;

class Api
{
public:
  /// Returns web page address for specified city id.
  static std::string GetCityUrl(std::string const & destId);

  /// Returns top-5 products for specified city id.
  /// @currency - currency of the price, if empty then USD will be used.
  void GetTop5Products(std::string const & destId, std::string const & currency,
                       GetTop5ProductsCallback const & fn) const;
};

bool operator<(Product const & lhs, Product const & rhs);
void SortProducts(std::vector<Product> & products);
}  // namespace viator
