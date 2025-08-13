#include "testing/testing.hpp"

#include "platform/products.hpp"

namespace products
{
UNIT_TEST(ProductsConfig_ValidConfig)
{
  std::string const json = R"({
    "placePagePrompt": "prompt1",
    "products": [
      {
        "title": "Product 1",
        "link": "http://product1"
      },
      {
        "title": "Product 2",
        "link": "http://product2"
      }
    ]
  })";

  auto const productsConfig = ProductsConfig::LoadFromString(json);
  TEST(productsConfig, ());
  TEST_EQUAL(productsConfig->placePagePrompt, "prompt1", ());

  auto const & products = productsConfig->products;
  TEST_EQUAL(products.size(), 2, ());
  TEST_EQUAL(products[0].title, "Product 1", ());
  TEST_EQUAL(products[0].link, "http://product1", ());
  TEST_EQUAL(products[1].title, "Product 2", ());
  TEST_EQUAL(products[1].link, "http://product2", ());
}

UNIT_TEST(ProductsConfig_EmptyPrompts)
{
  std::string const json = R"({
    "products": [
      {
        "title": "Product 1",
        "link": "http://product1"
      },
      {
        "title": "Product 2",
        "link": "http://product2"
      }
    ]
  })";

  auto const productsConfig = ProductsConfig::LoadFromString(json);
  TEST(productsConfig, ());
  TEST(!productsConfig->placePagePrompt, ());
  TEST_EQUAL(productsConfig->products.size(), 2, ());
}

UNIT_TEST(ProductsConfig_InvalidProduct)
{
  std::string const json = R"({
    "placePagePrompt": "prompt1",
    "products": [
      {
        "title": "Product 1"
      },
      {
        "title": "Product 2",
        "link": "http://product2"
      }
    ]
  })";

  auto const productsConfig = ProductsConfig::LoadFromString(json);
  TEST(!productsConfig, ());
}

UNIT_TEST(ProductsConfig_EmptyProducts)
{
  std::string const json = R"({
    "placePagePrompt": "prompt1",
    "products": []
  })";

  auto const productsConfig = ProductsConfig::LoadFromString(json);
  TEST(!productsConfig, ("Empty products list"));
}

UNIT_TEST(ProductsConfig_MissedProductsField)
{
  std::string const json = R"({
    "placePagePrompt": "prompt1"
  })";

  auto const productsConfig = ProductsConfig::LoadFromString(json);
  TEST(!productsConfig, ());
}
}  // namespace products
