#pragma once

#include <algorithm>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace products
{
struct ProductsConfig
{
  struct Product
  {
    std::string title;
    std::string link;
  };
  std::optional<std::string> placePagePrompt;
  std::vector<Product> products;

  bool HasProducts() const { return !products.empty(); }
  bool IsValid() const
  {
    return std::ranges::all_of(products,
                               [](auto const & product) { return !product.title.empty() && !product.link.empty(); });
  }

  static std::optional<ProductsConfig> LoadFromString(std::string const & jsonString);
  static std::optional<ProductsConfig> LoadFromFile(std::string const & jsonFilePath);
};

class ProductsSettings
{
private:
  ProductsSettings();

  std::optional<ProductsConfig> m_productsConfig;
  mutable std::mutex m_mutex;

public:
  static ProductsSettings & Instance();

  void Update(std::optional<ProductsConfig> && productsConfig);
  std::optional<ProductsConfig> Get() const;
};

}  // namespace products
