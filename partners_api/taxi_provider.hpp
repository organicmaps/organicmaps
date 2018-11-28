#pragma once

#include "base/assert.hpp"

#include <string>
#include <vector>

namespace taxi
{
struct Product
{
  std::string m_productId; // unique id of product for provider
  std::string m_name;      // name of product
  std::string m_time;      // wait time
  std::string m_price;     // for some currencies this field contains symbol of currency but not always
  std::string m_currency;  // currency can be empty, for ex. when m_price equal to Metered
};

class Provider
{
public:
  enum Type
  {
    Uber,
    Yandex,
    Maxim,
    Rutaxi,
    Count
  };

  using Iter = std::vector<Product>::iterator;
  using ConstIter = std::vector<Product>::const_iterator;

  Provider(Type type, std::vector<Product> const & products) : m_type(type), m_products(products) {}

  Type GetType() const { return m_type; }
  std::vector<Product> const & GetProducts() const { return m_products; }
  Product const & operator[](size_t i) const
  {
    ASSERT_LESS(i, m_products.size(), ());
    return m_products[i];
  }
  Product & operator[](size_t i)
  {
    ASSERT_LESS(i, m_products.size(), ());
    return m_products[i];
  }

  Iter begin() { return m_products.begin(); }
  Iter end() { return m_products.end(); }
  ConstIter begin() const { return m_products.cbegin(); }
  ConstIter end() const { return m_products.cend(); }

private:
  Type m_type;
  std::vector<Product> m_products;
};

using ProvidersContainer = std::vector<Provider>;

enum class ErrorCode
{
  NoProducts,
  RemoteError,
};

struct ProviderError
{
  ProviderError(Provider::Type type, ErrorCode code) : m_type(type), m_code(code) {}

  Provider::Type m_type;
  ErrorCode m_code;
};

using ErrorsContainer = std::vector<ProviderError>;

inline std::string DebugPrint(Provider::Type type)
{
  switch (type)
  {
  case Provider::Type::Uber: return "Uber";
  case Provider::Type::Yandex: return "Yandex";
  case Provider::Type::Maxim: return "Maxim";
  case Provider::Type::Rutaxi: return "Rutaxi";
  case Provider::Type::Count: ASSERT(false, ()); return "";
  }
  UNREACHABLE();
}

inline std::string DebugPrint(ErrorCode code)
{
  switch (code)
  {
  case ErrorCode::NoProducts: return "NoProducts";
  case ErrorCode::RemoteError: return "RemoteError";
  }
  UNREACHABLE();
}

inline std::string DebugPrint(ProviderError error)
{
  return "ProviderError [" + DebugPrint(error.m_type) + ", " + DebugPrint(error.m_code) + "]";
}
}  // namespace taxi
