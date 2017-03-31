#pragma once

#include "indexer/feature_data.hpp"

#include "base/macros.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace ads
{
class ContainerBase
{
public:
  virtual ~ContainerBase() = default;
  virtual bool HasBanner(feature::TypesHolder const & types) const = 0;
  virtual std::string GetBannerId(feature::TypesHolder const & types) const = 0;
  virtual std::string GetBannerIdForOtherTypes() const = 0;
};

// Class which matches feature types and banner ids.
class Container : public ContainerBase
{
public:
  Container();

  // ContainerBase overrides:
  bool HasBanner(feature::TypesHolder const & types) const override;
  std::string GetBannerId(feature::TypesHolder const & types) const override;
  std::string GetBannerIdForOtherTypes() const override;

protected:
  void AppendEntry(std::vector<std::vector<std::string>> const & types, std::string const & id);
  void AppendExcludedTypes(std::vector<std::vector<std::string>> const & types);

private:
  std::unordered_map<uint32_t, std::string> m_typesToBanners;
  std::unordered_set<uint32_t> m_excludedTypes;

  DISALLOW_COPY(Container);
};
}  // namespace ads
