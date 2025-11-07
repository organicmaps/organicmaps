#include "platform/products.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "defines.hpp"

#include "coding/file_writer.hpp"

#include "glaze/core/opts.hpp"
#include "glaze/json.hpp"

namespace products
{

std::string GetProductsFilePath()
{
  return GetPlatform().SettingsPathForFile(PRODUCTS_SETTINGS_FILE_NAME);
}

static bool IsProductsConfigValid(ProductsConfig const & config)
{
  return config.HasProducts() && std::ranges::all_of(config.products, [](auto const & product)
  { return !product.link.empty() && !product.title.empty(); });
}

std::optional<ProductsConfig> ProductsConfig::LoadFromFile(std::string const & jsonFilePath)
{
  std::string jsonString;
  try
  {
    // Do not use glaze file reader because it uses std::filesystem which works only on iOS 13+.
    GetPlatform().GetReader(jsonFilePath)->ReadAsString(jsonString);
  }
  catch (RootException const & ex)
  {
    LOG(LERROR, ("Failed to load data from JSON file", jsonFilePath, "with error:", ex.Msg()));
    return {};
  }

  return LoadFromString(jsonString);
}

std::optional<ProductsConfig> ProductsConfig::LoadFromString(std::string const & jsonString)
{
  ProductsConfig config;
  glz::context ctx;
  auto const error = read<glz::opts{.error_on_missing_keys = true}>(config, jsonString, ctx);
  if (error)
  {
    LOG(LWARNING, ("Error", glz::format_error(error), "when decoding ProductsConfig from JSON", jsonString));
    return {};
  }

  if (!IsProductsConfigValid(config))
  {
    LOG(LWARNING, ("Invalid ProductsConfig in JSON", jsonString));
    return {};
  }

  return config;
}

ProductsSettings::ProductsSettings()
{
  auto const path = GetProductsFilePath();
  std::lock_guard guard(m_mutex);
  if (Platform::IsFileExistsByFullPath(path))
    m_productsConfig = ProductsConfig::LoadFromFile(path);
  else
    LOG(LINFO, ("ProductsConfig file not found."));
}

ProductsSettings & ProductsSettings::Instance()
{
  static ProductsSettings instance;
  return instance;
}

std::optional<ProductsConfig> ProductsSettings::Get() const
{
  std::lock_guard guard(m_mutex);
  return m_productsConfig;
}

void ProductsSettings::Update(std::optional<ProductsConfig> && productsConfig)
{
  auto const outFilePath = GetProductsFilePath();
  std::lock_guard guard(m_mutex);
  if (!productsConfig)
  {
    m_productsConfig.reset();
    FileWriter::DeleteFileX(outFilePath);
  }
  else
  {
    m_productsConfig = std::move(productsConfig);
    if (auto const error = glz::write_file_json(productsConfig, outFilePath, std::string{}); error)
      LOG(LERROR, ("Error writing ProductsConfig file", outFilePath, glz::format_error(error)));
  }
}

}  // namespace products
